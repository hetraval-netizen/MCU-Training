#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "main.h"
#include "uart.h"
#include "led_control.h"

#define MIN_ANIMATION_LIMIT 0
#define MAX_ANIMATION_LIMIT 100

volatile bool is_animating = false; 
volatile int current_anim_duty = 0; 
volatile int anim_direction = 1; // 1 means fading UP, -1 means fading DOWN

volatile bool is_counting = false;
volatile int count_value = 0;
volatile int target_blinks = 0;

uint32_t error_start_time = 0; // Stores the timestamp of the error
bool is_error_active = false;  // Tracks if the error LED is currently ON

void led_init(void){
    // Enable clock for GPIOA
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN; 
    
    GPIOA->MODER &= ~GPIO_MODER_MODE8;   // Clear both bits for Pin 8
    GPIOA->MODER |= GPIO_MODER_MODE8_0;  // Set Pin 8 to Output
    
    // Set PA5 to General Purpose Output Mode (01)
    GPIOA->MODER &= ~GPIO_MODER_MODE5;   // Clear both bits for Pin 5
    GPIOA->MODER |= GPIO_MODER_MODE5_1;  // Set Pin 5 to Output

    GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL5; // Clear the 4 bits for Pin 5 (AFSEL5)
    GPIOA->AFR[0] |= (1UL << GPIO_AFRL_AFSEL5_Pos); // Set AF1 for TIM2_CH1 on PA5

}

void trigger_error(void) {
    // 1. Turn ON the Error LED (PA8)
    GPIOA->ODR |= GPIO_ODR_OD8; 
    
    // 2. Record the exact millisecond this happened
    error_start_time = HAL_GetTick(); 
    
    // 3. Set the flag so our background loop knows to watch it
    is_error_active = true;
}

void reset_error_led(void) {
    // If the error LED is on, check if 500 milliseconds have passed
    if (is_error_active && (HAL_GetTick() - error_start_time >= 1000)) {
        
        // Time is up! Turn OFF the Error LED (PA8)
        GPIOA->ODR &= ~GPIO_ODR_OD8; 
        
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
            if (current_anim_duty >= MAX_ANIMATION_LIMIT) {
                current_anim_duty = MAX_ANIMATION_LIMIT; // Safety clamp
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

    TIM2->ARR = 16000 - 1;
    TIM2->CCR1 = 8000;         // Duty Cycle: 50% of 16000
    
    TIM2->CR1 |= TIM_CR1_CEN;  // Restart timer
}

void pwm_start(int duty_cycle) {
    if (duty_cycle < MIN_ANIMATION_LIMIT) duty_cycle = MIN_ANIMATION_LIMIT;
    if (duty_cycle > MAX_ANIMATION_LIMIT) duty_cycle = MAX_ANIMATION_LIMIT;

    is_animating = false; // Stop any ongoing animation
    is_counting = false; // Stop counting if it was active

    uart_sendstring("System Indication: Starting PWM...\r\n");
    
    TIM2->CR1 &= ~TIM_CR1_CEN; // Pause timer

    TIM2->CNT = 0; // Reset counter to start from 0

    TIM2->ARR = MAX_ANIMATION_LIMIT - 1;     // Speed: 1 second (1 Hz)

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