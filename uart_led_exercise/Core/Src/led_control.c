#include "main.h"
#include "console.h"
#include "led_control.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

void toggle_led(int led_num) {
    GPIOA->ODR ^= (1UL << led_num);
}

void set_led(int led_num, bool state) {
    if (state) {
        GPIOA->ODR |= (1UL << led_num); // Set the bit to turn on the LED
    } else {
        GPIOA->ODR &= ~(1UL << led_num); // Clear the bit to turn off the LED
    }
}

void TIM2_IRQHandler(void) {
    // Check if the "Update Interrupt Flag" (UIF) triggered this
    if (TIM2->SR & TIM_SR_UIF) {
        
        // 1. CLEAR THE FLAG! If you forget this, the CPU gets stuck here forever.
        TIM2->SR &= ~TIM_SR_UIF;

        // 2. Toggle the LED on PA5
        GPIOA->ODR ^= GPIO_ODR_OD5;
    }
}

void fast_blink(void) {
    uart_sendstring("System Indication: Starting Fast Blink...\r\n");
    TIM2->ARR = 3000 - 1;      // Set timer to 300ms
    TIM2->CR1 |= TIM_CR1_CEN; // Enable the Timer!
}

void slow_blink(void) {
    uart_sendstring("System Indication: Starting Slow Blink...\r\n");
    TIM2->ARR = 10000 - 1;      // Set timer to 1000ms
    TIM2->CR1 |= TIM_CR1_CEN; // Enable the Timer!
}

void pwm_start(void) {
    uart_sendstring("System Indication: Starting PWM...\r\n");
    // We will implement this later in the course! For now, just a placeholder.
}

void count(void) {
    uart_sendstring("System Indication: Starting Count...\r\n");
    // We will implement this later in the course! For now, just a placeholder.
}