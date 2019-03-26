/**
 * @file   i2c.c
 *
 * @brief  I2C implementation on rpi 2
 *
 * @date   02.18.2017
 * @author yanyingz
 */

#include <kstdint.h>
#include <i2c.h>
#include <gpio.h>
#include <BCM2836.h>

/**@brief gpio pin number for SDA*/
#define IRC1_SDA	2
/**@brief gpio pin number for SCL*/
#define IRC1_SCL	3

/**@brief CONTROL register*/
#define BSC1_C (volatile uint32_t *)(MMIO_BASE_PHYSICAL + 0x804000)
/**@brief STATUS register*/
#define BSC1_S (volatile uint32_t *)(MMIO_BASE_PHYSICAL + 0x804004)
/**@brief DATA LENGTH register*/
#define BSC1_DLEN (volatile uint32_t *)(MMIO_BASE_PHYSICAL + 0x804008)
/**@brief ADDRESS register*/
#define BSC1_A (volatile uint32_t *)(MMIO_BASE_PHYSICAL + 0x80400c)
/**@brief FIFO register*/
#define BSC1_FIFO (volatile uint32_t *)(MMIO_BASE_PHYSICAL + 0x804010)
/**@brief CLK DIV register*/
#define BSC1_DIV (volatile uint32_t *)(MMIO_BASE_PHYSICAL + 0x804014)

void i2c_master_init(uint16_t clk) {
  //configure GPIO pullups
  gpio_set_pull(IRC1_SDA, GPIO_PULL_DISABLE);
  gpio_set_pull(IRC1_SCL, GPIO_PULL_DISABLE);  
  //set GPIO pins to correct function on peripherals
  gpio_config(IRC1_SDA, GPIO_FUN_ALT0);
  gpio_config(IRC1_SCL, GPIO_FUN_ALT0);

  //put the clock speed into the CDIV reg
  *BSC1_DIV = clk;
  //enable I2C, clear FIFO
  *BSC1_C |= 0x8030;
}

uint8_t i2c_master_write(uint8_t *buf, uint16_t len, uint8_t addr) {
  //length check
  if (!(len <= 16 && len >=0)) return -1;
  //set data length to DLEN
  *BSC1_DLEN = len;
  //set addr to slave address ADDR
  *BSC1_A = addr;
  //clear FIFO
  *BSC1_C |= 0x30;
  //put data into FIFO reg
  int i;
  for (i = 0; i < len; i++) *BSC1_FIFO = buf[i];
  //configure control reg to send package
  *BSC1_C &= 0xfffffffe;
  *BSC1_C |= 0x80;
  //wait until the transfer is done
  while (!((*BSC1_S >> 1) & 1));

  unsigned x;
  x = *(BSC1_S);
  *(BSC1_S) = x;

  return 0;

}

uint8_t i2c_master_read(uint8_t *buf, uint16_t len, uint8_t addr) {
  //length check 
  if (!(len >= 0 && len <= 16)) return -1;
  //set data length to DLEN
  *BSC1_DLEN = len;
  //set addr to slave addr REG
  *BSC1_A = addr;
  //clear FIFO
  //*BSC1_C |= 0x30;
  //configure control reg  to start a read
  *BSC1_C |= 0x01;
  *BSC1_C |= 0x80;
  //wait while in transfer
  while (!((*BSC1_S >>1) & 1));
  //read data from FIFO
  int i;
  for (i = 0; i < len; i++) buf[i] = *BSC1_FIFO;

  unsigned x;
  x = *(BSC1_S);
  *(BSC1_S) = x;

  return 0;
}
