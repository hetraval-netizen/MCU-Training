#include "main.h"
#include "console.h"
#include "led_control.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

volatile bool is_animating = false; 
volatile int current_anim_duty = 0; 
volatile int anim_direction = 1; // 1 means fading UP, -1 means fading DOWN

volatile bool is_counting = false;
volatile int count_value = 0;
volatile int target_blinks = 0;

uint32_t error_start_time = 0; // Stores the timestamp of the error
bool is_error_active = false;  // Tracks if the error LED is currently ON

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

void trigger_error(void) {
    // 1. Turn ON the Error LED (PA8)
    GPIOA->ODR |= (1UL << 8); 
    
    // 2. Record the exact millisecond this happened
    error_start_time = HAL_GetTick(); 
    
    // 3. Set the flag so our background loop knows to watch it
    is_error_active = true;
}

void process_error_led(void) {
    // If the error LED is on, check if 500 milliseconds have passed
    if (is_error_active && (HAL_GetTick() - error_start_time >= 1000)) {
        
        // Time is up! Turn OFF the Error LED (PA8)
        GPIOA->ODR &= ~(1UL << 8); 
        
        // Reset the flag
        is_error_active = false;
    }
}

void TIM2_IRQHandler(void) {
    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR &= ~TIM_SR_UIF; // Clear flag

        // --- The Breathing Animation State Machine ---
        if (is_animating) {
            // 1. Change the brightness based on our current direction
            current_anim_duty += anim_direction;      
            TIM2->CCR1 = current_anim_duty; 

            // 2. Did we hit the ceiling? Turn around!
            if (current_anim_duty >= 100) {
                current_anim_duty = 100; // Safety clamp
                anim_direction = -1;     // Change direction to DOWN
            } 
            // 3. Did we hit the floor? Turn around!
            else if (current_anim_duty <= 0) {
                current_anim_duty = 0;   // Safety clamp
                anim_direction = 1;      // Change direction to UP
            }
        }
        else if (is_counting) {
            count_value++;

            if (count_value >= target_blinks) {
                is_counting = false;
                TIM2->CR1 &= ~TIM_CR1_CEN; // Stop timer
                TIM2->CCR1 = 0;
                uart_sendstring("System Indication: Count Complete!\r\n");
            }
        }
    }
}

void fast_blink(void) {
    uart_sendstring("System Indication: Starting Fast Blink...\r\n");
    is_animating = false; // Stop any ongoing animation
    is_counting = false; // Stop counting if it was active
    TIM2->CR1 &= ~TIM_CR1_CEN; // Pause timer to update safely

    TIM2->CNT = 0; // Reset counter to start from 0

    TIM2->ARR = 5000 - 1;      // Speed: 100ms (10 Hz)
    TIM2->CCR1 = 2500;          // Duty Cycle: 50% of 1000
    
    TIM2->CR1 |= TIM_CR1_CEN;  // Restart timer
}

void slow_blink(void) {
    uart_sendstring("System Indication: Starting Slow Blink...\r\n");
    is_animating = false; // Stop any ongoing animation
    is_counting = false; // Stop counting if it was active
    TIM2->CR1 &= ~TIM_CR1_CEN; // Pause timer
    
    TIM2->CNT = 0; // Reset counter to start from 0

    TIM2->ARR = 16000 - 1;      // Speed: 500ms (2 Hz)
    TIM2->CCR1 = 8000;         // Duty Cycle: 50% of 5000
    
    TIM2->CR1 |= TIM_CR1_CEN;  // Restart timer
}

void pwm_start(int duty_cycle) {
    if (duty_cycle < 0) duty_cycle = 0;
    if (duty_cycle > 100) duty_cycle = 100;

    is_animating = false; // Stop any ongoing animation
    is_counting = false; // Stop counting if it was active

    uart_sendstring("System Indication: Starting PWM...\r\n");
    
    TIM2->CR1 &= ~TIM_CR1_CEN; // Pause timer

    TIM2->CNT = 0; // Reset counter to start from 0

    TIM2->ARR = 100 - 1;     // Speed: 1 second (1 Hz)

    TIM2->CCR1 = duty_cycle -1; // Duty Cycle: duty_cycle% of 100

    TIM2->CR1 |= TIM_CR1_CEN;  // Restart timer
}

void pwm_animation(void){
    uart_sendstring("System Indication: Starting Breathing LED...\r\n");
    is_counting = false; // Stop counting if it was active
    
    TIM2->CR1 &= ~TIM_CR1_CEN; // Pause timer

    TIM2->CNT = 0;             
    TIM2->ARR = 100 - 1;       // 100 Hz frequency
    TIM2->CCR1 = 0;            
    
    // Arm the state machine!
    current_anim_duty = 0;
    anim_direction = 1;        // <-- Start by going UP
    is_animating = true;       

    TIM2->CR1 |= TIM_CR1_CEN;  // Restart timer
}

void count(int target) {
    uart_sendstring("System Indication: Starting Count...\r\n");
    is_animating = false; // Stop any ongoing animation
    is_counting = true;
    count_value = 0;
    target_blinks = target;

    TIM2->CR1 &= ~TIM_CR1_CEN; // Pause timer

    TIM2->CNT = 0; // Reset counter to start from 0
    TIM2->ARR = 16000 - 1;
    TIM2->CCR1 = 8000; // Duty Cycle: 50% of 16000

    TIM2->CR1 |= TIM_CR1_CEN;  // Restart timer
}