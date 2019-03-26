/**
 * @file   spi.h
 *
 * @brief  SPI interface with rpi 2
 *
 * @date   July 25th 2015
 * @author Aaron Reyes <areyes@andrew.cmu.edu>
 */

#ifndef _SPI_H_
#define _SPI_H_

#include <kstdint.h>


/** @brief CPOL = 0, CPHA = 0 */
#define SPI_MODE0 0
/** @brief 7.8125MHz */
#define SPI_CLK_DIV_32    32
/** @brief 3.90625MHz */
#define SPI_CLK_DIV_64    64

/* SPI pins on pi 2 */
/** SPI pins on RPi2 */
/** @brief SPI chip enable 1 */
#define CE1_N 7
/** @brief SPI chip enable 0 */
#define CE0_N 8
/** @brief SPI MISO */
#define MISO  9
/** @brief SPI MOSI */
#define MOSI  10
/** @brief SPI clock */
#define SCLK  11
/** @brief SPI reset */
#define RESET 16

/**
 * @brief initializes SPI given the mode and clock divider
 *
 * @param mode the mode (SPI_MODE0, ...1, ...2, ...3)
 * @param clk the clock rate (SPI_CLK_DIV_...)
 */
void spi_master_init(uint32_t mode, uint32_t clk);

/**
 * @brief begin an SPI transaction. should be called before
 *        spi_master_transfer()
 */
void spi_begin(uint8_t cmdMode, uint32_t clk);

/**
 * @brief end an SPI transaction. should be called after all
 *        spi_master_transfer() calls have finished.
 */
void spi_end(void);

/**
 * @brief transfers/receives data using SPI
 *
 * @param data the byte to send
 * @return the byte received
 */
uint8_t spi_transfer(uint8_t data);

#endif /* _SPI_H_ */
