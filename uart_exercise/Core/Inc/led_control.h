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
    LED_MODE_COUNT         /**< Fixed flashing run loop sequence that auto-terminates */
} Led_Mode_t;

void led_init(void);
void reset_error_led(void);
void trigger_error(void);
void led_set_mode(Led_Mode_t mode, int param);

#endif /* SRC_LED_CONTROL_H_ */