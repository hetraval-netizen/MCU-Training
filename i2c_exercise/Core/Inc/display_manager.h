#ifndef INC_DISPLAY_MANAGER_H_
#define INC_DISPLAY_MANAGER_H_

#include "main.h"
#include "u8g2.h"

typedef enum {
    FAST_BLINK,
    SLOW_BLINK,
    BREATHING,
    STATIC_LED,
    MAIN_MENU_TOTAL_COUNT
} main_menu_t;

typedef enum {
    CONTINEOUS,
    FIX_COUNT,
    BLINK_MENU_TOTAL_COUNT
} blink_menu_t;

typedef enum {
    MAIN_MENU,
    BLINK_MENU,
    BREATHING,
    COUNT,
    STATIC_BRIGHTNESS,
    TOTAL_MENU_COUNT
} display_state_t;

typedef enum {
    BACK,
    UP,
    DOWN,
    ENTER
    TOTAL_BTN_COUNT
} button_t;

uint8_t u8g2_gpio_and_delay_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
uint8_t u8g2_byte_hw_i2c_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
void display_manager_init(void);

#endif /* INC_DISPLAY_MANAGER_H_ */
