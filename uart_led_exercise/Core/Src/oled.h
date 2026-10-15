#ifndef SRC_OLED_H_
#define SRC_OLED_H_

#include <stdbool.h>

enum led_pattern
{
    FAST_BLINK,
    SLOW_BLINK,
    BREATHING
};

void oled_send_cmd(uint8_t cmd);
void oled_send_data(uint8_t data);
void oled_clear(void);
void oled_draw_char(char c, uint8_t page, uint8_t column, bool invert);
void oled_print_str(char *str, uint8_t page);
void oled_print_str_highlight(char *str, uint8_t page);
void print_setting_oled (int led_pattern);

#endif // SRC_OLED_H_
