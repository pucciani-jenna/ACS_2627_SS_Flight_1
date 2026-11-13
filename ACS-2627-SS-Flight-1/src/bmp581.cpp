#include "bmp581.h"
#include "data_logger.h"
const float SEA_LEVEL_PRESSURE_PA = 101325.0f;

SPISettings bmpSpiSettings(1000000, MSBFIRST, SPI_MODE0);

// Low-Level Platform Callbacks for Bosch API
BMP5_INTF_RET_TYPE platform_readALT(uint8_t reg_addr, uint8_t *reg_data, uint32_t length, void *intf_ptr) {
  SpiDeviceHandle *dev = static_cast<SpiDeviceHandle *>(intf_ptr);

  dev->bus->beginTransaction(bmpSpiSettings);
  digitalWrite(dev->cs_pin, LOW);

  // BMP581 SPI Read: MSB = 1 (bit 7 set)
  dev->bus->transfer(reg_addr | 0x80);
  for (uint32_t i = 0; i < length; i++) {
    reg_data[i] = dev->bus->transfer(0x00);
  }

  digitalWrite(dev->cs_pin, HIGH);
  dev->bus->endTransaction();
  return BMP5_OK;
}

BMP5_INTF_RET_TYPE platform_writeALT(uint8_t reg_addr, const uint8_t *reg_data, uint32_t length, void *intf_ptr) {
  SpiDeviceHandle *dev = static_cast<SpiDeviceHandle *>(intf_ptr);

  dev->bus->beginTransaction(bmpSpiSettings);
  digitalWrite(dev->cs_pin, LOW);

  // BMP581 SPI Write: MSB = 0 (bit 7 cleared)
  dev->bus->transfer(reg_addr & 0x7F);
  for (uint32_t i = 0; i < length; i++) {
    dev->bus->transfer(reg_data[i]);
  }

  digitalWrite(dev->cs_pin, HIGH);
  dev->bus->endTransaction();
  return BMP5_OK;
}

void platform_delay_us(uint32_t period_us, void *intf_ptr) {
  (void)intf_ptr;
  delayMicroseconds(period_us);
}

// Sensor Contexts (i think i can keep it like this, we'll see)
static struct bmp5_dev dev_ctx1; static SpiDeviceHandle sensor1_handle;
static struct bmp5_dev dev_ctx2; static SpiDeviceHandle sensor2_handle;
static struct bmp5_dev dev_ctx3; static SpiDeviceHandle sensor3_handle;

// Store OSR/ODR configs needed by bmp5_get_sensor_data()
static struct bmp5_osr_odr_press_config osr_cfg1;
static struct bmp5_osr_odr_press_config osr_cfg2;
static struct bmp5_osr_odr_press_config osr_cfg3;

static float calculateAltitudeMeters(float pressurePa) {
  return 44330.0f * (1.0f - powf(pressurePa / SEA_LEVEL_PRESSURE_PA, 0.190295f));
}

bool init_bmp(struct bmp5_dev *dev, struct bmp5_osr_odr_press_config *osr_cfg,
                     SpiDeviceHandle *handle, SPIClass *bus, uint8_t cs_pin, const char *label) {
  pinMode(cs_pin, OUTPUT);
  digitalWrite(cs_pin, HIGH);

  handle->bus = bus;
  handle->cs_pin = cs_pin;

  dev->read = platform_readALT;
  dev->write = platform_writeALT;
  dev->delay_us = platform_delay_us;
  dev->intf = BMP5_SPI_INTF;
  dev->intf_ptr = handle;

  // SPI dummy read to enable 4-wire SPI mode
  uint8_t dummy = 0;
  platform_readALT(0x00, &dummy, 1, handle);
  delay(5);

  int8_t rslt = bmp5_soft_reset(dev);
  if (rslt != BMP5_OK) {
    Serial.printf("[%s] Soft-reset failed.\n", label);
    return false;
  }
  delay(15);

  rslt = bmp5_init(dev);
  Serial.printf("[%s] Chip ID: 0x%02X\n", label, dev->chip_id);

  if (rslt != BMP5_OK || dev->chip_id != BMP581_CHIP_ID) {
    Serial.printf("[%s] Init failed! Expected Chip ID 0x%02X, got 0x%02X\n", 
                  label, BMP581_CHIP_ID, dev->chip_id);
    return false;
  }

  // Configure OSR and ODR settings
  osr_cfg->osr_t = BMP5_OVERSAMPLING_2X;
  osr_cfg->osr_p = BMP5_OVERSAMPLING_16X;
  osr_cfg->press_en = BMP5_ENABLE;
  osr_cfg->odr = BMP5_ODR_50_HZ;

  rslt = bmp5_set_osr_odr_press_config(osr_cfg, dev);
  if (rslt != BMP5_OK) return false;

  // Set continuous normal mode
  rslt = bmp5_set_power_mode(BMP5_POWERMODE_NORMAL, dev);
  return (rslt == BMP5_OK);
}

void read_bmp(
  struct bmp5_dev *dev, 
  struct bmp5_osr_odr_press_config *osr_cfg, 
  const char *label, 
  const char *sensorId) {
  struct bmp5_sensor_data data = {0};

  if (bmp5_get_sensor_data(&data, osr_cfg, dev ) == BMP5_OK) {
    // Bat: 11/13/2026 - This checks whether the data logger is initialized and attempts to log pressure data.
    if (DataLoggerIsInitialized() && !DataLoggerLogPressure(millis(), sensorId, data.pressure, data.temperature)) {
      Serial.printf("[%s] Could not queue SD log row\n", label);
    }

    float altM = calculateAltitudeMeters(data.pressure);
    float altFt = altM * 3.28084f;

    Serial.printf("[%s] P: %7.2f hPa | T: %5.2f C | Alt: %6.2f m (%5.1f ft)\n",
                  label, data.pressure / 100.0f, data.temperature, altM, altFt);
  } else {
    Serial.printf("[%s] Read error!\n", label);
  }
}

void setupAltimeter() {
  // Set CS pins HIGH upfront
  pinMode(BMP_CS_1, OUTPUT); digitalWrite(BMP_CS_1, HIGH);
  pinMode(BMP_CS_2, OUTPUT); digitalWrite(BMP_CS_2, HIGH);
  pinMode(BMP_CS_3, OUTPUT); digitalWrite(BMP_CS_3, HIGH);

  // Sensor bring-up
  init_bmp(&dev_ctx1, &osr_cfg1, &sensor1_handle, &SPI,  BMP_CS_1, "BMP581 #1 (SPI0)");
  init_bmp(&dev_ctx2, &osr_cfg2, &sensor2_handle, &SPI1, BMP_CS_2, "BMP581 #2 (SPI1)");
  init_bmp(&dev_ctx3, &osr_cfg3, &sensor3_handle, &SPI,  BMP_CS_3, "BMP581 #3 (SPI0)");

  Serial.println("BMP581 setup finished.\n");
}

// Bat: 11/13/2026 - Each of the 3 BMP581 sensors has a numbered ID (A1, A2, A3) for logging purposes. 
void useAltimeter() {
  read_bmp(&dev_ctx1, &osr_cfg1, "BMP 1", "A1");
  read_bmp(&dev_ctx2, &osr_cfg2, "BMP 2", "A2");
  read_bmp(&dev_ctx3, &osr_cfg3, "BMP 3", "A3");
  Serial.println("------------------------------------------------------------------");
}