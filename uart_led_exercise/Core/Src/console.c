#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define RX_BFR_SIZE 64
volatile char rx_buffer[RX_BFR_SIZE];
volatile int head = 0;
volatile int tail = 0;

char cmd_buffer[RX_BFR_SIZE];
int cmd_index = 0;
bool cmd_ready = false;


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

void console_init(void) {
    while (1){
        if (head != tail){
            char current_char = rx_buffer[tail];
            tail = (tail + 1) % RX_BFR_SIZE; // Move tail index

            uart_sendchar(current_char); // Echo back the received character

            if (current_char == '\r'){
                uart_sendstring("\r\n"); // Move to new line after Enter key
                cmd_buffer[cmd_index] = '\0'; // Null-terminate the command string
                
                if (strcmp(cmd_buffer, "help") == 0) {
                    uart_sendstring("Available commands: fast, slow, pwm, count, help\r\n");
                } 
                else if (strcmp(cmd_buffer, "fast") == 0) {
                    uart_sendstring("System Indication: Starting Fast Blink...\r\n");
                    // We will trigger the timer here later!
                }
                else if (strcmp(cmd_buffer, "slow") == 0) {
                    uart_sendstring("System Indication: Starting Slow Blink...\r\n");
                    // We will trigger the timer here later!
                }
                else if (cmd_index > 0) { // If user typed something unknown
                    uart_sendstring("Error: Unknown command. Type 'help'.\r\n");
                }

                uart_sendstring("\r> "); // Print a fresh prompt
                cmd_index = 0;

            }
            else if (current_char == '\b'){
                cmd_index--; // Move back the command index for backspace
            }
            else {
                cmd_buffer[cmd_index] = current_char; // Store character in command buffer
                cmd_index++;
            }
        }
    }
}
