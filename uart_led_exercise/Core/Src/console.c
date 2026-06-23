#include "main.h"
#include "console.h"
#include "led_control.h"
#include "oled.h"
#include "mpu.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#define RX_BFR_SIZE 64
volatile char rx_buffer[RX_BFR_SIZE];
volatile int head = 0;
volatile int tail = 0;

char cmd_buffer[RX_BFR_SIZE];
int cmd_index = 0;
bool cmd_ready = false;

volatile uint32_t last_press_time_pin9  = 0;
volatile uint32_t last_press_time_pin10 = 0;

#define DEBOUNCE_DELAY_MS 50 

volatile int current_setting = 0;
volatile int selected_setting = 0;


void oled_execute_led_pattern(int led_pattern){
    if (led_pattern == FAST_BLINK)
        fast_blink();
    
    if (led_pattern == SLOW_BLINK)
        slow_blink();

    if (led_pattern == BREATHING)
        pwm_animation();

    return;
}


void EXTI9_5_IRQHandler(void) {
    if (EXTI->PR1 & EXTI_PR1_PIF9) {
        EXTI->PR1 = EXTI_PR1_PIF9; // Clear flag
        
        uint32_t current_time = HAL_GetTick();

        if ((current_time - last_press_time_pin9) < DEBOUNCE_DELAY_MS)
            return;

        last_press_time_pin9 = current_time;
        
        current_setting++;
        if (current_setting >= 3)
            current_setting = 0;
        
        print_setting_oled(current_setting);
        

        uart_sendstring("scroll button pressed\r\n");
    }
}

void EXTI15_10_IRQHandler(void) {
    if (EXTI->PR1 & EXTI_PR1_PIF10) {
        EXTI->PR1 = EXTI_PR1_PIF10; 

        uint32_t current_time = HAL_GetTick();

        if ((current_time - last_press_time_pin10) < DEBOUNCE_DELAY_MS)
            return;

        last_press_time_pin10 = current_time;

        if (selected_setting == current_setting) {
            uart_sendstring("Setting already in action");
        }
        selected_setting = current_setting;
        oled_execute_led_pattern(selected_setting);
        uart_sendstring("select button pressed\r\n");
    }
}


void uart_sendchar (char c) {
    // Wait until the previous bit is send and we are ready for next char
    // TXE Transmit empty bit is set in ISR register once the bit is moved to shift register

    while (!(USART2->ISR & (1UL << 7))) {
    }

    // Then drop the character in the TDR (Transmit data register)
    USART2->TDR = c;
}

void uart_sendstring (char *str) {
    while (*str != '\0'){
        uart_sendchar(*str);
        str++;
    }
}

void USART2_IRQHandler (void) {
    if (USART2->ISR & USART_ISR_RXNE) {
        rx_buffer[head] = USART2->RDR; // Read received byte
        head = (head + 1) % RX_BFR_SIZE; // Move head index
    }

    // shield so the ISR doesn't get stuck if some other interrupt flags are set (like ORE, FE, PE)
    if (USART2->ISR & 0x0F) {
        USART2->ICR = 0x0F; 
    }
}

void parse_command(char *cmd) {
    if (strcmp(cmd, "help") == 0) {
        uart_sendstring("\r\nAvailable commands:\r\n"
                        "  fast                            - Fast blink\r\n"
                        "  slow                            - Slow blink\r\n"
                        "  count <Number of Blinks>        - Start counting\r\n"
                        "  led brightness <Led Brightness> - Glow led at a specific brightness\r\n"
                        "  breathing                       - Start breathing animation\r\n"
                        "  imu                             - Print IMU stats\r\n"
                        "  help                            - Show this help message\r\n");
    }
    else if (strncmp(cmd, "led brightness ", 15) == 0){
        char *endptr;
        long duty = strtol(cmd + 15, &endptr, 10);
        pwm_start(duty);
    }
    else if (strcmp (cmd, "breathing") == 0){
        pwm_animation();
    }
    else if (strcmp(cmd, "fast") == 0) {
        fast_blink();
    }
    else if (strcmp(cmd, "slow") == 0) {
        slow_blink();
    }
    else if (strncmp(cmd, "count ", 6) == 0) {
        char *endptr;
        long target = strtol(cmd + 6, &endptr, 10);
        count(target);
    }
    else if (strcmp(cmd, "imu") == 0) {
        int16_t accel[3] = {0};
        int16_t gyro[3] = {0};
        char output_bfr[128];

        // 1. Fetch fresh raw bits from the MPU-9250 over SPI
        mpu_read_raw_data(accel, gyro);

        // 2. Print Accelerometer Raw Values
        uart_sendstring("\r\n--- MPU-9250 Raw Sensor Telemetry ---\r\n");
        sprintf(output_bfr, "  ACCEL -> X: %d\tY: %d\tZ: %d\r\n", accel[0], accel[1], accel[2]);
        uart_sendstring(output_bfr);

        // 3. Print Gyroscope Raw Values
        sprintf(output_bfr, "  GYRO  -> X: %d\tY: %d\tZ: %d\r\n", gyro[0], gyro[1], gyro[2]);
        uart_sendstring(output_bfr);
        uart_sendstring("-------------------------------------\r\n");
    }
    else if (cmd[0] != '\0') { 
        uart_sendstring("Error: Unknown command. Type 'help'.\r\n");
        trigger_error(); // <-- Flash the Red LED!
    }
}

void console_init(void) {

    uart_sendstring("Welcome to the UART Console!\r\n");
    uart_sendstring("Type 'help' for a list of commands.\r\n");
    uart_sendstring("\r> "); // Print prompt

    while (1){

        process_error_led();

        if (head != tail){
            char current_char = rx_buffer[tail];
            tail = (tail + 1) % RX_BFR_SIZE; // Move tail index

            uart_sendchar(current_char); // Echo back the received character

            if (current_char == '\r'){
                uart_sendstring("\r\n"); // Move to new line after Enter key
                cmd_buffer[cmd_index] = '\0'; // Null-terminate the command string
                parse_command(cmd_buffer);
                uart_sendstring("\r> "); // Print a fresh prompt
                cmd_index = 0;

            }
            else if (current_char == '\b'){
                if (cmd_index > 0) {
                    cmd_index--;

                    uart_sendstring(" \b");
                }
                else {
                    uart_sendchar('\a'); // Beep if backspace is pressed at the beginning of the line
                }
                 // Move back the command index for backspace
            }
            else {
                cmd_buffer[cmd_index] = current_char; // Store character in command buffer
                cmd_index++;
            }
        }
    }
}
