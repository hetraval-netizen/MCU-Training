#ifndef SRC_CONSOLE_H_
#define SRC_CONSOLE_H_

void console_init(void);
void uart_sendchar (char c);
void uart_sendstring (char *str);

#endif /* SRC_CONSOLE_H_ */
