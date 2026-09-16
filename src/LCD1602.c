#include "LCD1602.h"

/* ---- Pin mapping: only place to edit if you rewire ---- */
#define RS_PORT GPIOB
#define RS_PIN  GPIO_PIN_0
#define EN_PORT GPIOB
#define EN_PIN  GPIO_PIN_1
#define D4_PORT GPIOB
#define D4_PIN  GPIO_PIN_2
#define D5_PORT GPIOB
#define D5_PIN  GPIO_PIN_3
#define D6_PORT GPIOB
#define D6_PIN  GPIO_PIN_4
#define D7_PORT GPIOB
#define D7_PIN  GPIO_PIN_5

/* ---- Microsecond delay via TIM1 counting at 1 MHz ---- */
extern TIM_HandleTypeDef htim1;

void delay_us(uint16_t us)
{
    __HAL_TIM_SET_COUNTER(&htim1, 0);
    while (__HAL_TIM_GET_COUNTER(&htim1) < us) { }
}

static void send_to_lcd(char data, int rs)
{
    HAL_GPIO_WritePin(RS_PORT, RS_PIN, rs ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(D7_PORT, D7_PIN, ((data >> 3) & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(D6_PORT, D6_PIN, ((data >> 2) & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(D5_PORT, D5_PIN, ((data >> 1) & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(D4_PORT, D4_PIN, ((data >> 0) & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    HAL_GPIO_WritePin(EN_PORT, EN_PIN, GPIO_PIN_SET);
    delay_us(20);
    HAL_GPIO_WritePin(EN_PORT, EN_PIN, GPIO_PIN_RESET);
    delay_us(20);
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
    delay_us(150);
    send_to_lcd(0x03, 0);
    HAL_Delay(10);
    send_to_lcd(0x02, 0);
    HAL_Delay(10);
    lcd_send_cmd(0x28);
    HAL_Delay(1);
    lcd_send_cmd(0x08);
    HAL_Delay(1);
    lcd_send_cmd(0x01);
    HAL_Delay(2);
    lcd_send_cmd(0x06);
    HAL_Delay(1);
    lcd_send_cmd(0x0C);
    HAL_Delay(1);
}