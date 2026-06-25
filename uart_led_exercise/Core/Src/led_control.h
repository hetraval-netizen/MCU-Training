#ifndef SRC_LED_CONTROL_H_
#define SRC_LED_CONTROL_H_

void fast_blink(void);
void slow_blink(void);
void pwm_start(int duty_cycle);
void pwm_animation(void);
void count(int target);
void process_error_led(void);
void trigger_error(void);

#endif /* SRC_LED_CONTROL_H_ */