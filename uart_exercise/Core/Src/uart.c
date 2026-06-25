#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include "main.h"
#include "uart.h"
#include "console.h"

#define OPERATING_FREQUENCY 80000000
#define BAUD_RATE 115200

void uart_init(void){
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN; // Enable clock for USART2

    // Set alternate function for PA2 (Pin 2)
    GPIOA->MODER &= ~GPIO_MODER_MODE2;      // Clear bits for Pin 2
    GPIOA->MODER |= GPIO_MODER_MODE2_1;     // Set to Alternate Function (10)

    // Set alternate function for PA3 (Pin 3)
    GPIOA->MODER &= ~GPIO_MODER_MODE3;      // Clear bits for Pin 3
    GPIOA->MODER |= GPIO_MODER_MODE3_1;     // Set to Alternate Function (10)

    // --- Set Alternate function register to UART TX (PA2 -> AF7) ---
    // Clear the 4 bits for Pin 2 (AFSEL2)
    GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL2; 
    // Shift the number '7' into the Pin 2 position
    GPIOA->AFR[0] |= (7UL << GPIO_AFRL_AFSEL2_Pos); 

    // --- Set Alternate function register to UART RX (PA3 -> AF7) ---
    // Clear the 4 bits for Pin 3 (AFSEL3)
    GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL3; 
    // Shift the number '7' into the Pin 3 position
    GPIOA->AFR[0] |= (7UL << GPIO_AFRL_AFSEL3_Pos);

    // UART Hardware Configuration
    USART2->CR1 = 0;
    USART2->BRR = OPERATING_FREQUENCY / BAUD_RATE;

    USART2->CR1 |= USART_CR1_RXNEIE; // Enable RXNE interrupt
    USART2->CR1 |= USART_CR1_TE;     // Enable transmitter
    USART2->CR1 |= USART_CR1_RE;     // Enable receiver

    NVIC_EnableIRQ(USART2_IRQn);

    USART2->CR1 |= USART_CR1_UE;     // Enable USART2
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

#define RX_BFR_SIZE 64
volatile char rx_buffer[RX_BFR_SIZE];
volatile int head = 0;
volatile int tail = 0;

char cmd_buffer[RX_BFR_SIZE];
int cmd_index = 0;
bool cmd_ready = false;

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

void uart_receive_and_process(void) {
    // Check if there is data in the circular buffer
    if (head != tail) {
        char current_char = rx_buffer[tail];
        tail = (tail + 1) % RX_BFR_SIZE; // Move tail index

        uart_sendchar(current_char); // Echo back the received character

        if (current_char == '\r') {
            uart_sendstring("\r\n"); // Move to new line after Enter key
            cmd_buffer[cmd_index] = '\0'; // Null-terminate the command string
            parse_command(cmd_buffer);
            uart_sendstring("\r> "); // Print a fresh prompt
            cmd_index = 0;
        }
        else if (current_char == '\b') {
            if (cmd_index > 0) {
                cmd_index--;
                uart_sendstring(" \b");
            }
            else {
                uart_sendchar('\a'); // Beep if backspace is pressed at the start
            }
        }
        else {
            // Prevent buffer overflow if user types too many characters without pressing Enter
            if (cmd_index < (RX_BFR_SIZE - 1)) {
                cmd_buffer[cmd_index] = current_char; // Store character
                cmd_index++;
            }
            else {
                uart_sendchar('\a'); // Beep indicating buffer full
            }
        }
    }
}