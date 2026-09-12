#ifndef __KEY_H
#define __KEY_H

#include <stdint.h>

/* 按键状态枚举 */
typedef enum {
    Key_State_IDEL = 0,           // 空闲
    Key_State_DEBOUNCE,           // 消抖
    Key_State_PRESS_WAIT_UP,      // 按下等待松开（短按/长按判定）
    Key_State_LONG_PRESS,         // 长按保持中
    Key_State_DOUBLE_CLICK_WAIT,  // 双击等待（短按松开后等待第二击）
    Key_State_DOUBLE_CLICK_WAIT_UP // 双击第二击按下后等待松开
} Key_state_t;

/* 按键事件枚举 */
typedef enum {
    KEY_EVENT_NONE = 0,           // 无事件
    KEY_EVENT_PRESS_DOWN,         // 短按按下（消抖完成）
    KEY_EVENT_PRESS_UP,           // 短按松开（确认单击后）
    KEY_EVENT_LONG_PRESS_START,   // 长按开始（达到长按阈值）
    KEY_EVENT_LONG_PRESS_HOLD,    // 长按保持（每次扫描时持续触发）
    KEY_EVENT_LONG_PRESS_UP,      // 长按松开
    KEY_EVENT_DOUBLE_CLICK        // 双击事件（第二击消抖完成）
} Key_event_t;

/* 按键对象结构体 */
typedef struct {
    uint8_t (*p)(void);           // 获取按键电平：按下返回0，松开返回1
    Key_state_t state;            // 当前状态
    uint8_t DEBOUNCE_NUM;         // 消抖计数阈值（调用次数）
    uint16_t press_cnt;           // 消抖/通用计数器

    uint16_t LONG_PRESS_NUM;      // 长按触发计数阈值
    uint16_t DOUBLE_CLICK_NUM;    // 双击间隔计数阈值
    uint16_t hold_cnt;            // 按下持续时间计数
    uint16_t dc_wait_cnt;         // 双击等待计数
    uint8_t  double_click_flag;   // 双击标志，1表示当前消抖用于第二击
} Key_obj_t;

void Key_Init(void);
uint8_t Key_GetNum(void);
Key_event_t key_Scan(Key_obj_t *key);

#endif
