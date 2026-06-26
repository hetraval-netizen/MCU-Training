#ifndef SRC_LED_CONTROL_H_
#define SRC_LED_CONTROL_H_

/**
 * @brief Supported operating status profiles managed by the system state engine.
 */
typedef enum {
    LED_MODE_FAST_BLINK,   /**< Continuous 10Hz square wave cycling pattern */
    LED_MODE_SLOW_BLINK,   /**< Continuous slow square wave cycling pattern */
    LED_MODE_PWM,          /**< Constant solid beam fixed to a precise brightness */
    LED_MODE_BREATHING,    /**< Seamless automatic sinusoidal fading ramp pattern */
    LED_MODE_COUNT,        /**< Fixed flashing run loop sequence that auto-terminates */
	LED_MODE_MAX_CNT
} Led_Mode_t;

#define PRESCALE_CLOCK 7999 // Prescale to 8MHz
#define AUTO_RELOAD_FAST_BLINK 4999
#define DUTY_CYCLE_FAST_BLINK 2500
#define AUTO_RELOAD_SLOW_BLINK 15999
#define DUTY_CYCLE_SLOW_BLINK 8000
#define AUTO_RELOAD_BREATHING 99
#define ERR_LED_INDICATION_TIME 1000 // 1 second
#define MIN_COUNT_LIMIT 0
#define MIN_ANIMATION_LIMIT 0
#define MAX_ANIMATION_LIMIT 100
#define ANIM_DIRECTION_POSITIVE 1
#define ANIM_DIRECTION_NEGATIVE -1

void led_control_init(void);
void set_error_led(bool start);
void led_set_mode(Led_Mode_t mode, int param);

#endif /* SRC_LED_CONTROL_H_ */
