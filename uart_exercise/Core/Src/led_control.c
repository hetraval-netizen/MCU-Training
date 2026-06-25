#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "main.h"
#include "uart.h"
#include "led_control.h"

/* --- Volatile State Variables (Accessed in ISR and Threads) --- */
volatile bool is_animating = false;     // Controls breathing mode active status
volatile int current_anim_duty = 0;     // Stores the live dynamic PWM duty cycle value
volatile int anim_direction = 1;        // Tracks fade direction: 1=UP, -1=DOWN

volatile bool is_counting = false;      // Controls blink sequence loop execution
volatile int count_value = 0;           // Tracks active elapsed transitions completed
volatile int target_blinks = 0;         // Upper cutoff limit for finite count tracker

/**
 * @brief  helper to reset the TIM2 register configuration.
 */
static void led_reset_timer_state(void) {
    TIM2->CR1 &= ~TIM_CR1_CEN;
    TIM2->CNT = 0;
    TIM2->CCR1 = 0;
    is_animating = false;
    is_counting = false;
}

/*
 * @brief This API will be initalising the timer for PWM
 */
void timer_init(void) {
  RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN; 

  // Gear down to 10,000 Hz (10 ticks per millisecond)
  TIM2->PSC = 8000 - 1; 

  // Configure Channel 1 for PWM Mode 1
  // (Output goes HIGH when counter < CCR1, and LOW when counter >= CCR1)
  TIM2->CCMR1 &= ~TIM_CCMR1_OC1M;
  TIM2->CCMR1 |= (6UL << TIM_CCMR1_OC1M_Pos); 

  // Enable the Output on Channel 1 so it can drive PA5
  TIM2->CCER |= TIM_CCER_CC1E;

  TIM2->DIER |= TIM_DIER_UIE;
  NVIC_EnableIRQ(TIM2_IRQn);
}

/**
 * @brief  Initializes GPIOA pins for Status (PA5) and Error (PA8) LEDs.
 */
void led_init(void){
    /* Supply power clock to GPIOA port peripheral */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN; 
    
    /* Configure Pin 8 (Error Indicator LED) as General Purpose Output */
    GPIOA->MODER &= ~GPIO_MODER_MODE8;   
    GPIOA->MODER |= GPIO_MODER_MODE8_0;  
    
    /* Configure Pin 5 (Status PWM LED) as Alternate Function Mode */
    GPIOA->MODER &= ~GPIO_MODER_MODE5;   
    GPIOA->MODER |= GPIO_MODER_MODE5_1;  

    /* Map GPIOA Pin 5 specifically to hardware Alternate Function 1 (TIM2_CH1) */
    GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL5;
    GPIOA->AFR[0] |= (1UL << GPIO_AFRL_AFSEL5_Pos);

    timer_init();
}

/*
 * @brief This API toggles the LED for the error indication
*/
void set_error_led(bool start) {
    static uint32_t error_start_time = 0;
    if (start) {
        GPIOA->ODR |= GPIO_ODR_OD8;       
        error_start_time = HAL_GetTick();
        return;
    }
    
    if (HAL_GetTick() - error_start_time >= 1000)
        GPIOA->ODR &= ~GPIO_ODR_OD8;
}

/**
 * @brief  Centralized API to control status LED behavioral operational configurations.
 */
void led_set_mode(Led_Mode_t mode, int param) {
    led_reset_timer_state();

    switch (mode) {
        case LED_MODE_FAST_BLINK:
            uart_sendstring("System Indication: Starting Fast Blink...\r\n");
            TIM2->ARR = 5000 - 1;
            TIM2->CCR1 = 2500;
            break;

        case LED_MODE_SLOW_BLINK:
            uart_sendstring("System Indication: Starting Slow Blink...\r\n");
            TIM2->ARR = 16000 - 1;
            TIM2->CCR1 = 8000;
            break;

        case LED_MODE_PWM:
            if (param < MIN_ANIMATION_LIMIT) param = MIN_ANIMATION_LIMIT;
            if (param > MAX_ANIMATION_LIMIT) param = MAX_ANIMATION_LIMIT;

            uart_sendstring("System Indication: Starting PWM...\r\n");
            TIM2->ARR = MAX_ANIMATION_LIMIT - 1;
            TIM2->CCR1 = param - 1;
            break;

        case LED_MODE_BREATHING:
            uart_sendstring("System Indication: Starting Breathing LED...\r\n");
            TIM2->ARR = 100 - 1;
            
            current_anim_duty = 0;
            anim_direction = 1;
            is_animating = true;
            break;

        case LED_MODE_COUNT:
            uart_sendstring("System Indication: Starting Count...\r\n");
            count_value = 0;
            target_blinks = param;
            TIM2->ARR = 16000 - 1;
            TIM2->CCR1 = 8000;
            is_counting = true;
            break;

        default:
            uart_sendstring("System Error: Invalid LED Mode Selected\r\n");
            return;
    }

    TIM2->CR1 |= TIM_CR1_CEN;
}

/**
 * @brief  TIM2 Global Update Interrupt Service Routine.
 */
void TIM2_IRQHandler(void) {
    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR &= ~TIM_SR_UIF; 

        if (is_animating) {
            current_anim_duty += anim_direction; 
            TIM2->CCR1 = current_anim_duty;      

            if (current_anim_duty >= MAX_ANIMATION_LIMIT) {
                current_anim_duty = MAX_ANIMATION_LIMIT; 
                anim_direction = -1; 
            } 
            else if (current_anim_duty <= 0) {
                current_anim_duty = 0; 
                anim_direction = 1;  
            }
        }
        else if (is_counting) {
            count_value++; 

            if (count_value >= target_blinks) {
                is_counting = false;       
                TIM2->CR1 &= ~TIM_CR1_CEN; 
                TIM2->CCR1 = 0;            
                uart_sendstring("System Indication: Count Complete!\r\n");
            }
        }
    }
}
