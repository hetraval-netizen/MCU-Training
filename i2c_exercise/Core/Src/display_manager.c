#include "display_manager.h"
#include "i2c_peripheral.h"
#include "led_control.h"
#include "u8g2.h"

#include <stdio.h>
#include <string.h>

extern u8g2_t u8g2; // Extern variable for using the u8g2 library

/* The names for all Main menu options */
char *main_menu[] = {
    "Fast Blink",
    "Slow Blink",
    "Breathing",
    "Static LED",
};

/* The names for all Blink menu options */
char *blink_menu[] = {
    "Contineous",
    "Fix count"
};

/* Static global variables to manage the menu switches and current settings */
static unsigned int current_main_menu_setting = 0;
static unsigned int current_blink_menu_setting = 0; 
static unsigned int current_blink_count = 0;
static unsigned int current_led_brightness = 0; 

display_state_t current_state = MAIN_MENU;

/* 
 * @brief API to Clear the oled display 
 */
void clear_display(){
    u8g2_ClearDisplay(&u8g2);
    u8g2_ClearBuffer(&u8g2);
}

/*
 * @brief delay API callback config for the u8g2 library
 */
uint8_t u8g2_gpio_and_delay_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    switch(msg) {
        case U8X8_MSG_DELAY_MILLI:
            HAL_Delay(arg_int); 
            break;
        default:
            return 0;
    }
    return 1;
}

/*
 * @brief Byte API callback config for the u8g2 library for various byte related ops
 */
uint8_t u8g2_byte_hw_i2c_stm32(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    static uint8_t buffer[32]; /* U8g2 never sends more than 32 bytes per transfer */
    static uint8_t buf_index;
    
    switch(msg) {
        case U8X8_MSG_BYTE_INIT:
            return 1; /* i2c_hardware_init() is already called in main.c */
            
        case U8X8_MSG_BYTE_START_TRANSFER:
            buf_index = 0;
            break;
            
        case U8X8_MSG_BYTE_SEND: {
            uint8_t *src = (uint8_t *)arg_ptr;
            for(uint8_t i = 0; i < arg_int; i++) {
                if (buf_index < 32) {
                    buffer[buf_index++] = src[i];
                }
            }
            break;
        }
        
        case U8X8_MSG_BYTE_END_TRANSFER: {
            /* Pass the exact accumulated buffer to your hardware line */
            uint8_t target_address = 0x78;
            i2c_master_transmit(target_address, buffer, buf_index);
            break;
        }
        default:
            return 0;
    }
    return 1;
}

/*
 * @brief The API to print the desired menu type on the oled display
 */
void oled_print_menu (display_state_t menu_type) {
    printf("Current Menu type: %d\n", menu_type);

    unsigned int total_menu_options = 0;
    char **current_menu_options = NULL;
    unsigned int current_setting = 0;
    switch (menu_type) {
        case MAIN_MENU:
            total_menu_options = MAIN_MENU_TOTAL_COUNT;
            current_menu_options = main_menu;
            current_setting = current_main_menu_setting;
            break;

        case BLINK_MENU:
            total_menu_options = BLINK_MENU_TOTAL_COUNT;
            current_menu_options = blink_menu;
            current_setting = current_blink_menu_setting;
            break;

        default:
            printf("Invalid menu type\n");
            return;
    }

    clear_display();
    unsigned int x_axis = OLED_DISPLAY_STR_INIT_X_AXIS;
    unsigned int y_axis = OLED_DISPLAY_STR_INIT_Y_AXIS;
    unsigned int current_box_pos = OLED_DISPLAY_INIT_BOX_POS;

    for (int menu_ptr = 0; menu_ptr < total_menu_options; menu_ptr++) {
        if (menu_ptr == current_setting) {
            u8g2_SetDrawColor(&u8g2, 1);
            u8g2_DrawBox(&u8g2, 0, current_box_pos, OLED_DISPLAY_PIXEL_WIDTH, OLED_DISPLAY_BOX_HEIGHT);
            u8g2_SetDrawColor(&u8g2, 0);
            u8g2_DrawStr(&u8g2, x_axis, y_axis, current_menu_options[menu_ptr]);
            u8g2_SetDrawColor(&u8g2, 1);
        }
        else {
            u8g2_DrawStr(&u8g2, x_axis, y_axis, current_menu_options[menu_ptr]);
        }
        u8g2_SendBuffer(&u8g2);
        y_axis += OLED_DISPLAY_STR_STEP_SIZE;
        current_box_pos += OLED_DISPLAY_STR_STEP_SIZE;
    }
}

/*
 * @brief The API to print the led brightness bar on the oled display
 */
void oled_print_static_led_brightness (int brightness_level) {
    clear_display();

    float brightness_level_oled_display = brightness_level * OLED_DISPLAY_PERCENTAGE_TO_PIXEL_FOR_BRIGHTNESS_BAR;
    u8g2_DrawStr(&u8g2, OLED_DISPLAY_STR_MIDDLE_INIT_X_AXIS, OLED_DISPLAY_STR_INIT_Y_AXIS, "Brightness level <>");
    u8g2_DrawBox(&u8g2, 0, OLED_DISPLAY_BOX_POS_BRIGHTNESS_BAR, brightness_level_oled_display, OLED_DISPLAY_BOX_HEIGHT);
    u8g2_SendBuffer(&u8g2);
}

/*
 * @brief The API to print the desired counts for the display
 */
void oled_print_count_number (int count) {
    clear_display();

    char temp_str[32];
    snprintf(temp_str, sizeof(temp_str), "Total blinks: <%d>", count);
    
    u8g2_DrawStr(&u8g2, OLED_DISPLAY_STR_MIDDLE_INIT_X_AXIS, OLED_DISPLAY_STR_INIT_Y_AXIS, temp_str);
    u8g2_SendBuffer(&u8g2);
}

/*
 * @brief The event handler for the back button pressed
 */
void back_event_handler() {
    printf ("back event handler state: %d\n", current_state);
    if (current_state == MAIN_MENU)
        return;

    switch (current_state) {
        case BLINK_MENU:
            current_state = MAIN_MENU;
            oled_print_menu(current_state);
            break;

        case COUNT:
            current_state = BLINK_MENU;
            oled_print_menu(current_state);
            break;

        case STATIC_BRIGHTNESS:
            current_state = MAIN_MENU;
            oled_print_menu(current_state);
            break;

        default:
            break;
    }
}

/*
 * @brief The event handler for the up button pressed
 */
void up_event_handler() {
    printf ("up event handler state: %d\n", current_state);
    switch (current_state) {
        case MAIN_MENU:
            if (current_main_menu_setting == FAST_BLINK)
                return;
            
            current_main_menu_setting--;
            oled_print_menu(current_state);
            break;

        case BLINK_MENU:
            if (current_blink_menu_setting == CONTINEOUS)
                return;

            current_blink_menu_setting--;
            oled_print_menu(current_state);
            break;
        
        case COUNT:
            current_blink_count++;
            oled_print_count_number(current_blink_count);
            break;
            
        case STATIC_BRIGHTNESS:
            if (current_led_brightness == MAX_BRIGHTNESS_LIMIT)
                return;

            current_led_brightness += OLED_DISPLAY_BRIGHTNESS_STEP;
            oled_print_static_led_brightness(current_led_brightness);
            led_set_mode(LED_MODE_FIXED_BRIGHTNESS, current_led_brightness);
            break;

        default:
            break;
    }
}

/*
 * @brief The event handler for the down button pressed
 */
void down_event_handler() {
    printf ("down event handler state: %d\n", current_state);
    switch (current_state) {
        case MAIN_MENU:
            if (current_main_menu_setting == STATIC_LED)
                return;
            
            current_main_menu_setting++;
            oled_print_menu(current_state);
            break;

        case BLINK_MENU:
            if (current_blink_menu_setting == FIX_COUNT)
                return;

            current_blink_menu_setting++;
            oled_print_menu(current_state);
            break;
        
        case COUNT:
            if (current_blink_count <= MIN_COUNT_LIMIT)
                return;

            current_blink_count--;
            oled_print_count_number(current_blink_count);
            break;
            
        case STATIC_BRIGHTNESS:
            if (current_led_brightness == MIN_BRIGHTNESS_LIMIT)
                return;

            current_led_brightness -= OLED_DISPLAY_BRIGHTNESS_STEP;
            oled_print_static_led_brightness(current_led_brightness);
            led_set_mode(LED_MODE_FIXED_BRIGHTNESS, current_led_brightness);
            break;

        default:
            break;
    }
}

/*
 * @brief The event handler for the Enter button pressed
 */
void enter_event_handler() {
    printf ("Enter event handler state: %d\n", current_state);
    switch (current_state) {
        case MAIN_MENU:
            switch (current_main_menu_setting) {
                case FAST_BLINK:
                    current_state = BLINK_MENU;
                    oled_print_menu(current_state);
                    break;

                case SLOW_BLINK:
                    current_state = BLINK_MENU;
                    oled_print_menu(current_state);
                    break;

                case BREATHING:
                    current_state = MAIN_MENU;
                    led_set_mode(LED_MODE_BREATHING, 0);
                    break;

                case STATIC_LED:
                    current_state = STATIC_BRIGHTNESS;
                    oled_print_static_led_brightness(current_led_brightness);
                    led_set_mode(LED_MODE_FIXED_BRIGHTNESS, current_led_brightness);
                    break;
                
                default:
                    break;
            }
            break;
        
        case BLINK_MENU:
            switch (current_blink_menu_setting) {
                case CONTINEOUS:
                    if (current_main_menu_setting == FAST_BLINK)
                        led_set_mode(LED_MODE_FAST_BLINK, 0);
                    else
                        led_set_mode(LED_MODE_SLOW_BLINK, 0);
                    break;

                case FIX_COUNT:
                    current_state = COUNT;
                    current_blink_count = 1;
                    oled_print_count_number(current_blink_count);
                    break;
            }
            break;

        case COUNT:
            if (current_main_menu_setting == FAST_BLINK)
                led_set_mode(LED_MODE_COUNT_FAST, current_blink_count);
            else
                led_set_mode(LED_MODE_COUNT_SLOW, current_blink_count);
            break;

        default:
            break;
    }
}

/*
 * @brief The initialization code for the ssd1306 for using the u8g2 library
 */
void display_manager_init(void) {
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0, u8g2_byte_hw_i2c_stm32, u8g2_gpio_and_delay_stm32);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    clear_display();
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
    oled_print_menu(current_state);
}
