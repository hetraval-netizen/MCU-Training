#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "btn.h"
#include "main.h"
#include "display_manager.h"

/* Initalize all the button event flags with false initially */
static button_status_t btn_status = {false, false, false, false};

/*
 * @brief The API to initialize all the required push buttons
 */
void btn_init(void){
    // Enable GPIOA and SYSCFG peripheral clocks
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    // PA0 Configuration (Down button)
    GPIOA->MODER &= ~GPIO_MODER_MODE0;
    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD0;
    GPIOA->PUPDR |= GPIO_PUPDR_PUPD0_0;   // Pull-up

    // PA9 Configuration (Back Button)
    GPIOA->MODER &= ~GPIO_MODER_MODE9;
    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD9;
    GPIOA->PUPDR |= GPIO_PUPDR_PUPD9_0;   // Pull-up

    // PA10 Configuration (Up Button)
    GPIOA->MODER &= ~GPIO_MODER_MODE10;
    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD10;
    GPIOA->PUPDR |= GPIO_PUPDR_PUPD10_0;  // Pull-up

    // PA12 Configuration (Enter Button)
    GPIOA->MODER &= ~GPIO_MODER_MODE12;
    GPIOA->PUPDR &= ~GPIO_PUPDR_PUPD12;
    GPIOA->PUPDR |= GPIO_PUPDR_PUPD12_0;  // Pull-up

    // Route EXTI lines to Port A 
    SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI0;   
    SYSCFG->EXTICR[2] &= ~SYSCFG_EXTICR3_EXTI9;   
    SYSCFG->EXTICR[2] &= ~SYSCFG_EXTICR3_EXTI10;  
    SYSCFG->EXTICR[3] &= ~SYSCFG_EXTICR4_EXTI12;  

    // Falling Edge Trigger Register (Detects when button is PUSHED DOWN)
    EXTI->FTSR1 |= (EXTI_FTSR1_FT0 | EXTI_FTSR1_FT9 | EXTI_FTSR1_FT10 | EXTI_FTSR1_FT12);
    
    // Rising Edge Trigger Register (Detects when button is RELEASED)
    EXTI->RTSR1 |= (EXTI_RTSR1_RT0 | EXTI_RTSR1_RT9 | EXTI_RTSR1_RT10 | EXTI_RTSR1_RT12); 

    // Unmask (Enable) Interrupt lines
    EXTI->IMR1 |= (EXTI_IMR1_IM0 | EXTI_IMR1_IM9 | EXTI_IMR1_IM10 | EXTI_IMR1_IM12);

    // Enable channels in NVIC
    NVIC_EnableIRQ(EXTI0_IRQn);       
    NVIC_EnableIRQ(EXTI9_5_IRQn);     
    NVIC_EnableIRQ(EXTI15_10_IRQn);   
}

/*
 * @brief Private Interrupt handler dedicated exclusively to PA0 (Down Button)
 */ 
void EXTI0_IRQHandler(void) {
    if (EXTI->PR1 & EXTI_PR1_PIF0) {
        EXTI->PR1 = EXTI_PR1_PIF0; // Clear hardware pending flag
        
        static uint32_t down_press_time = 0; 
        uint32_t current_tick = HAL_GetTick();

        if ((GPIOA->IDR & GPIO_PIN_0) == 0) {
            // FIRING EDGE 1 (Falling Edge): Button was just pushed down
            printf("Down button pressed!!\n");
            down_press_time = current_tick;
        }
        else {
            // FIRING EDGE 2 (Rising Edge): Button was just let go!
            // Now the logic can reliably subtract the initial press timestamp
            if ((current_tick - down_press_time) >= DEBOUNCE_DELAY_MS) {
                printf("time till tick: %ld, debounce: %d\n", (current_tick - down_press_time), DEBOUNCE_DELAY_MS);
                btn_status.down_event = true;
            }
        }
    }
}

/*
 * @brief Private Interrupt handler dedicated exclusively to PA9 (Back Button)
 */ 
void EXTI9_5_IRQHandler(void) {
    if (EXTI->PR1 & EXTI_PR1_PIF9) {
        EXTI->PR1 = EXTI_PR1_PIF9; 
        
        static uint32_t back_press_time = 0;
        uint32_t current_tick = HAL_GetTick();

        if ((GPIOA->IDR & GPIO_PIN_9) == 0) {
            printf("Back button pressed!!\n");
            back_press_time = current_tick;
        }
        else {
            if ((current_tick - back_press_time) >= DEBOUNCE_DELAY_MS) {
                printf("time till tick: %ld, debounce: %d\n", (current_tick - back_press_time), DEBOUNCE_DELAY_MS);
                btn_status.back_event = true;
            }
        }
    }
}

/*
 * @brief Private Interrupt handler dedicated exclusively to PA10 and PA12 (Up & Enter)
 */ 
void EXTI15_10_IRQHandler(void) {
    uint32_t current_tick = HAL_GetTick();
    static uint32_t up_press_time = 0;
    static uint32_t enter_press_time = 0;

    // Check PA10 (Up Button)
    if (EXTI->PR1 & EXTI_PR1_PIF10) {
        EXTI->PR1 = EXTI_PR1_PIF10; 
        
        if ((GPIOA->IDR & GPIO_PIN_10) == 0) {
            printf("Up button pressed!!\n");
            up_press_time = current_tick;
        }
        else {
            if ((current_tick - up_press_time) >= DEBOUNCE_DELAY_MS) {
                printf("time till tick: %ld, debounce: %d\n", (current_tick - up_press_time), DEBOUNCE_DELAY_MS);
                btn_status.up_event = true;
            }
        }
    }
    
    // Check PA12 (Enter Button)
    if (EXTI->PR1 & EXTI_PR1_PIF12) {
        EXTI->PR1 = EXTI_PR1_PIF12; 
        
        if ((GPIOA->IDR & GPIO_PIN_12) == 0) {
            printf("Enter button pressed!!\n");
            enter_press_time = current_tick;
        }
        else {
            if ((current_tick - enter_press_time) >= DEBOUNCE_DELAY_MS) {
                printf("time till tick: %ld, debounce: %d\n", (current_tick - enter_press_time), DEBOUNCE_DELAY_MS);
                btn_status.enter_event = true;
            }
        }
    }
}

/*
 * @brief The API to proccess various events based on the button pressed
 */
void button_process_events(void) {
    button_status_t active_clicks = btn_status;
    
    btn_status.back_event = false;
    btn_status.up_event = false;
    btn_status.down_event = false;
    btn_status.enter_event = false;

    if (active_clicks.back_event) {
        back_event_handler();
    }
    if (active_clicks.up_event) {
        up_event_handler();
    }
    if (active_clicks.down_event) {
        down_event_handler();
    }
    if (active_clicks.enter_event) {
        enter_event_handler();
    }
}
