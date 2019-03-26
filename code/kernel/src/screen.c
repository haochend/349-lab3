/** @file screen.c
 *
 *  @brief Implementation of function to interact with OLED screen
 *
 *  Most functions interact with a Solomon SSD1306 OLED/PLED Controller.
 *  The datasheet for that device is much more helpful for comprehension
 *  than the OLED display datasheet
 *
 *  @date  2/21/2018
 *  @author Neil Ryan
 */

#include <gpio.h>
#include <panic.h>
#include "screen.h"
#include "spi.h"

/** Highest row we can index */
#define OLED_MAX_ROW (OLED_ROWS-1)
/** Highest column we can index */
#define OLED_MAX_COL (OLED_COLS-1)
/** Number of pixels that each buffer element represents */
#define OLED_CELL_SIZE (8)
/** Size of the OLED internal buffer */
#define OLED_BUF_SIZE ((OLED_ROWS * OLED_COLS) / (OLED_CELL_SIZE))
/** Addressing Page End Address */
#define OLED_MAX_PAGE 0x03

/** Command to set the charge pump */
#define SSD1306B_DCDC_CONFIG_PREFIX_8D          (0x8D)
/** Set the charge pump to 7.5V */
#define SSD1306B_DCDC_CONFIG_7p5v_14            (0x14)
/** Command to turn the OLED display OFF */
#define SSD1306B_DISPLAY_OFF_YES_SLEEP_AE       (0xAE)
/** Command to turn the OLED display ON */
#define SSD1306B_DISPLAY_ON_NO_SLEEP_AF         (0xAF)
/** Command to set display clock divide ratio and oscillator frequency */
#define SSD1306B_CLOCK_DIVIDE_PREFIX_D5         (0xD5)
/** Clock divide ratio = 1, default oscillator frequency */
#define NO_CLK_DIV_DEFAULT_OSC_FREQ             (0x80)
/** Command to set the multiplex ratio */
#define SSD1306B_MULTIPLEX_RATIO_PREFIX_A8      (0xA8)
/** Multiplex ratio #31 */
#define MULTIPLEX_RATIO_VALUE                   (0x1F)
/** Command to set the COM display vertical shift */
#define SSD1306B_DISPLAY_OFFSET_PREFIX_D3       (0xD3)
/** Command to set the RAM display start line to 0 */
#define SSD1306B_DISPLAY_START_LINE_40          (0x40)
/** Command to map column 127 to SEG0 */
#define SSD1306B_SEG0_IS_COL_127_A1             (0xA1)
/** Command to set COM output scan direction to COM0->COM63 */
#define SSD1306B_SCAN_DIR_DOWN_C8               (0xC8)
/** Command to set COM pins hardware configuration */
#define SSD1306B_COM_CONFIG_PREFIX_DA           (0xDA)
/** Scan from COM0 to COM63, sequential pin config, enable L/R remap */
#define SSD1306B_COM_CONFIG_SEQUENTIAL_LEFT_02  (0x02)
/** Command to set contrast control for BANK0 */
#define SSD1306B_CONTRAST_PREFIX_81             (0x81)
/** Contrast setting for the display (higher == more segment current) */
#define BANK0_CONTRAST_SETTING                  (0x8F)
/** Command to set the duration of the pre-charge period */
#define SSD1306B_PRECHARGE_PERIOD_PREFIX_D9     (0xD9)
/** Precharge period should be 241 DCLK periods */
#define PRECHARGE_PERIOD_VALUE                  (0xF1)
/** Comamnd to adjust Vcomh regulator output */
#define SSD1306B_VCOMH_DESELECT_PREFIX_DB       (0xDB)
/** Set Vcomh deselect level to ~.65 * VCC */
#define SSD1306B_VCOMH_DESELECT_0p65xVCC_00     (0x00)
/** Set Vcomh deselect level to ~.71 * VCC */
#define SSD1306B_VCOMH_DESELECT_0p71xVCC_10     (0x10)
/** Set Vcomh deselect level to ~.77 * VCC */
#define SSD1306B_VCOMH_DESELECT_0p77xVCC_20     (0x20)
/** Set Vcomh deselect level to ~.83 * VCC */
#define SSD1306B_VCOMH_DESELECT_0p83xVCC_30     (0x30)
/** Command to enable display outputs */
#define SSD1306B_ENTIRE_DISPLAY_NORMAL_A4       (0xA4)
/** Command to set the display to normal (1 == ON) */
#define SSD1306B_INVERSION_NORMAL_A6            (0xA6)
/** Command to set the memory addressing mode of the SSD1306 */
#define SSD1306B_SET_MEMORY_ADDRESS_MODE        (0x20)
/** Address along pages, column 0 to column 127 */
#define SSD1306B_MEMORY_ADDRESS_MODE_HORIZONTAL (0x00)
/** Command to set column starting and ending addresses of display RAM */
#define SSD1306B_SET_COLUMN_ADDRESS             (0x21)
/** Command to set page starting and ending addresses of display RAM */
#define SSD1306B_SET_PAGE_ADDRESS               (0x22)

/** @brief Internal buffer to hold OLED display state */
static uint8_t _oled_frame_buffer[OLED_BUF_SIZE];

/** @brief Spin wait for a given number of CPU cycles
 *  @param twait Number of cycles to delay for
 */
static void delay(uint32_t twait) {
    while (twait--) { asm("mov r0, r0");}
}

/** @brief Write a command out to the OLED display
 *  @param command Command to write to OLED display
 */
static void oled_write_command(unsigned char command) {
    gpio_clr(MISO);
    delay(1000);
    spi_begin(0, SPI_CLK_DIV_64);
    spi_transfer(command);
    spi_end();
}

/** @brief Start a sequence of transfers to the OLED screen */
static void oled_start_sequence(void) {
    // Set the range of column addresses [0-OLED_MAX_COL]
    oled_write_command(SSD1306B_SET_COLUMN_ADDRESS);
    oled_write_command(0);
    oled_write_command(OLED_MAX_COL);
    // Set the range of pages [0-OLED_MAX_PAGE]
    oled_write_command(SSD1306B_SET_PAGE_ADDRESS);
    oled_write_command(0);
    oled_write_command(OLED_MAX_PAGE);
}

void oled_buf_pixel_set(uint32_t row, uint32_t col) {
    int offset, index;
    if (col >= OLED_COLS || row >= OLED_ROWS) { panic();}
    col = OLED_MAX_COL - col;
    row = OLED_MAX_ROW - row;
    index = col + (row / OLED_CELL_SIZE) * OLED_COLS;
    offset = row % OLED_CELL_SIZE;
    _oled_frame_buffer[index] |= 1 << offset;
}

void oled_buf_pixel_clr(uint32_t row, uint32_t col ) {
    int offset, index;
    if (col >= OLED_COLS || row >= OLED_ROWS) { panic();}
    col = OLED_MAX_COL - col;
    row = OLED_MAX_ROW - row;
    index = col + (row / OLED_CELL_SIZE) * OLED_COLS;
    offset = row % OLED_CELL_SIZE;
    _oled_frame_buffer[index] &= ~(1 << offset);
}


void oled_buf_clr() {
    int i;
    for (i = 0; i < OLED_COLS; i++) { _oled_frame_buffer[i] = 0;}
}

void oled_buf_draw() {
    int index;

    oled_start_sequence();
    gpio_set(MISO);
    spi_begin(0, SPI_CLK_DIV_32);

    for (index = 0; index < OLED_BUF_SIZE; index++) {
        spi_transfer(_oled_frame_buffer[index]);  // black pixels
    }

    spi_end();
}


void oled_reset(void) {
    gpio_config(RESET, GPIO_FUN_OUTPUT);
    gpio_set(RESET);  // should already be high, but just in case
    delay(100000);
    gpio_clr(RESET);
    delay(50000);
    gpio_set(RESET);
}

void oled_clear_screen(void) {
    int i;
    oled_start_sequence();
    gpio_set(MISO);
    spi_begin(0, SPI_CLK_DIV_32);

    for (i = 0; i < OLED_BUF_SIZE; i++) { spi_transfer(0x00);} // Black
    spi_end();
}

void oled_init(void) {
    oled_reset();
    gpio_config(RESET, GPIO_FUN_OUTPUT);
    gpio_config(MISO, GPIO_FUN_OUTPUT);
    gpio_set(RESET);
    gpio_clr(MISO);
    delay(10000);

    spi_master_init(SPI_MODE0, SPI_CLK_DIV_32);
    // This register dump matches the data sheet (CFAL12832D-B p16)
    // and a Linux driver SPI capture at startup

    // Set display off
    oled_write_command(SSD1306B_DISPLAY_OFF_YES_SLEEP_AE);
    // Set Display Clock Divide Ratio/Oscillator Frequency
    oled_write_command(SSD1306B_CLOCK_DIVIDE_PREFIX_D5);
    oled_write_command(NO_CLK_DIV_DEFAULT_OSC_FREQ);
    // Set Multiplex ratio
    oled_write_command(SSD1306B_MULTIPLEX_RATIO_PREFIX_A8);
    oled_write_command(MULTIPLEX_RATIO_VALUE);
    // Set Display Offset
    oled_write_command(SSD1306B_DISPLAY_OFFSET_PREFIX_D3);
    oled_write_command(0x00);
    // Set Display Start Line
    oled_write_command(SSD1306B_DISPLAY_START_LINE_40);
    // Set Charge Pump
    oled_write_command(SSD1306B_DCDC_CONFIG_PREFIX_8D);
    oled_write_command(SSD1306B_DCDC_CONFIG_7p5v_14);
    // Set memory addressing mode to horizontal
    oled_write_command(SSD1306B_SET_MEMORY_ADDRESS_MODE);
    oled_write_command(SSD1306B_MEMORY_ADDRESS_MODE_HORIZONTAL);
    // Set Segment Re-map
    oled_write_command(SSD1306B_SEG0_IS_COL_127_A1);
    // Set COM Ouptut Scan Direction
    oled_write_command(SSD1306B_SCAN_DIR_DOWN_C8);
    // Set COM Pins Hardware Configuration
    oled_write_command(SSD1306B_COM_CONFIG_PREFIX_DA);
    oled_write_command(SSD1306B_COM_CONFIG_SEQUENTIAL_LEFT_02);
    // Set Contrast Control
    oled_write_command(SSD1306B_CONTRAST_PREFIX_81);
    oled_write_command(BANK0_CONTRAST_SETTING);
    // Set Pre-Charge Period
    oled_write_command(SSD1306B_PRECHARGE_PERIOD_PREFIX_D9);
    oled_write_command(PRECHARGE_PERIOD_VALUE);
    // Set VCOMH Deselect Level
    oled_write_command(SSD1306B_VCOMH_DESELECT_PREFIX_DB);
    oled_write_command(0x40); // TODO should be 0x30 ??
    // Set Entire Display On/Off
    oled_write_command(SSD1306B_ENTIRE_DISPLAY_NORMAL_A4);
    // Set Normal/Inverse Display
    oled_write_command(SSD1306B_INVERSION_NORMAL_A6);
    // Set Display On
    oled_write_command(SSD1306B_DISPLAY_ON_NO_SLEEP_AF);
}
