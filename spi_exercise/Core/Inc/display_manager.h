#ifndef INC_DISPLAY_MANAGER_H_
#define INC_DISPLAY_MANAGER_H_

#include "main.h"
#include "u8g2.h"

#include <stdbool.h>

/* Enum for the main menu options */
typedef enum {
    FAST_BLINK,
    SLOW_BLINK,
    BREATHING,
    STATIC_LED,
    SENSOR_DATA,
    MAIN_MENU_TOTAL_COUNT
} main_menu_t;

/* Enum for the blink menu options */
typedef enum {
    CONTINEOUS,
    FIX_COUNT,
    BLINK_MENU_TOTAL_COUNT
} blink_menu_t;

/* Enum for the display states handling */
typedef enum {
    MAIN_MENU,
    BLINK_MENU,
    COUNT,
    BREATHE_MENU,
    STATIC_BRIGHTNESS,
    SENSOR_MENU,
    TOTAL_MENU_COUNT
} display_state_t;

typedef struct {
    display_state_t state;
    unsigned int main_menu_option;
    unsigned int blink_menu_option;
    unsigned int blink_count;
    unsigned int led_brightness;
}menu_settings_t;

typedef struct {
    bool is_breathing;
    volatile int animation_direction;
    volatile unsigned int current_animation_stage;
}breathing_settings_t;

#define OLED_BREATHING_DELAY 249
#define OLED_SENSOR_DATA_DELAY 3000
#define OLED_BREATHING_BAR_MIN 0
#define OLED_BREATHING_BAR_MAX 105
#define OLED_BREATHING_BAR_INIT 1
#define OLED_BREATHING_BAR_STEP 1
#define OLED_DISPLAY_BOX_HEIGHT 14
#define OLED_DISPLAY_INIT_BOX_POS 24
#define OLED_DISPLAY_PIXEL_WIDTH 128
#define OLED_DISPLAY_STR_INIT_X_AXIS 4
#define OLED_DISPLAY_STR_INIT_Y_AXIS 34
#define OLED_DISPLAY_STR_STEP_SIZE 13 
#define OLED_DISPLAY_BRIGHTNESS_STEP 5
#define OLED_DISPLAY_STR_MIDDLE_INIT_X_AXIS 10
#define OLED_DISPLAY_BOX_POS_BRIGHTNESS_BAR 38
#define OLED_DISPLAY_PERCENTAGE_TO_PIXEL_FOR_BRIGHTNESS_BAR 1.24

/* Definitions for various files usage */
void back_event_handler();
void up_event_handler();
void down_event_handler();
void enter_event_handler();
uint8_t u8g2_gpio_and_delay_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
uint8_t u8g2_byte_hw_i2c_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
void display_manager_init(void);
void process_oled_events(void);

#endif /* INC_DISPLAY_MANAGER_H_ */
