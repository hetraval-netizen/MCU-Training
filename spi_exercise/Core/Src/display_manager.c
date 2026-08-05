#include "display_manager.h"
#include "i2c_peripheral.h"
#include "led_control.h"
#include "u8g2.h"

#include <stdio.h>
#include <string.h>

extern u8g2_t u8g2; // Extern variable for using the u8g2 library

/* The names for all Main menu options */
const char *main_menu_labels[] = {
    "Fast Blink",
    "Slow Blink",
    "Breathing",
    "Static LED",
    "Sensor Data",
};

/* The names for all Blink menu options */
const char *blink_menu_labels[] = {
    "Contineous",
    "Fix count"
};

static menu_settings_t current_settings = (menu_settings_t) { 0 };
volatile uint8_t oled_needs_refresh = 0;
volatile unsigned int current_animation_stage = 0;
volatile int animation_direction = 1;
bool is_breathing = false;
static unsigned int menu_start_ptr = FAST_BLINK;
static unsigned int menu_end_ptr = BREATHING;

/**
 * @brief  Helper to reset the TIM3 register configuration and stop the animation.
 */
static void reset_tim3_state(void) {
    TIM3->CR1 &= ~TIM_CR1_CEN; // Disable Timer 3
    is_breathing = false;
}

/**
 * @brief  Helper to reset the TIM3 register configuration and stop the animation.
 */
static void set_tim3_state(void) {
    TIM3->ARR = OLED_BREATHING_DELAY;
    TIM3->DIER |= TIM_DIER_UIE;
    TIM3->CR1 |= TIM_CR1_CEN;
}

/**
 * @brief This API will be initalising the timer 3 
 */
void oled_tim3_init(void) {

    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM3EN; // Enable the clock for Timer 3
    TIM3->CR1 = 0;
    TIM3->PSC = PRESCALE_CLOCK; // Gear down to 10,000 Hz (10 ticks per millisecond)

    // Set the lower priority so there is no conflict with the button and led intrupts
    NVIC_SetPriority(TIM3_IRQn, 5);
    NVIC_EnableIRQ(TIM3_IRQn);
}

/**
 * @brief  Hardware Interrupt Service Routine for TIM3
 */
void TIM3_IRQHandler(void) {
    // Check if the Update Interrupt Flag (UIF) is set
    if (TIM3->SR & TIM_SR_UIF) {
        TIM3->SR &= ~TIM_SR_UIF;

        if (animation_direction == 1) {
            current_animation_stage += OLED_BREATHING_BAR_STEP;
            if (current_animation_stage >= OLED_BREATHING_BAR_MAX) {
                animation_direction = ANIM_DIRECTION_NEGATIVE;
            }
        } else {
            current_animation_stage -= OLED_BREATHING_BAR_STEP;
            if (current_animation_stage == OLED_BREATHING_BAR_MIN) {
                animation_direction = ANIM_DIRECTION_POSITIVE;
            }
        }
        oled_needs_refresh = 1;
    }
}

/* 
 * @brief API to Clear the oled display 
 */
void clear_display(){
    u8g2_ClearDisplay(&u8g2);
    u8g2_ClearBuffer(&u8g2);
}

/**
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

/**
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

/**
 * @brief This API will print the headers for each menu
 */
static void oled_print_header(char *title) {
    // Set bold font for the title
    u8g2_SetFont(&u8g2, u8g2_font_7x14B_tr);
    u8g2_DrawStr(&u8g2, 35, 12, title);

    u8g2_DrawHLine(&u8g2, 0, 18, 128);

    u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
}

/*
 * @brief The API to print the led brightness bar on the oled display.
 *        This function is non-blocking and relies on TIM3 interrupts.
 */
void oled_print_breathing (void) {
    set_tim3_state();
    if (is_breathing && oled_needs_refresh) {
        oled_needs_refresh = 0;
        float brightness_level_oled_display = current_animation_stage * OLED_DISPLAY_PERCENTAGE_TO_PIXEL_FOR_BRIGHTNESS_BAR;
        u8g2_ClearBuffer(&u8g2);
        oled_print_header("Breathing");
        u8g2_DrawBox(&u8g2, 0, OLED_DISPLAY_BOX_POS_BRIGHTNESS_BAR, brightness_level_oled_display, OLED_DISPLAY_BOX_HEIGHT);
        u8g2_SendBuffer(&u8g2);
    }
}

/*
 * @brief The API to print the led brightness bar on the oled display
 */
void oled_print_static_led_brightness (int brightness_level) {
    clear_display();
    oled_print_header("Static LED");
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

    oled_print_header("Fix Count");
    char temp_str[32];
    snprintf(temp_str, sizeof(temp_str), "Total blinks: <%d>", count);
    
    u8g2_DrawStr(&u8g2, OLED_DISPLAY_STR_MIDDLE_INIT_X_AXIS, OLED_DISPLAY_STR_INIT_Y_AXIS, temp_str);
    u8g2_SendBuffer(&u8g2);
}

static void oled_print_main_menu(void) {
    clear_display();

    oled_print_header("Main Menu");

    unsigned int x_axis = OLED_DISPLAY_STR_INIT_X_AXIS;
    unsigned int y_axis = OLED_DISPLAY_STR_INIT_Y_AXIS;
    unsigned int current_box_pos = OLED_DISPLAY_INIT_BOX_POS;

    for (int menu_ptr = menu_start_ptr; menu_ptr < menu_end_ptr + 1; menu_ptr++){
        if (menu_ptr == current_settings.main_menu_option){
            u8g2_SetDrawColor(&u8g2, 1);
            u8g2_DrawBox(&u8g2, 0, current_box_pos, OLED_DISPLAY_PIXEL_WIDTH, OLED_DISPLAY_BOX_HEIGHT);
            u8g2_SetDrawColor(&u8g2, 0);
            u8g2_DrawStr(&u8g2, x_axis, y_axis, main_menu_labels[menu_ptr]);
            u8g2_SetDrawColor(&u8g2, 1);
        }
        else {
            u8g2_DrawStr(&u8g2, x_axis, y_axis, main_menu_labels[menu_ptr]);
        }

        u8g2_SendBuffer(&u8g2);
        y_axis += OLED_DISPLAY_STR_STEP_SIZE;
        current_box_pos += OLED_DISPLAY_STR_STEP_SIZE;
    }
}

static void oled_print_blink_menu(void) {
    clear_display();

    oled_print_header("Blink Menu");

    unsigned int x_axis = OLED_DISPLAY_STR_INIT_X_AXIS;
    unsigned int y_axis = OLED_DISPLAY_STR_INIT_Y_AXIS;
    unsigned int current_box_pos = OLED_DISPLAY_INIT_BOX_POS;

    for (int menu_ptr = 0; menu_ptr < BLINK_MENU_TOTAL_COUNT; menu_ptr++){
        if (menu_ptr == current_settings.blink_menu_option){
            u8g2_SetDrawColor(&u8g2, 1);
            u8g2_DrawBox(&u8g2, 0, current_box_pos, OLED_DISPLAY_PIXEL_WIDTH, OLED_DISPLAY_BOX_HEIGHT);
            u8g2_SetDrawColor(&u8g2, 0);
            u8g2_DrawStr(&u8g2, x_axis, y_axis, blink_menu_labels[menu_ptr]);
            u8g2_SetDrawColor(&u8g2, 1);
        }
        else {
            u8g2_DrawStr(&u8g2, x_axis, y_axis, blink_menu_labels[menu_ptr]);
        }

        u8g2_SendBuffer(&u8g2);
        y_axis += OLED_DISPLAY_STR_STEP_SIZE;
        current_box_pos += OLED_DISPLAY_STR_STEP_SIZE;
    }
}

/*
 * @brief The API to print the desired menu type on the oled display
 */
void oled_print_menu (display_state_t menu_type) {
    printf("Current Menu type: %d\n", menu_type);

    switch (menu_type) {
        case MAIN_MENU:
            oled_print_main_menu();
            break;

        case BLINK_MENU:
            oled_print_blink_menu();
            break;

        default:
            printf("Invalid menu type\n");
            return;
    }
}

static void main_menu_handler (void) {
    switch (current_settings.main_menu_option) {
        case FAST_BLINK:
            current_settings.state = BLINK_MENU;
            oled_print_menu(current_settings.state);
            break;

        case SLOW_BLINK:
            current_settings.state = BLINK_MENU;
            oled_print_menu(current_settings.state);
            break;

            case BREATHING:
            current_settings.state = BREATHE_MENU; // Ensure state tracks cleanly
            current_animation_stage = OLED_BREATHING_BAR_INIT;
            is_breathing = true;
            oled_print_breathing();
            led_set_mode(LED_MODE_BREATHING, 0); // Complete snippet implementation
            break;

        case STATIC_LED:
            current_settings.state = STATIC_BRIGHTNESS;
            oled_print_static_led_brightness(current_settings.led_brightness);
            led_set_mode(LED_MODE_FIXED_BRIGHTNESS, current_settings.led_brightness);
            break;
        
        default:
            printf("Unexpected event!!\n");
            break;
    }
}

static void blink_menu_handler (void) {
    switch (current_settings.blink_menu_option) {
        case CONTINEOUS:
            if (current_settings.main_menu_option == FAST_BLINK)
                led_set_mode(LED_MODE_FAST_BLINK, 0);
            else
                led_set_mode(LED_MODE_SLOW_BLINK, 0);
            break;

        case FIX_COUNT:
            current_settings.state = COUNT;
            current_settings.blink_count = MIN_COUNT_LIMIT;
            oled_print_count_number(current_settings.blink_count);
            break;

        default:
            printf("Unexpected event!!\n");
            break;
    }
}

/*
 * @brief The event handler for the back button pressed
 */
void back_event_handler() {
    printf ("back event handler state: %d\n", current_settings.state);
    if (current_settings.state == MAIN_MENU)
        return;

    switch (current_settings.state) {
        case BLINK_MENU:
            current_settings.state = MAIN_MENU;
            oled_print_menu(current_settings.state);
            break;

        case COUNT:
            current_settings.state = BLINK_MENU;
            oled_print_menu(current_settings.state);
            break;

        case BREATHE_MENU:
            current_settings.state = MAIN_MENU;
            is_breathing = false;
            reset_tim3_state();
            oled_print_menu(current_settings.state);
            break;

        case STATIC_BRIGHTNESS:
            current_settings.state = MAIN_MENU;
            oled_print_menu(current_settings.state);
            break;

        default:
            break;
    }
    led_reset_timer_state();
}

/*
 * @brief The event handler for the up button pressed
 */
void up_event_handler() {
    printf ("up event handler state: %d\n", current_settings.state);
    switch (current_settings.state) {
        case MAIN_MENU:
            if (current_settings.main_menu_option == FAST_BLINK)
                return;
            
            if (current_settings.main_menu_option == menu_start_ptr){
                menu_start_ptr--;
                menu_end_ptr--;
            }

            current_settings.main_menu_option--;
            oled_print_menu(current_settings.state);
            break;

        case BLINK_MENU:
            if (current_settings.blink_menu_option == CONTINEOUS)
                return;

            current_settings.blink_menu_option--;
            oled_print_menu(current_settings.state);
            break;
        
        case COUNT:
            current_settings.blink_count++;
            oled_print_count_number(current_settings.blink_count);
            break;
            
        case STATIC_BRIGHTNESS:
            if (current_settings.led_brightness == MAX_BRIGHTNESS_LIMIT)
                return;

            current_settings.led_brightness += OLED_DISPLAY_BRIGHTNESS_STEP;
            oled_print_static_led_brightness(current_settings.led_brightness);
            led_set_mode(LED_MODE_FIXED_BRIGHTNESS, current_settings.led_brightness);
            break;

        default:
            printf("Unexpected event!!\n");
            break;
    }
}

/*
 * @brief The event handler for the down button pressed
 */
void down_event_handler() {
    printf ("down event handler state: %d\n", current_settings.state);
    switch (current_settings.state) {
        case MAIN_MENU:
            if (current_settings.main_menu_option == SENSOR_DATA)
                return;
            
            if (current_settings.main_menu_option == menu_end_ptr){
                menu_start_ptr++;
                menu_end_ptr++;
            }

            current_settings.main_menu_option++;
            oled_print_menu(current_settings.state);
            break;

        case BLINK_MENU:
            if (current_settings.blink_menu_option == FIX_COUNT)
                return;

            current_settings.blink_menu_option++;
            oled_print_menu(current_settings.state);
            break;
        
        case COUNT:
            if (current_settings.blink_count <= MIN_COUNT_LIMIT)
                return;

            current_settings.blink_count--;
            oled_print_count_number(current_settings.blink_count);
            break;
            
        case STATIC_BRIGHTNESS:
            if (current_settings.led_brightness == MIN_BRIGHTNESS_LIMIT)
                return;

            current_settings.led_brightness -= OLED_DISPLAY_BRIGHTNESS_STEP;
            oled_print_static_led_brightness(current_settings.led_brightness);
            led_set_mode(LED_MODE_FIXED_BRIGHTNESS, current_settings.led_brightness);
            break;

        default:
            printf("Unexpected event!!\n");
            break;
    }
}

/*
 * @brief The event handler for the Enter button pressed
 */
void enter_event_handler() {
    printf ("Enter event handler state: %d\n", current_settings.state);
    switch (current_settings.state) {
        case MAIN_MENU:
            main_menu_handler();
            break;
        
        case BLINK_MENU:
            blink_menu_handler();
            break;

        case COUNT:
            if (current_settings.main_menu_option == FAST_BLINK)
                led_set_mode(LED_MODE_COUNT_FAST, current_settings.blink_count);
            else
                led_set_mode(LED_MODE_COUNT_SLOW, current_settings.blink_count);
            break;

        default:
            break;
    }
}

/**
 * @brief The initialization code for the ssd1306 for using the u8g2 library
 */
void display_manager_init(void) {
    oled_tim3_init();
    u8g2_Setup_ssd1306_i2c_128x64_noname_f(&u8g2, U8G2_R0, u8g2_byte_hw_i2c_stm32, u8g2_gpio_and_delay_stm32);
    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
    clear_display();
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tf);
    oled_print_menu(current_settings.state);
}

/**
 * @brief The API to control the oled events that needs to be controlled via Timers or Intrupts
 */
void process_oled_events(void) {
    // Call the print breathing API if the flag is set via main_menu_handler
    if (is_breathing)
        oled_print_breathing();
}
