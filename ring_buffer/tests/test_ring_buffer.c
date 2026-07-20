#include <stdio.h>
#include <assert.h>
#include "ring_buffer.h"

/* Simple test framework */
static int tests_run   = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) static void name(void)

#define RUN_TEST(name) do { \
    tests_run++; \
    printf("  RUN   %-50s ... ", #name); \
    int __prev_fail = tests_failed; \
    name(); \
    if (tests_failed == __prev_fail) { \
        tests_passed++; \
        printf("PASSED\n"); \
    } \
} while(0)

#define FAIL() do { \
    tests_failed++; \
    printf("FAILED\n"); \
    return; \
} while(0)

#define ASSERT_TRUE(expr) do { \
    if (!(expr)) { \
        printf("FAILED\n    %s:%d: ASSERT_TRUE(%s)\n", __FILE__, __LINE__, #expr); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))

#define ASSERT_EQ(a, b) do { \
    uint32_t _a = (uint32_t)(a); \
    uint32_t _b = (uint32_t)(b); \
    if (_a != _b) { \
        printf("FAILED\n    %s:%d: ASSERT_EQ(%s, %s) -> %u != %u\n", \
               __FILE__, __LINE__, #a, #b, _a, _b); \
        tests_failed++; \
        return; \
    } \
} while(0)

/* -------------------------------------------------------------------------- */
/* Test data type                                                             */
/* -------------------------------------------------------------------------- */

struct test_elem_t
{
    uint16_t num;
    uint8_t  cnt;
};

/* -------------------------------------------------------------------------- */
/* Init tests                                                                 */
/* -------------------------------------------------------------------------- */

TEST(test_init_null_ring_buf)
{
    struct test_elem_t buf[4];
    bool result = ring_buffer_init(NULL, buf, sizeof(buf), sizeof(struct test_elem_t));
    ASSERT_FALSE(result);
}

TEST(test_init_null_buffer)
{
    ring_buffer_t rb;
    bool result = ring_buffer_init(&rb, NULL, 8, sizeof(struct test_elem_t));
    ASSERT_FALSE(result);
}

TEST(test_init_non_power_of_two)
{
    ring_buffer_t rb;
    struct test_elem_t buf[3];   /* 3 elements -> not power of 2 */
    bool result = ring_buffer_init(&rb, buf, sizeof(buf), sizeof(struct test_elem_t));
    ASSERT_FALSE(result);
}

TEST(test_init_zero_elem_size)
{
    ring_buffer_t rb;
    struct test_elem_t buf[4];
    bool result = ring_buffer_init(&rb, buf, sizeof(buf), 0);
    ASSERT_FALSE(result);
}

TEST(test_init_success)
{
    ring_buffer_t rb;
    struct test_elem_t buf[4];
    bool result = ring_buffer_init(&rb, buf, sizeof(buf), sizeof(struct test_elem_t));
    ASSERT_TRUE(result);
    ASSERT_EQ(rb.in, 0u);
    ASSERT_EQ(rb.out, 0u);
    ASSERT_EQ(rb.mask, 3u);   /* 4 elements -> mask = 3 */
    ASSERT_EQ(rb.e_size, sizeof(struct test_elem_t));
}

/* -------------------------------------------------------------------------- */
/* Empty / Full tests                                                         */
/* -------------------------------------------------------------------------- */

TEST(test_empty_after_init)
{
    ring_buffer_t rb;
    struct test_elem_t buf[4];
    ring_buffer_init(&rb, buf, sizeof(buf), sizeof(struct test_elem_t));
    ASSERT_TRUE(ring_buffer_is_empty(&rb));
    ASSERT_FALSE(ring_buffer_is_full(&rb));
}

TEST(test_full_after_fill)
{
    ring_buffer_t rb;
    struct test_elem_t buf[4];
    ring_buffer_init(&rb, buf, sizeof(buf), sizeof(struct test_elem_t));

    struct test_elem_t e = {.num = 1, .cnt = 2};

    uint32_t w = ring_buffer_write(&rb, &e, 1);
    ASSERT_EQ(w, 1u);
    ASSERT_FALSE(ring_buffer_is_empty(&rb));
    ASSERT_FALSE(ring_buffer_is_full(&rb));

    ring_buffer_write(&rb, &e, 1);
    ring_buffer_write(&rb, &e, 1);
    ASSERT_FALSE(ring_buffer_is_full(&rb));

    ring_buffer_write(&rb, &e, 1);
    ASSERT_TRUE(ring_buffer_is_full(&rb));
}

/* -------------------------------------------------------------------------- */
/* Write / Read basic                                                         */
/* -------------------------------------------------------------------------- */

TEST(test_write_read_one_element)
{
    ring_buffer_t rb;
    struct test_elem_t buf[4] = {0};
    ring_buffer_init(&rb, buf, sizeof(buf), sizeof(struct test_elem_t));

    struct test_elem_t src = {.num = 42, .cnt = 7};
    uint32_t written = ring_buffer_write(&rb, &src, 1);
    ASSERT_EQ(written, 1u);
    ASSERT_EQ(buf[0].num, 42u);
    ASSERT_EQ(buf[0].cnt, 7u);

    struct test_elem_t dst = {0};
    uint32_t read = ring_buffer_read(&rb, &dst, 1);
    ASSERT_EQ(read, 1u);
    ASSERT_EQ(dst.num, 42u);
    ASSERT_EQ(dst.cnt, 7u);
    ASSERT_TRUE(ring_buffer_is_empty(&rb));
}

TEST(test_write_read_multiple_elements)
{
    ring_buffer_t rb;
    struct test_elem_t buf[4] = {0};
    ring_buffer_init(&rb, buf, sizeof(buf), sizeof(struct test_elem_t));

    /* Proper array for multi-element write */
    struct test_elem_t src[3] = {
        {.num = 1, .cnt = 10},
        {.num = 2, .cnt = 20},
        {.num = 3, .cnt = 30},
    };
    uint32_t written = ring_buffer_write(&rb, src, 3);
    ASSERT_EQ(written, 3u);

    struct test_elem_t dst[3] = {0};
    uint32_t read = ring_buffer_read(&rb, dst, 3);
    ASSERT_EQ(read, 3u);
    ASSERT_EQ(dst[0].num, 1u);
    ASSERT_EQ(dst[0].cnt, 10u);
    ASSERT_EQ(dst[1].num, 2u);
    ASSERT_EQ(dst[1].cnt, 20u);
    ASSERT_EQ(dst[2].num, 3u);
    ASSERT_EQ(dst[2].cnt, 30u);
}

/* -------------------------------------------------------------------------- */
/* Write when full / Read when empty                                          */
/* -------------------------------------------------------------------------- */

TEST(test_write_when_full_returns_zero)
{
    ring_buffer_t rb;
    struct test_elem_t buf[4] = {0};
    ring_buffer_init(&rb, buf, sizeof(buf), sizeof(struct test_elem_t));

    struct test_elem_t e = {.num = 99, .cnt = 99};
    ring_buffer_write(&rb, &e, 1);
    ring_buffer_write(&rb, &e, 1);
    ring_buffer_write(&rb, &e, 1);
    ring_buffer_write(&rb, &e, 1);
    ASSERT_TRUE(ring_buffer_is_full(&rb));

    uint32_t written = ring_buffer_write(&rb, &e, 1);
    ASSERT_EQ(written, 0u);
}

TEST(test_read_when_empty_returns_zero)
{
    ring_buffer_t rb;
    struct test_elem_t buf[4] = {0};
    ring_buffer_init(&rb, buf, sizeof(buf), sizeof(struct test_elem_t));

    struct test_elem_t dst = {0};
    uint32_t read = ring_buffer_read(&rb, &dst, 1);
    ASSERT_EQ(read, 0u);
}

TEST(test_read_null_buffer_returns_zero)
{
    ring_buffer_t rb;
    struct test_elem_t buf[4] = {0};
    ring_buffer_init(&rb, buf, sizeof(buf), sizeof(struct test_elem_t));

    struct test_elem_t e = {.num = 1, .cnt = 1};
    ring_buffer_write(&rb, &e, 1);

    uint32_t read = ring_buffer_read(&rb, NULL, 1);
    ASSERT_EQ(read, 0u);
    /* Buffer should still have data */
    ASSERT_FALSE(ring_buffer_is_empty(&rb));
}

/* -------------------------------------------------------------------------- */
/* Partial write (buffer has less free space than requested)                  */
/* -------------------------------------------------------------------------- */

TEST(test_write_partial_when_space_limited)
{
    ring_buffer_t rb;
    struct test_elem_t buf[4] = {0};
    ring_buffer_init(&rb, buf, sizeof(buf), sizeof(struct test_elem_t));

    /* Write 3 elements using an array */
    struct test_elem_t e[3] = {{.num = 1, .cnt = 1}, {.num = 2, .cnt = 2}, {.num = 3, .cnt = 3}};
    ring_buffer_write(&rb, e, 3);
    ASSERT_EQ(ring_buffer_free_len(&rb), 1u);

    /* Try to write 2 more, only 1 should succeed */
    struct test_elem_t src[2] = {{.num = 10, .cnt = 10}, {.num = 20, .cnt = 20}};
    uint32_t written = ring_buffer_write(&rb, src, 2);
    ASSERT_EQ(written, 1u);
    ASSERT_TRUE(ring_buffer_is_full(&rb));
}

/* -------------------------------------------------------------------------- */
/* Partial read (buffer has less data than requested)                         */
/* -------------------------------------------------------------------------- */

TEST(test_read_partial_when_data_limited)
{
    ring_buffer_t rb;
    struct test_elem_t buf[4] = {0};
    ring_buffer_init(&rb, buf, sizeof(buf), sizeof(struct test_elem_t));

    struct test_elem_t e[2] = {{.num = 5, .cnt = 6}, {.num = 7, .cnt = 8}};
    ring_buffer_write(&rb, e, 2);

    struct test_elem_t dst[10] = {0};
    uint32_t read = ring_buffer_read(&rb, dst, 10);
    ASSERT_EQ(read, 2u);
    ASSERT_TRUE(ring_buffer_is_empty(&rb));
}

/* -------------------------------------------------------------------------- */
/* Wrap-around tests                                                          */
/* -------------------------------------------------------------------------- */

TEST(test_write_wraparound)
{
    ring_buffer_t rb;
    struct test_elem_t buf[4] = {0};
    ring_buffer_init(&rb, buf, sizeof(buf), sizeof(struct test_elem_t));

    /* Fill the buffer with proper array */
    struct test_elem_t e1[4] = {
        {.num = 1, .cnt = 1},
        {.num = 2, .cnt = 2},
        {.num = 3, .cnt = 3},
        {.num = 4, .cnt = 4},
    };
    ring_buffer_write(&rb, e1, 4);
    ASSERT_TRUE(ring_buffer_is_full(&rb));

    /* Read 2, freeing slots at indices 0,1 */
    struct test_elem_t dst[2] = {0};
    ring_buffer_read(&rb, dst, 2);
    ASSERT_EQ(dst[0].num, 1u);
    ASSERT_EQ(dst[1].num, 2u);

    /* Now in=4, out=2. Write 2 more — wraps to indices 0,1 */
    struct test_elem_t e2[2] = {{.num = 99, .cnt = 88}, {.num = 77, .cnt = 66}};
    ring_buffer_write(&rb, e2, 2);

    /* Read remaining 2 original (indices 2,3), then the 2 wrapped */
    struct test_elem_t result[4] = {0};
    uint32_t r = ring_buffer_read(&rb, result, 4);
    ASSERT_EQ(r, 4u);
    ASSERT_EQ(result[0].num, 3u);   /* was at index 2 */
    ASSERT_EQ(result[1].num, 4u);   /* was at index 3 */
    ASSERT_EQ(result[2].num, 99u);  /* wrapped to index 0 */
    ASSERT_EQ(result[2].cnt, 88u);
    ASSERT_EQ(result[3].num, 77u);  /* wrapped to index 1 */
    ASSERT_EQ(result[3].cnt, 66u);
}

TEST(test_read_wraparound)
{
    ring_buffer_t rb;
    struct test_elem_t buf[4] = {0};
    ring_buffer_init(&rb, buf, sizeof(buf), sizeof(struct test_elem_t));

    /* Fill buffer */
    struct test_elem_t e[4] = {
        {.num = 10, .cnt = 1},
        {.num = 20, .cnt = 2},
        {.num = 30, .cnt = 3},
        {.num = 40, .cnt = 4},
    };
    ring_buffer_write(&rb, e, 4);

    /* Read 2, advance out past the wrap boundary later */
    struct test_elem_t tmp[2] = {0};
    ring_buffer_read(&rb, tmp, 2);
    ASSERT_EQ(tmp[0].num, 10u);
    ASSERT_EQ(tmp[1].num, 20u);

    /* Write 2 more — they go to indices 0,1 (wrap in) */
    struct test_elem_t e2[2] = {{.num = 50, .cnt = 5}, {.num = 60, .cnt = 6}};
    ring_buffer_write(&rb, e2, 2);

    /* Now logical data order: idx2=30, idx3=40, idx0=50, idx1=60.
     * out=2, in=6. Reading 4 should wrap the read. */
    struct test_elem_t result[4] = {0};
    uint32_t read = ring_buffer_read(&rb, result, 4);
    ASSERT_EQ(read, 4u);
    ASSERT_EQ(result[0].num, 30u);
    ASSERT_EQ(result[0].cnt, 3u);
    ASSERT_EQ(result[1].num, 40u);
    ASSERT_EQ(result[1].cnt, 4u);
    ASSERT_EQ(result[2].num, 50u); /* wrapped read */
    ASSERT_EQ(result[2].cnt, 5u);
    ASSERT_EQ(result[3].num, 60u);
    ASSERT_EQ(result[3].cnt, 6u);
}

/* -------------------------------------------------------------------------- */
/* Data length and free length                                                */
/* -------------------------------------------------------------------------- */

TEST(test_data_len_and_free_len)
{
    ring_buffer_t rb;
    struct test_elem_t buf[4] = {0};
    ring_buffer_init(&rb, buf, sizeof(buf), sizeof(struct test_elem_t));

    ASSERT_EQ(ring_buffer_data_len(&rb), 0u);
    ASSERT_EQ(ring_buffer_free_len(&rb), 4u);

    struct test_elem_t e = {.num = 1, .cnt = 1};
    ring_buffer_write(&rb, &e, 1);
    ASSERT_EQ(ring_buffer_data_len(&rb), 1u);
    ASSERT_EQ(ring_buffer_free_len(&rb), 3u);

    /* Write 2 more — use an array */
    struct test_elem_t e2[2] = {{.num = 2, .cnt = 2}, {.num = 3, .cnt = 3}};
    ring_buffer_write(&rb, e2, 2);
    ASSERT_EQ(ring_buffer_data_len(&rb), 3u);
    ASSERT_EQ(ring_buffer_free_len(&rb), 1u);

    struct test_elem_t dst = {0};
    ring_buffer_read(&rb, &dst, 1);
    ASSERT_EQ(ring_buffer_data_len(&rb), 2u);
    ASSERT_EQ(ring_buffer_free_len(&rb), 2u);

    ring_buffer_read(&rb, &dst, 2);
    ASSERT_EQ(ring_buffer_data_len(&rb), 0u);
    ASSERT_EQ(ring_buffer_free_len(&rb), 4u);
}

/* -------------------------------------------------------------------------- */
/* Single-byte element type                                                   */
/* -------------------------------------------------------------------------- */

TEST(test_byte_sized_elements)
{
    ring_buffer_t rb;
    uint8_t buf[8];
    ring_buffer_init(&rb, buf, sizeof(buf), sizeof(uint8_t));

    uint8_t src[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    uint32_t written = ring_buffer_write(&rb, src, 6);
    ASSERT_EQ(written, 6u);

    ASSERT_EQ(ring_buffer_data_len(&rb), 6u);
    ASSERT_EQ(ring_buffer_free_len(&rb), 2u);

    uint8_t dst[8] = {0};
    uint32_t read = ring_buffer_read(&rb, dst, 8);
    ASSERT_EQ(read, 6u);
    ASSERT_EQ(dst[0], 0xAAu);
    ASSERT_EQ(dst[1], 0xBBu);
    ASSERT_EQ(dst[2], 0xCCu);
    ASSERT_EQ(dst[3], 0xDDu);
    ASSERT_EQ(dst[4], 0xEEu);
    ASSERT_EQ(dst[5], 0xFFu);
}

/* -------------------------------------------------------------------------- */
/* Stress: fill and drain in cycles                                           */
/* -------------------------------------------------------------------------- */

TEST(test_fill_drain_cycles)
{
    ring_buffer_t rb;
    uint32_t buf[8];
    ring_buffer_init(&rb, buf, sizeof(buf), sizeof(uint32_t));

    for (uint32_t cycle = 0; cycle < 100; cycle++)
    {
        /* Fill */
        for (uint32_t i = 0; i < 8; i++)
        {
            uint32_t written = ring_buffer_write(&rb, &i, 1);
            ASSERT_EQ(written, 1u);
        }
        ASSERT_TRUE(ring_buffer_is_full(&rb));

        /* Drain */
        for (uint32_t i = 0; i < 8; i++)
        {
            uint32_t val = 0;
            uint32_t read = ring_buffer_read(&rb, &val, 1);
            ASSERT_EQ(read, 1u);
            ASSERT_EQ(val, i);
        }
        ASSERT_TRUE(ring_buffer_is_empty(&rb));
    }
}

/* -------------------------------------------------------------------------- */
/* uint64_t element type                                                      */
/* -------------------------------------------------------------------------- */

TEST(test_large_element_type)
{
    ring_buffer_t rb;
    uint64_t buf[4];
    ring_buffer_init(&rb, buf, sizeof(buf), sizeof(uint64_t));

    uint64_t src = 0xDEADBEEFCAFEBABEULL;
    uint32_t written = ring_buffer_write(&rb, &src, 1);
    ASSERT_EQ(written, 1u);
    ASSERT_EQ(buf[0], 0xDEADBEEFCAFEBABEULL);

    uint64_t dst = 0;
    uint32_t read = ring_buffer_read(&rb, &dst, 1);
    ASSERT_EQ(read, 1u);
    ASSERT_EQ(dst, 0xDEADBEEFCAFEBABEULL);
}

/* -------------------------------------------------------------------------- */
/* Test with struct elements in original main.c style                         */
/* -------------------------------------------------------------------------- */

TEST(test_struct_elements_in_sequence)
{
    /* Same struct as main.c */
    struct buff_t
    {
        uint16_t num;
        uint8_t  cnt;
    };

    struct buff_t buf[8];
    ring_buffer_t rb;
    bool ok = ring_buffer_init(&rb, buf, sizeof(buf), sizeof(struct buff_t));
    ASSERT_TRUE(ok);

    /* Write one element */
    struct buff_t w1 = {.cnt = 5, .num = 6};
    uint32_t w = ring_buffer_write(&rb, &w1, 1);
    ASSERT_EQ(w, 1u);

    /* Read it back */
    struct buff_t r1 = {0};
    uint32_t r = ring_buffer_read(&rb, &r1, 1);
    ASSERT_EQ(r, 1u);
    ASSERT_EQ(r1.num, 6u);
    ASSERT_EQ(r1.cnt, 5u);
}

/* -------------------------------------------------------------------------- */
/* Run all tests                                                              */
/* -------------------------------------------------------------------------- */

int main(void)
{
    printf("\n========================================\n");
    printf("  Ring Buffer Unit Tests\n");
    printf("========================================\n\n");

    printf("[1] Init\n");
    RUN_TEST(test_init_null_ring_buf);
    RUN_TEST(test_init_null_buffer);
    RUN_TEST(test_init_non_power_of_two);
    RUN_TEST(test_init_zero_elem_size);
    RUN_TEST(test_init_success);

    printf("\n[2] Empty / Full\n");
    RUN_TEST(test_empty_after_init);
    RUN_TEST(test_full_after_fill);

    printf("\n[3] Write / Read\n");
    RUN_TEST(test_write_read_one_element);
    RUN_TEST(test_write_read_multiple_elements);
    RUN_TEST(test_write_when_full_returns_zero);
    RUN_TEST(test_read_when_empty_returns_zero);
    RUN_TEST(test_read_null_buffer_returns_zero);
    RUN_TEST(test_write_partial_when_space_limited);
    RUN_TEST(test_read_partial_when_data_limited);

    printf("\n[4] Wrap-around\n");
    RUN_TEST(test_write_wraparound);
    RUN_TEST(test_read_wraparound);

    printf("\n[5] Length queries\n");
    RUN_TEST(test_data_len_and_free_len);

    printf("\n[6] Element types\n");
    RUN_TEST(test_byte_sized_elements);
    RUN_TEST(test_large_element_type);
    RUN_TEST(test_struct_elements_in_sequence);

    printf("\n[7] Stress\n");
    RUN_TEST(test_fill_drain_cycles);

    printf("\n========================================\n");
    printf("  Results: %d / %d passed", tests_passed, tests_run);
    if (tests_failed > 0)
        printf("  (%d FAILED)", tests_failed);
    printf("\n========================================\n");

    return tests_failed > 0 ? 1 : 0;
}
