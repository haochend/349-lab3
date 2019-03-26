/**
 * @file   ads1015.c
 *
 * @brief  I2C driver for ads1015
 *
 * @date   02.18.2018
 * @author yanyingz
 */

#include <kstdint.h>
#include <ads1015.h>
#include <i2c.h>

/**@brief slave address*/
#define	SLAVE_ADDR	0x49
/**@brief configeration register*/
#define CONFIG_REG 	1
/**@brief convertion register*/
#define CONV_REG	0
/**@brief deafult value of config register MSB*/
#define DEFAULT_MSB	0x05
/**@brief default value of config register LSB*/
#define DEFAULT_LSB	0x83

void adc_init(void) {
  i2c_master_init(I2C_CLK_100KHZ);
}


uint16_t adc_read(uint8_t channel) {
  uint8_t config_data[3];
  config_data[0] = CONFIG_REG;			//points to config register
  config_data[1] = (DEFAULT_MSB | 0x80);
  config_data[1] = (config_data[1] & 0x8f) | (channel << 4);
  if (channel == 3) config_data[1] &= 0xf1;
  config_data[2] = DEFAULT_LSB;			//default value of LSB of config register

  uint8_t conv_data[1];
  conv_data[0] = CONV_REG;

  //perform a series of reads and writes command
  i2c_master_write(config_data, 3, SLAVE_ADDR);
  i2c_master_write(conv_data, 1, SLAVE_ADDR);
  uint8_t buffer[2];
  i2c_master_read(buffer, 2, SLAVE_ADDR);

  uint16_t result = (buffer[0] << 8) | buffer[1];
  return result;
}
