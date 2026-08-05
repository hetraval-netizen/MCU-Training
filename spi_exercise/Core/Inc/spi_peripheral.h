#ifndef INC_SPI_PERIPHERAL_H_
#define INC_SPI_PERIPHERAL_H_

#include <stdint.h>

void spi_init(void);
uint8_t spi_transfer (uint8_t data);

#endif // INC_SPI_PERIPHERAL_H_