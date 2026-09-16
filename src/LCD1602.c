#include "LCD1602.h"

/* ---- I2C handle and PCF8574 backpack address ----
 * Change to 0x3F << 1 if 0x27 doesn't respond. */
#define LCD_I2C_ADDR (0x27 << 1)
extern I2C_HandleTypeDef hi2c1;

/* PCF8574 bit layout: P7 P6 P5 P4 | P3 P2 P1 P0
 *                      D7 D6 D5 D4 | BL EN RW RS   */
#define LCD_BACKLIGHT 0x08

static void lcd_write_byte(uint8_t data)
{
    HAL_I2C_Master_Transmit(&hi2c1, LCD_I2C_ADDR, &data, 1, HAL_MAX_DELAY);
}

static void send_to_lcd(uint8_t nibble, int rs)
{
    uint8_t data = (nibble << 4) | LCD_BACKLIGHT | (rs ? 0x01 : 0x00);

    lcd_write_byte(data | 0x04);  // EN high
    HAL_Delay(1);
    lcd_write_byte(data & ~0x04); // EN low (latches data)
    HAL_Delay(1);
}

void lcd_send_cmd(char cmd)
{
    send_to_lcd((cmd >> 4) & 0x0F, 0);
    send_to_lcd(cmd & 0x0F, 0);
}

void lcd_send_data(char data)
{
    send_to_lcd((data >> 4) & 0x0F, 1);
    send_to_lcd(data & 0x0F, 1);
}

void lcd_send_string(char *str)
{
    while (*str) lcd_send_data(*str++);
}

void lcd_clear(void)
{
    lcd_send_cmd(0x01);
    HAL_Delay(2);
}

void lcd_put_cur(int row, int col)
{
    switch (row)
    {
        case 0: col |= 0x80; break;
        case 1: col |= 0xC0; break;
        default: return;
    }
    lcd_send_cmd((char)col);
}

void lcd_init(void)
{
    HAL_Delay(50);
    send_to_lcd(0x03, 0);
    HAL_Delay(5);
    send_to_lcd(0x03, 0);
    HAL_Delay(1);
    send_to_lcd(0x03, 0);
    HAL_Delay(1);
    send_to_lcd(0x02, 0);   // switch to 4-bit mode
    HAL_Delay(1);

    lcd_send_cmd(0x28);     // 4-bit, 2 lines, 5x8 font
    lcd_send_cmd(0x08);     // display off
    lcd_send_cmd(0x01);     // clear
    HAL_Delay(2);
    lcd_send_cmd(0x06);     // cursor auto-increment
    lcd_send_cmd(0x0C);     // display on, no cursor, no blink
}