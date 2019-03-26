/** @file screen.h
 *  @brief Interface for interacting with OLED screen
 *
 *  @author Neil Ryan <nryan@andrew.cmu.edu>
 *  @date 2/21/2018
 */

#ifndef _SCREEN_H_
#define _SCREEN_H_

#include <kstdint.h>

/** Number of rows in the OLED display */
#define OLED_ROWS 32
/** Number of columns in the OLED display */
#define OLED_COLS 128

/** @brief Initialize the OLED screen */
void oled_init(void);

/** @brief Assert the RESET signal for the OLED display
 */
void oled_reset(void);

/** @brief Clear the internal OLED display buffer */
void oled_buf_clr();

/** @brief Set the OLED display to be the contents of the internal buffer */
void oled_buf_draw();

/** @brief Set a pixel of the OLED display in our internal buffer
 *  @param row Row of pixel to set
 *  @param col Column of pixel to set
 */
void oled_buf_pixel_set(uint32_t row, uint32_t col);

/** @brief Clear a pixel of the OLED display in our internal buffer
 *  @param row Row of pixel to cler
 *  @param col Column of pixel to clear
 */
void oled_buf_pixel_clr(uint32_t row, uint32_t col);

/** @brief Clear all pixels from the OLED screen */
void oled_clear_screen(void);

#endif /* _SCREEN_H_ */
