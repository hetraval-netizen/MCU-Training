#ifndef INC_I2C_PERIPHERAL_H_
#define INC_I2C_PERIPHERAL_H_

#include "main.h"
void i2c_hardware_init(void);
int8_t i2c_master_transmit(uint8_t dev_addr, uint8_t *payload, uint16_t size);

#endif // INC_I2C_PERIPHERAL_H_
