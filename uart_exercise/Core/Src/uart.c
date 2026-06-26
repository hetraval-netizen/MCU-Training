#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include "main.h"
#include "uart.h"
#include "cmd.h"

/* --- Buffer and Flag Management --- */
char cmd_buffer[RX_BFR_SIZE];             // Array storing the assembled string
volatile int cmd_index = 0;               // Track length (volatile: modified inside ISR)
volatile bool cmd_ready = false;          // Flag set when '\r' is detected inside ISR

/**
 * @brief  Initializes USART2 peripheral and its associated GPIO pins.
 */
void uart_init(void){
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN; 

    GPIOA->MODER &= ~GPIO_MODER_MODE2;      
    GPIOA->MODER |= GPIO_MODER_MODE2_1;     

    GPIOA->MODER &= ~GPIO_MODER_MODE3;      
    GPIOA->MODER |= GPIO_MODER_MODE3_1;     

    GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL2; 
    GPIOA->AFR[0] |= (7UL << GPIO_AFRL_AFSEL2_Pos); 

    GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL3; 
    GPIOA->AFR[0] |= (7UL << GPIO_AFRL_AFSEL3_Pos);

    USART2->CR1 = 0;
    USART2->BRR = OPERATING_FREQUENCY / BAUD_RATE;

    USART2->CR1 |= USART_CR1_RXNEIE; 
    USART2->CR1 |= USART_CR1_TE;     
    USART2->CR1 |= USART_CR1_RE;     

    NVIC_EnableIRQ(USART2_IRQn);

    USART2->CR1 |= USART_CR1_UE;     
}

/**
 * @brief  Transmits a single character over UART2.
 */
void uart_sendchar (char c) {
    while (!(USART2->ISR & (1UL << 7))) {
    }
    USART2->TDR = c;
}

/**
 * @brief  Transmits a null-terminated string over UART2.
 */
void uart_sendstring (char *str) {
    while (*str != '\0'){
        uart_sendchar(*str);
        str++;
    }
}

/**
 * @brief  USART2 Global Interrupt Handler.
 * @details Extracts single bytes from the hardware, builds the command string,
 *          handles live echo/editing, and flags when a command is complete.
 */
void USART2_IRQHandler (void) {
    /* Check if character received flag (RXNE) is raised */
    if (USART2->ISR & USART_ISR_RXNE) {
        char current_char = (char)USART2->RDR; // Read byte

        /* Block new inputs if the previous command is still pending processing */
        if (!cmd_ready) {
            
            uart_sendchar(current_char);

            /* Case 1: Carriage Return (Enter Key) - Signal string is ready */
            if (current_char == '\r') {
                uart_sendstring("\r\n");
                cmd_buffer[cmd_index] = '\0';
                cmd_ready = true;
            }
            /* Case 2: Backspace Key - Modify index and clean console */
            else if (current_char == '\b') {
                if (cmd_index > 0) {
                    cmd_index--;
                    uart_sendstring(" \b");
                } else {
                    uart_sendchar('\a');
                }
            }
            /* Case 3: Character Storage with Overflow Shield */
            else {
                if (cmd_index < (RX_BFR_SIZE - 1)) {
                    cmd_buffer[cmd_index] = current_char;
                    cmd_index++;
                } else {
                    uart_sendchar('\a');
                }
            }
        }
    }

    /* Clear fault statuses to keep interrupt pipeline healthy */
    if (USART2->ISR & 0x0F) {
        USART2->ICR = 0x0F; 
    }
}

/**
 * @brief  Polling function deployed from the main while loop.
 */
void uart_receive_and_process(void) {
    /* Act only when the Interrupt Handler signals string completion */
    if (cmd_ready) {
        
        /* Deliver string directly to application logic parser */
        parse_command(cmd_buffer);

        /* Refresh command state to capture the next input session */
        cmd_index = 0;
        uart_sendstring("\r> ");
        cmd_ready = false;
    }
}
