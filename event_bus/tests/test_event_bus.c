#include <stdio.h>
#include "event_bus.h"

/* ============================================================
 *  Minimal test framework
 * ============================================================ */

static int tests_run  = 0;
static int tests_pass = 0;
static int tests_fail = 0;

#define TEST(name) \
    static int test_##name(void); \
    static int test_##name(void)

#define RUN_TEST(name) do { \
    tests_run++; \
    printf("  %-52s", #name); \
    int __ret = test_##name(); \
    if (__ret == 0) { \
        tests_pass++; \
        printf("\033[32mPASS\033[0m\n"); \
    } else { \
        tests_fail++; \
        printf("\033[31mFAIL\033[0m\n"); \
    } \
} while(0)

#define ASSERT(cond, msg) do { \
    if (!(cond)) { \
        printf("\n    \033[33m[%s:%d] %s\033[0m\n", __FILE__, __LINE__, msg); \
        return -1; \
    } \
} while(0)

#define ASSERT_EQ(a, b, msg) do { \
    if ((a) != (b)) { \
        printf("\n    \033[33m[%s:%d] %s (expected %d, got %d)\033[0m\n", \
               __FILE__, __LINE__, msg, (int)(b), (int)(a)); \
        return -1; \
    } \
} while(0)

#define ASSERT_PTR_EQ(a, b, msg) do { \
    if ((a) != (b)) { \
        printf("\n    \033[33m[%s:%d] %s (expected %p, got %p)\033[0m\n", \
               __FILE__, __LINE__, msg, (void*)(b), (void*)(a)); \
        return -1; \
    } \
} while(0)

/* ============================================================
 *  Shared test data — callback capture
 * ============================================================ */

static void *g_captured_data       = NULL;
static int   g_callback_call_count = 0;
static int   g_callback_order[8]   = {0};
static int   g_order_pos           = 0;

static void capture_cb(void *data)
{
    g_captured_data = data;
    g_callback_call_count++;
}

static void multi_call_cb(void *data)
{
    int *counter = (int*)data;
    (*counter)++;
}

static void order_cb0(void *data) { (void)data; g_callback_order[g_order_pos++] = 0; }
static void order_cb1(void *data) { (void)data; g_callback_order[g_order_pos++] = 1; }
static void order_cb2(void *data) { (void)data; g_callback_order[g_order_pos++] = 2; }
static void order_cb3(void *data) { (void)data; g_callback_order[g_order_pos++] = 3; }

static void reset_capture(void)
{
    g_captured_data       = NULL;
    g_callback_call_count = 0;
    g_order_pos           = 0;
    for (int i = 0; i < 8; i++) g_callback_order[i] = -1;
}

/* ---- helper: manually zero a struct as event_init would, without
 *              consuming a record slot. Used by subscribe/publish tests. ---- */
static void manual_init(event_type_t *evt)
{
    memset(evt, 0, sizeof(event_type_t));
}

/* ============================================================
 *  event_init() tests
 *
 *  IMPORTANT: event_bus.c has a static record[] of size 10.
 *  All event_init() calls across these tests share the same
 *  record — once 10 events are registered, further inits fail.
 *  Therefore the record_full test is placed LAST in this group,
 *  and subscribe/publish tests use manual_init() instead.
 * ============================================================ */

TEST(init_null_event_type)
{
    bool ret = event_init(NULL, "test");
    ASSERT(ret == false, "NULL event_type should return false");
    return 0;
}

TEST(init_null_name)
{
    event_type_t evt;
    bool ret = event_init(&evt, NULL);
    ASSERT(ret == false, "NULL name should return false");
    return 0;
}

TEST(init_both_null)
{
    bool ret = event_init(NULL, NULL);
    ASSERT(ret == false, "both NULL should return false");
    return 0;
}

/* ---- 1 slot ---- */
TEST(init_normal)
{
    event_type_t evt;
    bool ret = event_init(&evt, "sensor");
    ASSERT(ret == true, "normal init should succeed");
    ASSERT(evt.name != NULL, "name should not be NULL");
    ASSERT(strcmp(evt.name, "sensor") == 0, "name should be 'sensor'");
    ASSERT_EQ(evt.cb_index, 0, "cb_index should be 0 after init");
    ASSERT_PTR_EQ(evt.data, NULL, "data should be NULL after init");
    for (int i = 0; i < CB_NUM_MAX; i++) {
        ASSERT_PTR_EQ(evt.callback[i], NULL, "callback slot should be NULL");
    }
    return 0;
}

/* ---- 1 slot (total: 2) ---- */
TEST(init_zeroes_dirty_struct)
{
    event_type_t evt;
    memset(&evt, 0xFF, sizeof(evt));  /* dirty the struct */
    bool ret = event_init(&evt, "clean");
    ASSERT(ret == true, "init should succeed");
    ASSERT_EQ(evt.cb_index, 0, "cb_index should be 0 after init");
    ASSERT_PTR_EQ(evt.data, NULL, "data should be NULL after init");
    return 0;
}

/* ---- 3 slots (total: 5) ---- */
TEST(init_multiple_events)
{
    event_type_t evt1, evt2, evt3;
    bool r1 = event_init(&evt1, "e1");
    bool r2 = event_init(&evt2, "e2");
    bool r3 = event_init(&evt3, "e3");
    ASSERT(r1 == true && r2 == true && r3 == true, "3 inits should succeed");
    ASSERT(strcmp(evt1.name, "e1") == 0, "evt1 name");
    ASSERT(strcmp(evt2.name, "e2") == 0, "evt2 name");
    ASSERT(strcmp(evt3.name, "e3") == 0, "evt3 name");
    return 0;
}

/* ---- 2 slots (total: 7) ---- */
TEST(init_reinit_same_struct)
{
    event_type_t evt;
    bool r1 = event_init(&evt, "first");
    ASSERT(r1 == true, "first init should succeed");
    bool r2 = event_init(&evt, "second");
    ASSERT(r2 == true, "reinit should succeed");
    ASSERT(strcmp(evt.name, "second") == 0, "name should be overwritten");
    ASSERT_EQ(evt.cb_index, 0, "cb_index should be reset");
    return 0;
}

/* ---- fills remaining 3 slots → total 10, then 11th fails ---- */
TEST(init_record_full)
{
    /* we already consumed 7 slots; fill the remaining 3 */
    event_type_t fill[3];
    for (int i = 0; i < 3; i++) {
        char name[8];
        snprintf(name, sizeof(name), "f%d", i);
        bool ret = event_init(&fill[i], name);
        ASSERT(ret == true, "fill init should succeed");
    }
    /* 11th must fail */
    event_type_t extra;
    bool ret = event_init(&extra, "overflow");
    ASSERT(ret == false, "init beyond RECORD_NUM_MAX should fail");
    return 0;
}

/* ============================================================
 *  event_subscribe() tests  (manual_init — 0 record slots)
 * ============================================================ */

TEST(sub_null_event_type)
{
    bool ret = event_subscribe(NULL, capture_cb);
    ASSERT(ret == false, "NULL event_type should return false");
    return 0;
}

TEST(sub_null_callback)
{
    event_type_t evt;
    manual_init(&evt);
    bool ret = event_subscribe(&evt, NULL);
    ASSERT(ret == false, "NULL callback should return false");
    return 0;
}

TEST(sub_normal)
{
    event_type_t evt;
    manual_init(&evt);
    bool ret = event_subscribe(&evt, capture_cb);
    ASSERT(ret == true, "subscribe should succeed");
    ASSERT_EQ(evt.cb_index, 1, "cb_index should be 1");
    ASSERT_PTR_EQ(evt.callback[0], capture_cb, "callback should be stored");
    return 0;
}

TEST(sub_multiple)
{
    event_type_t evt;
    manual_init(&evt);

    event_callback_t cbs[] = {capture_cb, multi_call_cb, order_cb0,
                              order_cb1, order_cb2};
    for (int i = 0; i < 5; i++) {
        bool ret = event_subscribe(&evt, cbs[i]);
        ASSERT(ret == true, "subscribe should succeed");
        ASSERT_EQ(evt.cb_index, (uint8_t)(i + 1), "cb_index mismatch");
        ASSERT_PTR_EQ(evt.callback[i], cbs[i], "slot mismatch");
    }
    return 0;
}

TEST(sub_up_to_max_then_fail)
{
    event_type_t evt;
    manual_init(&evt);

    for (int i = 0; i < CB_NUM_MAX; i++) {
        bool ret = event_subscribe(&evt, capture_cb);
        ASSERT(ret == true, "subscribe within limit should succeed");
    }
    ASSERT_EQ(evt.cb_index, CB_NUM_MAX, "cb_index should be CB_NUM_MAX");

    /* 9th must fail */
    bool ret = event_subscribe(&evt, capture_cb);
    ASSERT(ret == false, "subscribe beyond CB_NUM_MAX should fail");
    ASSERT_EQ(evt.cb_index, CB_NUM_MAX, "cb_index should not change on failure");
    return 0;
}

/* ============================================================
 *  event_publish() tests  (manual_init + manual subscribe)
 * ============================================================ */

TEST(pub_zero_subscribers)
{
    event_type_t evt;
    manual_init(&evt);
    reset_capture();
    event_publish(&evt, "data");  /* should not crash */
    ASSERT_EQ(g_callback_call_count, 0, "no callbacks should fire");
    return 0;
}

TEST(pub_single_subscriber)
{
    event_type_t evt;
    manual_init(&evt);
    event_subscribe(&evt, capture_cb);

    reset_capture();
    event_publish(&evt, (void*)"hello");

    ASSERT_EQ(g_callback_call_count, 1, "callback should fire once");
    ASSERT_PTR_EQ(g_captured_data, (void*)"hello", "data should match");
    return 0;
}

TEST(pub_multiple_subscribers)
{
    event_type_t evt;
    manual_init(&evt);
    event_subscribe(&evt, capture_cb);
    event_subscribe(&evt, capture_cb);
    event_subscribe(&evt, capture_cb);

    reset_capture();
    event_publish(&evt, (void*)"multi");

    ASSERT_EQ(g_callback_call_count, 3, "all 3 callbacks should fire");
    ASSERT_PTR_EQ(g_captured_data, (void*)"multi", "last data should match");
    return 0;
}

TEST(pub_null_data)
{
    event_type_t evt;
    manual_init(&evt);
    event_subscribe(&evt, capture_cb);

    reset_capture();
    event_publish(&evt, NULL);

    ASSERT_EQ(g_callback_call_count, 1, "callback should fire with NULL data");
    ASSERT_PTR_EQ(g_captured_data, NULL, "callback should receive NULL");
    return 0;
}

TEST(pub_callback_order)
{
    event_type_t evt;
    manual_init(&evt);
    event_subscribe(&evt, order_cb0);
    event_subscribe(&evt, order_cb1);
    event_subscribe(&evt, order_cb2);
    event_subscribe(&evt, order_cb3);

    reset_capture();
    event_publish(&evt, NULL);

    ASSERT_EQ(g_order_pos, 4, "4 callbacks should fire");
    ASSERT_EQ(g_callback_order[0], 0, "1st should be cb0");
    ASSERT_EQ(g_callback_order[1], 1, "2nd should be cb1");
    ASSERT_EQ(g_callback_order[2], 2, "3rd should be cb2");
    ASSERT_EQ(g_callback_order[3], 3, "4th should be cb3");
    return 0;
}

TEST(pub_passes_int_pointer)
{
    event_type_t evt;
    manual_init(&evt);
    int counter = 0;
    event_subscribe(&evt, multi_call_cb);
    event_subscribe(&evt, multi_call_cb);
    event_subscribe(&evt, multi_call_cb);

    event_publish(&evt, &counter);
    ASSERT_EQ(counter, 3, "counter should be 3 (3 subscribers)");
    return 0;
}

/* ============================================================
 *  Multi-event isolation  (manual_init — 0 record slots)
 * ============================================================ */

TEST(isolation_publish)
{
    event_type_t evt_a, evt_b;
    manual_init(&evt_a);
    manual_init(&evt_b);

    int counter_a = 0, counter_b = 0;
    event_subscribe(&evt_a, multi_call_cb);
    event_subscribe(&evt_a, multi_call_cb);
    event_subscribe(&evt_b, multi_call_cb);

    /* publish to A only */
    event_publish(&evt_a, &counter_a);
    ASSERT_EQ(counter_a, 2, "counter_a should be 2");
    ASSERT_EQ(counter_b, 0, "counter_b should be 0");

    /* publish to B */
    event_publish(&evt_b, &counter_b);
    ASSERT_EQ(counter_a, 2, "counter_a still 2");
    ASSERT_EQ(counter_b, 1, "counter_b should be 1");
    return 0;
}

TEST(isolation_same_callback_different_events)
{
    event_type_t evt_a, evt_b;
    manual_init(&evt_a);
    manual_init(&evt_b);

    event_subscribe(&evt_a, capture_cb);
    event_subscribe(&evt_b, capture_cb);

    reset_capture();
    event_publish(&evt_a, (void*)"from_a");
    ASSERT_EQ(g_callback_call_count, 1, "only A fires");
    ASSERT_PTR_EQ(g_captured_data, (void*)"from_a", "data from A");

    reset_capture();
    event_publish(&evt_b, (void*)"from_b");
    ASSERT_EQ(g_callback_call_count, 1, "only B fires");
    ASSERT_PTR_EQ(g_captured_data, (void*)"from_b", "data from B");
    return 0;
}

/* ============================================================
 *  Stress / edge cases  (manual_init — 0 record slots)
 * ============================================================ */

TEST(stress_many_subs_on_one_event)
{
    event_type_t evt;
    manual_init(&evt);

    /* subscribe CB_NUM_MAX distinct callbacks */
    event_callback_t cbs[CB_NUM_MAX] = {
        capture_cb, multi_call_cb, order_cb0, order_cb1,
        order_cb2, order_cb3, capture_cb, multi_call_cb
    };
    for (int i = 0; i < CB_NUM_MAX; i++) {
        bool ret = event_subscribe(&evt, cbs[i]);
        ASSERT(ret == true, "subscribe should succeed");
    }
    ASSERT_EQ(evt.cb_index, CB_NUM_MAX, "cb_index should be CB_NUM_MAX");

    bool ret = event_subscribe(&evt, capture_cb);
    ASSERT(ret == false, "9th subscribe should fail");
    return 0;
}

/* ============================================================
 *  Main
 * ============================================================ */

int main(void)
{
    printf("\n");
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║         event_bus  Unit Tests                       ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n");
    printf("\n");

    printf("── event_init ──────────────────────────────────────\n");
    RUN_TEST(init_null_event_type);
    RUN_TEST(init_null_name);
    RUN_TEST(init_both_null);
    RUN_TEST(init_normal);
    RUN_TEST(init_zeroes_dirty_struct);
    RUN_TEST(init_multiple_events);
    RUN_TEST(init_reinit_same_struct);
    RUN_TEST(init_record_full);   /* MUST be last init test (fills record) */

    printf("\n── event_subscribe ─────────────────────────────────\n");
    RUN_TEST(sub_null_event_type);
    RUN_TEST(sub_null_callback);
    RUN_TEST(sub_normal);
    RUN_TEST(sub_multiple);
    RUN_TEST(sub_up_to_max_then_fail);

    printf("\n── event_publish ───────────────────────────────────\n");
    RUN_TEST(pub_zero_subscribers);
    RUN_TEST(pub_single_subscriber);
    RUN_TEST(pub_multiple_subscribers);
    RUN_TEST(pub_null_data);
    RUN_TEST(pub_callback_order);
    RUN_TEST(pub_passes_int_pointer);

    printf("\n── multi-event isolation ───────────────────────────\n");
    RUN_TEST(isolation_publish);
    RUN_TEST(isolation_same_callback_different_events);

    printf("\n── stress / edge cases ─────────────────────────────\n");
    RUN_TEST(stress_many_subs_on_one_event);

    printf("\n═══════════════════════════════════════════════════════\n");
    printf("  Total: %d  │  \033[32mPass: %d\033[0m  │  \033[31mFail: %d\033[0m", tests_run, tests_pass, tests_fail);
    if (tests_fail > 0) {
        printf("  ← \033[31mFAILURES\033[0m");
    } else {
        printf("  ← \033[32mALL PASSED\033[0m");
    }
    printf("\n═══════════════════════════════════════════════════════\n\n");

    return (tests_fail > 0) ? 1 : 0;
}
