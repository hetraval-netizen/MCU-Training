#ifndef INC_SPI_PERIPHERAL_H_
#define INC_SPI_PERIPHERAL_H_

#include <stdint.h>

/* MPU WHO_AM_I register address */
#define MPU_WHO_AM_I_REG       0x75U

/* Expected device IDs returned by the WHO_AM_I register */
#define MPU9250_CHIP_ID        0x71U
#define MPU6500_CHIP_ID        0x70U

void spi_hardware_init(void);
uint8_t spi_transfer (uint8_t data);

#endif // INC_SPI_PERIPHERAL_H_
