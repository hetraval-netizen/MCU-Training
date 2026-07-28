#ifndef SRC_BTN_H_
#define SRC_BTN_H_

/* Debounce interval in milliseconds */
#define DEBOUNCE_DELAY_MS 20

typedef struct {
    bool back_event;
    bool up_event;
    bool down_event;
    bool enter_event;
} button_status_t;

void btn_init(void);
void button_process_events(void);

#endif // SRC_BTN_H_
