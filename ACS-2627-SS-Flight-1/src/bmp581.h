#ifndef BMP581_H
#define BMP581_H

// Includes
#include <Arduino.h>
#include <SPI.h>
#include "spiPlatform.h"
#include "bmp5_api/bmp5.h"


#ifndef BMP581_CHIP_ID
#define BMP581_CHIP_ID 0x50
#endif

// Pin Definitions
#define BMP_CS_1 10  // BMP581 #1 on SPI0 (MOSI: 11, MISO: 12, SCK: 13)
#define BMP_CS_2 14  // BMP581 #2 on SPI1 (MOSI: 26, MISO: 39, SCK: 27)
#define BMP_CS_3 16  // BMP581 #3 on SPI0 (MOSI: 11, MISO: 12, SCK: 13)


// Functions
BMP5_INTF_RET_TYPE platform_writeALT(uint8_t reg_addr, const uint8_t *reg_data, uint32_t length, void *intf_ptr);
BMP5_INTF_RET_TYPE platform_readALT(uint8_t reg_addr, uint8_t *reg_data, uint32_t length, void *intf_ptr);
void platform_delay_us(uint32_t period_us, void *intf_ptr);
bool init_bmp(struct bmp5_dev *dev, struct bmp5_osr_odr_press_config *osr_cfg,
              SpiDeviceHandle *handle, SPIClass *bus, uint8_t cs_pin, const char *label);
void read_bmp(struct bmp5_dev *dev, struct bmp5_osr_odr_press_config *osr_cfg, const char *label);

void setupAltimeter();
void useAltimeter();

#endif // BMP581_H