#ifndef SRC_UART_H
#define SRC_UART_H

#define RX_BFR_SIZE 64
#define BAUD_RATE 115200
#define OPERATING_FREQUENCY 80000000

void uart_init(void);
void uart_sendchar (char c);
void uart_sendstring (char *str);
void uart_receive_and_process(void);

#endif // SRC_UART_H
