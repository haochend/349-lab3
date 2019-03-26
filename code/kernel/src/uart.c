/**
 * @file   uart.c
 *
 * @brief  lower level hardware interactions for uart on pi
 *
 * @date   02.17.2018
 * @author yanyingz
 */
#include <uart.h>
#include <kstdint.h>
#include <gpio.h>
#include <BCM2836.h>

/**@brief  Enable register*/
#define AUXENB_REG (volatile uint32_t *)(MMIO_BASE_PHYSICAL + 0x215004)
/**@brief  IER register*/
#define AUX_MU_IER_REG (volatile uint32_t *)(MMIO_BASE_PHYSICAL + 0x215044)
/**@brief  IIR register*/
#define AUX_MU_IIR_REG (volatile uint32_t *)(MMIO_BASE_PHYSICAL + 0x215048)
/**@brief  LCR register*/
#define AUX_MU_LCR_REG (volatile uint32_t *)(MMIO_BASE_PHYSICAL + 0x21504C)
/**@brief  LSR register*/
#define AUX_MU_LSR_REG (volatile uint32_t *)(MMIO_BASE_PHYSICAL + 0x215054)
/**@brief  IO register*/
#define AUX_IO_REG (volatile uint32_t *)(MMIO_BASE_PHYSICAL + 0x215040)
/**@brief  BAUD RATE register*/
#define AUX_MU_BAUD_REG (volatile uint32_t *)(MMIO_BASE_PHYSICAL + 0x215068)


/** @brief GPIO UART RX pin */
#define RX_PIN 15
/** @brief GPIO UART TX pin */
#define TX_PIN 14

void uart_init(void) {

  //The AUXENB register is used to enable access to the MMIO peripherals of UART
  *AUXENB_REG |= 0x01;
  
  // configure GPIO pullups
  gpio_set_pull(RX_PIN, GPIO_PULL_DISABLE);
  gpio_set_pull(TX_PIN, GPIO_PULL_DISABLE);
  // set GPIO pins to correct function on pg 102 of BCM2835 peripherals
  gpio_config(RX_PIN, GPIO_FUN_ALT5);
  gpio_config(TX_PIN, GPIO_FUN_ALT5);

  //The AUX MU IER REG register should be set to 0
  *AUX_MU_IER_REG &= 0;
  //In the AUX MU IIR REG register, you only care about the bits pertaining to clearing the FIFOs.
  *AUX_MU_IIR_REG |= 0x6;
  //Do not set DLAP access inside of the AUX MU LCR REG register.
  *AUX_MU_LCR_REG |= 0x3;
  // The AUX MU BAUD register is where you should put your baud value after 
  // solving the equation on page 11 for baudrate reg.
  *AUX_MU_BAUD_REG = 270;
}


void uart_close(void) {
  *AUXENB_REG &= 0xfffffffe;
}


void uart_put_byte(uint8_t byte) {
  while (!((*AUX_MU_LSR_REG >> 5) & 1));
  *AUX_IO_REG = (uint32_t) byte;
}


uint8_t uart_get_byte(void) {
  while (!(*AUX_MU_LSR_REG & 1));
  uint32_t i = (*AUX_IO_REG);
  return (uint8_t) (i &= 0x000000ff);
}
