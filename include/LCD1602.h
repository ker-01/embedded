#ifndef INC_LCD1602_H_
#define INC_LCD1602_H_

#include "stm32l4xx_hal.h"
#include <stdint.h>

void lcd_init(void);
void lcd_send_cmd(char cmd);
void lcd_send_data(char data);
void lcd_send_string(char *str);
void lcd_put_cur(int row, int col);
void lcd_clear(void);
void delay_us(uint16_t us);

#endif /* INC_LCD1602_H_ */