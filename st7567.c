#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include "st7567.h"
#include "font6x8.h"

#define FONT_WIDTH 6
#define FONT_SPACING 1
/* -------- PIN CONFIG -------- */
#define LCD_PORT PORTC
#define LCD_DDR  DDRC

#define LCD_RST  PC0
#define LCD_RS   PC1
#define LCD_CS   PC2
#define LCD_SDA  PC3
#define LCD_SCK  PC5

/* -------- PIN MACROS -------- */
#define RS_HIGH()   (LCD_PORT |=  (1 << LCD_RS))
#define RS_LOW()    (LCD_PORT &= ~(1 << LCD_RS))
#define CS_HIGH()   (LCD_PORT |=  (1 << LCD_CS))
#define CS_LOW()    (LCD_PORT &= ~(1 << LCD_CS))
#define SDA_HIGH()  (LCD_PORT |=  (1 << LCD_SDA))
#define SDA_LOW()   (LCD_PORT &= ~(1 << LCD_SDA))
#define SCK_HIGH()  (LCD_PORT |=  (1 << LCD_SCK))
#define SCK_LOW()   (LCD_PORT &= ~(1 << LCD_SCK))
#define RST_HIGH()  (LCD_PORT |=  (1 << LCD_RST))
#define RST_LOW()   (LCD_PORT &= ~(1 << LCD_RST))

/* -------- LOW LEVEL -------- */
static void lcd_write(uint8_t val, uint8_t isData)
{
    if (isData) RS_HIGH(); else RS_LOW();

    CS_LOW();
    for (uint8_t i = 0; i < 8; i++)
    {
        SCK_LOW();
        if (val & 0x80) SDA_HIGH(); else SDA_LOW();
        val <<= 1;
        SCK_HIGH();
    }
    CS_HIGH();
}

static void lcd_cmd(uint8_t cmd)   { lcd_write(cmd, 0); }
static void lcd_data(uint8_t data){ lcd_write(data, 1); }

/* -------- POSITION -------- */
static void lcd_set_pos(uint8_t x, uint8_t page)
{
    lcd_cmd(0xB0 | page);
    lcd_cmd(0x10 | (x >> 4));
    lcd_cmd(x & 0x0F);
}

/* -------- INIT -------- */
void lcd_init(void)
{
    LCD_DDR |= (1<<LCD_RST)|(1<<LCD_RS)|(1<<LCD_CS)|(1<<LCD_SDA)|(1<<LCD_SCK);

    RST_HIGH(); _delay_ms(10);
    RST_LOW();  _delay_ms(10);
    RST_HIGH(); _delay_ms(10);

    lcd_cmd(0xE2);
    lcd_cmd(0x2C);
    lcd_cmd(0x2E);
    lcd_cmd(0x2F);
    lcd_cmd(0xF8);
    lcd_cmd(0x00);
    lcd_cmd(0x25);
    lcd_cmd(0x81); 
    lcd_cmd(0x20);
    lcd_cmd(0xA2);
    lcd_cmd(0xC8);
    lcd_cmd(0xA0);
    lcd_cmd(0xA6);
    lcd_cmd(0xAF);
}

/* -------- CLEAR -------- */
void lcd_clear(void)
{
    for (uint8_t page = 0; page < 4; page++)
    {
        lcd_set_pos(0, page);
        for (uint8_t col = 0; col < 132; col++)
            lcd_data(0x00);
    }
}

/* -------- TEST -------- */
void lcd_test_fill(void)
{
    for (uint8_t page = 0; page < 4; page++)
    {
        lcd_set_pos(0, page);
        for (uint8_t col = 0; col < 132; col++)
            lcd_data(0xFF);
    }
}


/* -------- DRAW ONE CHARACTER -------- */
void lcd_char(uint8_t x, uint8_t page, char c)
{
    if (c < 32 || c > 127) c = ' ';

    lcd_set_pos(x, page);
   for (uint8_t i = 0; i < 6; i++)
    lcd_data(Font6x8[c - 32][i]);

lcd_data(0x00);   // spacing column

}


/* -------- DRAW STRING -------- */
void lcd_string(uint8_t x, uint8_t page, const char *s)
{
    while (*s)
    {
        if (x > 125) break;
        lcd_char(x, page, *s++);
        x += 7;
    }
}



