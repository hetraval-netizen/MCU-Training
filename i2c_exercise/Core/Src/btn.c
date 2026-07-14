#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "main.h"
#include "display_manager.h"

// Debounce interval in milliseconds
#define DEBOUNCE_DELAY_MS 200

// Track the last time each button event was processed
static uint32_t last_back_tick  = 0;
static uint32_t last_down_tick  = 0;
static uint32_t last_up_tick    = 0;
static uint32_t last_enter_tick = 0;

void btn_init(void){
    // Enable GPIOA and SYSCFG peripheral clocks
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    // --- PA0 Configuration (NEW SAFEST PIN FOR DOWN BUTTON) ---
    GPIOA->MODER &= ~GPIO_MODER_MODE0;
    GPIOA->PUPDR &= ~(3UL << 0);
    GPIOA->PUPDR |= (1UL << 0);   // Pull-up

    // --- PA9 Configuration (Back Button) ---
    GPIOA->MODER &= ~GPIO_MODER_MODE9;
    GPIOA->PUPDR &= ~(3UL << 18);
    GPIOA->PUPDR |= (1UL << 18);  // Pull-up

    // --- PA10 Configuration (Up Button) ---
    GPIOA->MODER &= ~GPIO_MODER_MODE10;
    GPIOA->PUPDR &= ~(3UL << 20);
    GPIOA->PUPDR |= (1UL << 20);  // Pull-up

    // --- PA12 Configuration (Enter Button) ---
    GPIOA->MODER &= ~GPIO_MODER_MODE12;
    GPIOA->PUPDR &= ~(3UL << 24);
    GPIOA->PUPDR |= (1UL << 24);  // Pull-up

    // Route EXTI lines to Port A (Clearing the explicit registers defaults fields to Port A)
    SYSCFG->EXTICR[0] &= ~(0xFU << 0);  // Clear EXTI0 field (bits 0-3 of EXTICR1)
    SYSCFG->EXTICR[2] &= ~(0xFU << 4);  // Clear EXTI9 field (bits 4-7 of EXTICR3)
    SYSCFG->EXTICR[2] &= ~(0xFU << 8);  // Clear EXTI10 field (bits 8-11 of EXTICR3)
    SYSCFG->EXTICR[3] &= ~(0xFU << 0);  // Clear EXTI12 field (bits 0-3 of EXTICR4)

    // Set Falling Edge Trigger for all 4 lines
    EXTI->FTSR1 |= (EXTI_FTSR1_FT0 | EXTI_FTSR1_FT9 | EXTI_FTSR1_FT10 | EXTI_FTSR1_FT12);
    
    // Unmask (Enable) Interrupt lines
    EXTI->IMR1 |= (EXTI_IMR1_IM0 | EXTI_IMR1_IM9 | EXTI_IMR1_IM10 | EXTI_IMR1_IM12);

    // Enable channels in NVIC
    NVIC_EnableIRQ(EXTI0_IRQn);       // Private vector for PA0 (Down)
    NVIC_EnableIRQ(EXTI9_5_IRQn);     // Handles PA9 (Back)
    NVIC_EnableIRQ(EXTI15_10_IRQn);   // Handles PA10 (Up) and PA12 (Enter)
}

// Private Vector handler dedicated exclusively to PA0
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

void EXTI9_5_IRQHandler(void) {
    uint32_t current_tick = HAL_GetTick();

    // Check PA9 isolated from any historical snapshot modifications
    if (EXTI->PR1 & EXTI_PR1_PIF9) {
        EXTI->PR1 = EXTI_PR1_PIF9; // Clear flag
        
        if ((current_tick - last_back_tick) > DEBOUNCE_DELAY_MS) {
            last_back_tick = current_tick;
            back_event_handler();
        }
    }
}

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
