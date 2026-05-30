#ifndef ST7567_H
#define ST7567_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void lcd_init(void);
void lcd_clear(void);
//void lcd_cmd(uint8_t cmd);
//void lcd_data(uint8_t data);
//void lcd_set_pos(uint8_t x, uint8_t page);
void lcd_char(uint8_t x, uint8_t page, char c);
void lcd_string(uint8_t x, uint8_t page, const char *str);

#ifdef __cplusplus
}
#endif

#endif
