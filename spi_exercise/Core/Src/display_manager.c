#include "display_manager.h"
#include "i2c_peripheral.h"
#include "led_control.h"
#include "uart.h"
#include "mpu.h"
#include "u8g2.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

extern u8g2_t u8g2; // Extern variable for using the u8g2 library

/* The header names for all the menus */
const char *menu_headers[] = {
    "Main Menu",
    "Blink Menu",
    "Fix Count",
    "Breathing",
    "Static LED",
    "Sensor Data",
    "Contineous Fast",
    "Contineous Slow"
};

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
static breathing_settings_t breathing_settings = {
    .is_breathing = false,
    .animation_direction = 1,
    .current_animation_stage = 0,
};

static volatile bool oled_needs_refresh = false;
static unsigned int menu_start_index = 0;
static unsigned int menu_end_index = OLED_DISPLAY_MAX_MENU_OPTIONS;
static bool print_sensor_data = false;
static bool is_blinking = false;

const int center_x = 64;       // Horizontal center of the screen
const int center_y = 44;       // Vertical center of the screen
const float min_radius = 0.0;  // Smallest size of the circle (pixels)
const float max_radius = 20.0; // Largest size of the circle (pixels)
float disc_blink_speed = 0;      // Speed of the breath (higher = faster)
float angle = 0.0;             // Tracks the position in the sine wave

/**
 * @brief Helper function to reset timer 3 registers and stop any ongoing animations
 */
static void reset_tim3_state(void) {
    TIM3->CR1 &= ~TIM_CR1_CEN; // Disable Timer 3
    TIM3->CNT = 0;

    print_sensor_data = false;
    is_blinking = false;
    breathing_settings.is_breathing = false;
}


/**
 * @brief  Helper to reset the TIM3 register configuration and stop the animation.
 */
static void set_tim3_breathing_state(void) {
    TIM3->ARR = OLED_BREATHING_DELAY;
    TIM3->DIER |= TIM_DIER_UIE;
    TIM3->CR1 |= TIM_CR1_CEN;
}

/**
 * @brief  Helper to reset the TIM3 register configuration and stop the animation.
 */
static void set_tim3_sensor_data_state(void) {
    TIM3->ARR = OLED_SENSOR_DATA_DELAY;
    TIM3->DIER |= TIM_DIER_UIE;
    TIM3->CR1 |= TIM_CR1_CEN;
}

static void set_tim3_blink_state(void) {
    TIM3->ARR = 10;
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

        if (breathing_settings.is_breathing) {
            if (breathing_settings.animation_direction == 1) {
                breathing_settings.current_animation_stage += OLED_BREATHING_BAR_STEP;
                if (breathing_settings.current_animation_stage >= OLED_BREATHING_BAR_MAX) {
                    breathing_settings.animation_direction = ANIM_DIRECTION_NEGATIVE;
                }
            } else {
                breathing_settings.current_animation_stage -= OLED_BREATHING_BAR_STEP;
                if (breathing_settings.current_animation_stage == OLED_BREATHING_BAR_MIN) {
                    breathing_settings.animation_direction = ANIM_DIRECTION_POSITIVE;
                }
            }
            oled_needs_refresh = true;
        }

        if (print_sensor_data || is_blinking) {
            oled_needs_refresh = true;
        }
    }
}

/* 
 * @brief API to Clear the oled display 
 */
static void clear_display(){
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
static void oled_print_header(const char *title) {
    unsigned int title_size = (strlen(title) * OLED_TITLE_FONT_WIDTH);
    unsigned int start_ptr = (OLED_DISPLAY_PIXEL_WIDTH - title_size) / 2;

    // Set bold font for the title
    u8g2_SetFont(&u8g2, u8g2_font_7x14B_tr);
    u8g2_DrawStr(&u8g2, start_ptr, OLED_TITLE_STR_INIT_Y_AXIS, title);
    u8g2_DrawHLine(&u8g2, 0, OLED_TITLE_LINE_Y_AXIS, OLED_DISPLAY_PIXEL_WIDTH);
    u8g2_SetFont(&u8g2, u8g2_font_6x10_tf); // Switch to normal font
}

/**
 * @brief This API will print the main menu on the oled display
 */
static void oled_print_main_menu(void) {
    clear_display();

    oled_print_header(menu_headers[MAIN_MENU_INDEX]);

    unsigned int x_axis = OLED_DISPLAY_STR_INIT_X_AXIS;
    unsigned int y_axis = OLED_DISPLAY_STR_INIT_Y_AXIS;
    unsigned int current_box_pos = OLED_DISPLAY_INIT_BOX_POS;

    for (int menu_ptr = menu_start_index; menu_ptr < menu_end_index + 1; menu_ptr++){
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

/**
 * @brief This API will print the blink menu on the oled display
 */
static void oled_print_blink_menu(void) {
    clear_display();

    oled_print_header(menu_headers[BLINK_MENU_INDEX]);

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
 * @brief The API to print the desired counts for the display
 */
static void oled_print_count_number (int count) {
    clear_display();

    oled_print_header(menu_headers[FIX_COUNT_MENU_INDEX]);
    char temp_str[24];
    snprintf(temp_str, sizeof(temp_str), "Total blinks: <%d>", count);

    u8g2_DrawStr(&u8g2, OLED_DISPLAY_STR_MIDDLE_INIT_X_AXIS, OLED_DISPLAY_STR_INIT_Y_AXIS, temp_str);
    u8g2_SendBuffer(&u8g2);
}

/**
 * @brief Helper API to draw the brightness bar on the oled
 */
static void oled_draw_brightness_bar (int percentage) {
    float bar_width = percentage * OLED_DISPLAY_PERCENTAGE_TO_PIXEL_FOR_BRIGHTNESS_BAR;
    u8g2_DrawBox(&u8g2, 0, OLED_DISPLAY_BOX_POS_BRIGHTNESS_BAR, bar_width, OLED_DISPLAY_BOX_HEIGHT);
}

/*
 * @brief The API to print the led brightness bar on the oled display.
 *        This function is non-blocking and relies on TIM3 interrupts.
 */
static void oled_print_breathing (void) {
    set_tim3_breathing_state();
    if (breathing_settings.is_breathing && oled_needs_refresh) {
        oled_needs_refresh = false;
        u8g2_ClearBuffer(&u8g2);
        oled_print_header(menu_headers[BREATHING_MENU_INDEX]);
        oled_draw_brightness_bar (breathing_settings.current_animation_stage);
        u8g2_SendBuffer(&u8g2);
    }
}

/*
 * @brief The API to print the led brightness bar on the oled display
 */
static void oled_print_static_led_brightness (int brightness_level) {
    clear_display();
    oled_print_header(menu_headers[STATIC_LED_MENU_INDEX]);

    char temp_str[22];
    snprintf(temp_str, sizeof(temp_str), "Brightness level: %d", brightness_level);
    u8g2_DrawStr(&u8g2, 0, OLED_DISPLAY_STR_INIT_Y_AXIS, temp_str);
    oled_draw_brightness_bar(brightness_level);
    u8g2_SendBuffer(&u8g2);
}

/**
 * @brief The API to show the blink animation on display
 */
static void oled_print_blink (void) {
    u8g2_ClearBuffer(&u8g2);

    if (!is_blinking || !oled_needs_refresh)
        return;

    set_tim3_blink_state();
    if (current_settings.main_menu_option == FAST_BLINK) {
        disc_blink_speed = OLED_CIRCLE_FAST_BLINK;
        oled_print_header(menu_headers[CONTINEOUS_FAST_MENU_INDEX]);
    }
    else {
        oled_print_header(menu_headers[CONTINEOUS_SLOW_MENU_INDEX]);
        disc_blink_speed = OLED_CIRCLE_SLOW_BLINK;
    }

    oled_needs_refresh = false;
    float sineValue = sin(angle);

    // Normalize sineValue to 0.0 to 1.0 range
    float normalizedSine = (sineValue + 1.0) / 2.0;

    // Scale the normalized value to your pixel range
    int currentRadius = min_radius + (normalizedSine * (max_radius - min_radius));

    // Use u8g2.drawDisc() for a filled circle, or u8g2.drawCircle() for an outline
    u8g2_DrawDisc(&u8g2, center_x, center_y, currentRadius, U8G2_DRAW_ALL);

    u8g2_SendBuffer(&u8g2);           // Push internal memory to the display

    // 3. Advance the animation
    angle += disc_blink_speed;
    if (angle >= 2 * PI)
        angle = 0.0;               // Keep angle within bounds to prevent overflow
}

/**
 * @brief Helper API to truncate a reading to its first 3 digits.
 */
static int truncate_to_3_digits(int value)
{
    while (value >= 1000 || value <= -1000)
        value /= 10;

    return value;
}

/**
 * @brief API to print the current MPU-9250 sensor values.
 */
static void oled_print_sensor_data(void)
{
    set_tim3_sensor_data_state();

    if (!print_sensor_data || !oled_needs_refresh)
        return;

    oled_needs_refresh = false;

    u8g2_ClearBuffer(&u8g2);
    oled_print_header(menu_headers[SENSOR_DATA_MENU_INDEX]);

    char temp_str[2][21];
    int16_t accel[3] = {0};
    int16_t gyro[3] = {0};

    mpu_read_raw_data(accel, gyro);

    snprintf(temp_str[0], sizeof(temp_str[0]),
             "X:%03d Y:%03d Z:%03d",
             truncate_to_3_digits(accel[0]),
             truncate_to_3_digits(accel[1]),
             truncate_to_3_digits(accel[2]));

    snprintf(temp_str[1], sizeof(temp_str[1]),
             "X:%03d Y:%03d Z:%03d",
             truncate_to_3_digits(gyro[0]),
             truncate_to_3_digits(gyro[1]),
             truncate_to_3_digits(gyro[2]));

    unsigned int x_axis = OLED_DISPLAY_STR_INIT_X_AXIS;
    unsigned int y_axis = OLED_DISPLAY_STR_INIT_Y_AXIS;
    unsigned int current_box_pos = OLED_DISPLAY_INIT_BOX_POS;

    for (int menu_ptr = 0; menu_ptr < 2; menu_ptr++)
    {
        u8g2_DrawStr(&u8g2, x_axis, y_axis, temp_str[menu_ptr]);

        y_axis += OLED_DISPLAY_STR_STEP_SIZE;
        current_box_pos += OLED_DISPLAY_STR_STEP_SIZE;
    }

    u8g2_SendBuffer(&u8g2);
}


/*
 * @brief The API to print the desired menu type on the oled display
 */
static void oled_print_menu (display_state_t menu_type) {
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
    printf("Current main menu setting: %d\n\r", current_settings.main_menu_option);
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
            breathing_settings.current_animation_stage = OLED_BREATHING_BAR_INIT;
            breathing_settings.is_breathing = true;
            oled_print_breathing();
            led_set_mode(LED_MODE_BREATHING, 0); // Complete snippet implementation
            process_oled_events();
            break;

        case STATIC_LED:
            current_settings.state = STATIC_BRIGHTNESS;
            oled_print_static_led_brightness(current_settings.led_brightness);
            led_set_mode(LED_MODE_FIXED_BRIGHTNESS, current_settings.led_brightness);
            break;

        case SENSOR_DATA:
            current_settings.state = SENSOR_MENU;
            print_sensor_data = true;
            oled_print_sensor_data();
            process_oled_events();
            break;
        
        default:
            printf("Unexpected event!!\n");
            break;
    }
}

static void blink_menu_handler (void) {
    switch (current_settings.blink_menu_option) {
        case CONTINEOUS:
            printf("Setting is_blinking as true\n");
            is_blinking = true;
            oled_needs_refresh = true;
            set_tim3_blink_state();
            current_settings.state = CONTINEOUS_BLINK;
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
            current_settings.blink_menu_option = 0;
            break;

        case CONTINEOUS_BLINK:
            current_settings.state = BLINK_MENU;
            is_blinking = false;
            reset_tim3_state();
            oled_print_menu(current_settings.state);
            break;

        case COUNT:
            current_settings.state = BLINK_MENU;
            oled_print_menu(current_settings.state);
            break;

        case BREATHE_MENU:
            current_settings.state = MAIN_MENU;
            breathing_settings.is_breathing = false;
            reset_tim3_state();
            oled_print_menu(current_settings.state);
            break;

        case STATIC_BRIGHTNESS:
            current_settings.state = MAIN_MENU;
            oled_print_menu(current_settings.state);
            break;

        case SENSOR_MENU:
            current_settings.state = MAIN_MENU;
            print_sensor_data = false;
            reset_tim3_state();
            oled_print_menu(current_settings.state);

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
            
            if (current_settings.main_menu_option == menu_start_index){
                menu_start_index--;
                menu_end_index--;
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
            
            if (current_settings.main_menu_option == menu_end_index){
                menu_start_index++;
                menu_end_index++;
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
    if (breathing_settings.is_breathing)
        oled_print_breathing();

    if (print_sensor_data)
        oled_print_sensor_data();

    if (is_blinking)
        oled_print_blink();
}
