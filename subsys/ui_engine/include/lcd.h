#ifndef LCD_H_
#define LCD_H_

#include <stdint.h>

// Protótipos das funções implementadas no arquivo base do LCD
void lcd_fill_rect(int x, int y, int w, int h, uint16_t color);
void lcd_draw_char(int x, int y, char c, uint16_t color, uint16_t bg, uint8_t size);
void lcd_draw_string(int x, int y, const char *str, uint16_t color, uint16_t bg, uint8_t size);

#endif /* LCD_H_ */