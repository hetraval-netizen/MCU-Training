#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include "main.h"
#include "console.h"
#include "uart.h"
#include "led_control.h"

#define NUM_COMMANDS 6

typedef void (*cmd_handler_t)(char *args);

typedef struct {
    const char *name;
    cmd_handler_t handler;
    bool is_prefix;
} cmd_entry_t;

void handle_help (char *args);
void handle_fast (char *args);
void handle_slow (char *args);
void handle_breathing (char *args);
void handle_count (char *args);
void handle_led_brightness (char *args);

static const cmd_entry_t cmd_table[] = {
    {"help", handle_help, false},
    {"fast", handle_fast, false},
    {"slow", handle_slow, false},
    {"breathing", handle_breathing, false},
    {"count ", handle_count, true},
    {"led brightness ", handle_led_brightness, true}
};

void parse_command (char *cmd) {
    if (cmd[0] == '\0') {
        return;
    }

    for (int i = 0; i < NUM_COMMANDS; i++) {
        if (cmd_table[i].is_prefix){
            int length = strlen(cmd_table[i].name);
            if (strncmp(cmd, cmd_table[i].name, length) == 0){
                cmd_table[i].handler(cmd + length);
                return;
            }
        }
        else {
            if (strcmp (cmd, cmd_table[i].name) == 0){
                cmd_table[i].handler(NULL);
                return;
            }
        }
    }
    uart_sendstring("Error: Unknown command. Type 'help'.\r\n");
    trigger_error(); 
}

void handle_help (char *args){
    uart_sendstring("\r\nAvailable commands:\r\n"
        "  fast                            - Fast blink\r\n"
        "  slow                            - Slow blink\r\n"
        "  count <Number of Blinks>        - Start counting\r\n"
        "  led brightness <Led Brightness> - Glow led at a specific brightness\r\n"
        "  breathing                       - Start breathing animation\r\n"
        "  help                            - Show this help message\r\n");
}

void handle_fast (char *args){
    led_set_mode(LED_MODE_FAST_BLINK, 0);
}

void handle_slow (char *args){
    led_set_mode(LED_MODE_SLOW_BLINK, 0);
}

void handle_breathing (char *args){
    led_set_mode(LED_MODE_BREATHING, 0);
}

void handle_count (char *args){
    char *endptr;
    long target = strtol(args, &endptr, 10);
    led_set_mode(LED_MODE_COUNT, target);
}

void handle_led_brightness (char *args){
    char *endptr;
    long duty = strtol(args, &endptr, 10);
    led_set_mode(LED_MODE_PWM, duty);
}
