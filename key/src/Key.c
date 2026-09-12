#include "Key.h"

/**
 * @brief  按键扫描函数，需周期性调用（调用间隔固定）
 * @param  key: 按键对象指针
 * @return 当前触发的事件
 */
Key_event_t key_Scan(Key_obj_t *key) {
    Key_event_t ret = KEY_EVENT_NONE;
    uint8_t key_num = key->p();   // 获取当前电平

    switch (key->state) {
        /* 空闲状态：检测按下 */
        case Key_State_IDEL:
            if (key_num == 0) {
                key->state = Key_State_DEBOUNCE;
                key->press_cnt = 0;
                key->double_click_flag = 0; // 清除双击标志
            }
            break;

        /* 消抖状态 */
        case Key_State_DEBOUNCE:
            if (key_num == 0) {
                key->press_cnt++;
                if (key->press_cnt >= key->DEBOUNCE_NUM) {
                    // 消抖完成，确认按下
                    if (key->double_click_flag) {
                        // 是双击的第二击
                        ret = KEY_EVENT_DOUBLE_CLICK;
                        key->state = Key_State_DOUBLE_CLICK_WAIT_UP;
                    } else {
                        // 第一击或普通短按
                        ret = KEY_EVENT_PRESS_DOWN;
                        key->state = Key_State_PRESS_WAIT_UP;
                        key->hold_cnt = 0;  // 准备长按计时
                    }
                    key->press_cnt = 0;
                }
            } else {
                // 消抖过程中松开，返回空闲（双击标志也复位）
                key->state = Key_State_IDEL;
                key->press_cnt = 0;
                key->double_click_flag = 0;
            }
            break;

        /* 按下等待松开：兼顾短按长按判定 */
        case Key_State_PRESS_WAIT_UP:
            if (key_num == 0) {
                // 按键仍按下，长按计时
                key->hold_cnt++;
                if (key->hold_cnt >= key->LONG_PRESS_NUM) {
                    key->state = Key_State_LONG_PRESS;
                    ret = KEY_EVENT_LONG_PRESS_START;
                }
            } else {
                // 按键松开，未达到长按阈值 -> 进入双击等待
                key->state = Key_State_DOUBLE_CLICK_WAIT;
                key->dc_wait_cnt = 0;
                // 此时不产生 PRESS_UP，等待双击判定
            }
            break;

        /* 长按保持状态 */
        case Key_State_LONG_PRESS:
            if (key_num == 0) {
                ret = KEY_EVENT_LONG_PRESS_HOLD;  // 每次调用都返回保持事件
            } else {
                key->state = Key_State_IDEL;
                ret = KEY_EVENT_LONG_PRESS_UP;
            }
            break;

        /* 双击等待状态：等待第二击或超时 */
        case Key_State_DOUBLE_CLICK_WAIT:
            if (key_num == 0) {
                // 检测到第二击按下，进入消抖并标记双击
                key->double_click_flag = 1;
                key->state = Key_State_DEBOUNCE;
                key->press_cnt = 0;
            } else {
                key->dc_wait_cnt++;
                if (key->dc_wait_cnt >= key->DOUBLE_CLICK_NUM) {
                    // 超时，确认为单击，补发松开事件
                    key->state = Key_State_IDEL;
                    ret = KEY_EVENT_PRESS_UP;
                }
            }
            break;

        /* 双击按下后等待松开 */
        case Key_State_DOUBLE_CLICK_WAIT_UP:
            if (key_num == 1) {
                key->state = Key_State_IDEL;
                // 双击完成，不产生额外事件
            }
            break;

        default:
            key->state = Key_State_IDEL;
            break;
    }

    return ret;
}
