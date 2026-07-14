#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "btn.h"
#include "main.h"
#include "display_manager.h"

/* Global static variables to track the last time each button event was processed */
static uint32_t last_back_tick  = 0;
static uint32_t last_down_tick  = 0;
static uint32_t last_up_tick    = 0;
static uint32_t last_enter_tick = 0;

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

    // Route EXTI lines to Port A (Clearing the explicit registers defaults fields to Port A)
    SYSCFG->EXTICR[0] &= ~SYSCFG_EXTICR1_EXTI0;   // Clear EXTI0 field (bits 0-3 of EXTICR1)
    SYSCFG->EXTICR[2] &= ~SYSCFG_EXTICR3_EXTI9;   // Clear EXTI9 field (bits 4-7 of EXTICR3)
    SYSCFG->EXTICR[2] &= ~SYSCFG_EXTICR3_EXTI10;  // Clear EXTI10 field (bits 8-11 of EXTICR3)
    SYSCFG->EXTICR[3] &= ~SYSCFG_EXTICR4_EXTI12;  // Clear EXTI12 field (bits 0-3 of EXTICR4)

    // Set Falling Edge Trigger for all 4 lines
    EXTI->FTSR1 |= (EXTI_FTSR1_FT0 | EXTI_FTSR1_FT9 | EXTI_FTSR1_FT10 | EXTI_FTSR1_FT12);
    
    // Unmask (Enable) Interrupt lines
    EXTI->IMR1 |= (EXTI_IMR1_IM0 | EXTI_IMR1_IM9 | EXTI_IMR1_IM10 | EXTI_IMR1_IM12);

    // Enable channels in NVIC
    NVIC_EnableIRQ(EXTI0_IRQn);       // Private vector for PA0 (Down)
    NVIC_EnableIRQ(EXTI9_5_IRQn);     // Handles PA9 (Back)
    NVIC_EnableIRQ(EXTI15_10_IRQn);   // Handles PA10 (Up) and PA12 (Enter)
}

/*
 * @brief Private Intrupt handler dedicated exclusively to PA0
*/ 
void EXTI0_IRQHandler(void) {
    uint32_t current_tick = HAL_GetTick();

    if (EXTI->PR1 & EXTI_PR1_PIF0) {
        EXTI->PR1 = EXTI_PR1_PIF0; // Clear hardware flag
        
        if ((current_tick - last_down_tick) > DEBOUNCE_DELAY_MS) {
            last_down_tick = current_tick;
            down_event_handler();
        }
    }
}

/*
 * @brief Private Intrupt handler dedicated exclusively to PA9
*/ 
void EXTI9_5_IRQHandler(void) {
    uint32_t current_tick = HAL_GetTick();

    if (EXTI->PR1 & EXTI_PR1_PIF9) {
        EXTI->PR1 = EXTI_PR1_PIF9; // Clear flag
        
        if ((current_tick - last_back_tick) > DEBOUNCE_DELAY_MS) {
            last_back_tick = current_tick;
            back_event_handler();
        }
    }
}

/*
 * @brief Private Intrupt handler dedicated exclusively to PA10 and PA12
*/ 
void EXTI15_10_IRQHandler(void) {
    uint32_t current_tick = HAL_GetTick();

    // Check PA10 (Up Button)
    if (EXTI->PR1 & EXTI_PR1_PIF10) {
        EXTI->PR1 = EXTI_PR1_PIF10; // Clear flag
        
        if ((current_tick - last_up_tick) > DEBOUNCE_DELAY_MS) {
            last_up_tick = current_tick;
            up_event_handler();
        }
    }
    
    // Check PA12 (Enter Button)
    if (EXTI->PR1 & EXTI_PR1_PIF12) {
        EXTI->PR1 = EXTI_PR1_PIF12; // Clear flag
        
        if ((current_tick - last_enter_tick) > DEBOUNCE_DELAY_MS) {
            last_enter_tick = current_tick;
            enter_event_handler();
        }
    }
}
