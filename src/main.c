#include "stm32l4xx_hal.h"
#include "LCD1602.h"
#include <string.h>

I2C_HandleTypeDef hi2c1;

static void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = 0x10909CEC;   // standard 100kHz timing for 80MHz clock
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}

static void SystemClock_Config(void);
static void MX_GPIO_Init(void);

TIM_HandleTypeDef htim1;
static void MX_TIM1_Init(void)
{
    __HAL_RCC_TIM1_CLK_ENABLE();
    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 79;        // 80MHz / 80 = 1MHz
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = 65535;
    HAL_TIM_Base_Init(&htim1);
}

char get_keypad_input(void)
{

    // Keypad layout

    const char keys[4][3] = {

        {'1', '2', '3'},

        {'4', '5', '6'},

        {'7', '8', '9'},

        {'*', '0', '#'}

    };



    // Row and column pins

    const uint16_t row_pins[4] = {GPIO_PIN_4, GPIO_PIN_0, GPIO_PIN_6, GPIO_PIN_7};

    const uint16_t col_pins[3] = {GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10};



    for (int row = 0; row < 4; row++)

    {

        // Set the current row low and others high

        for (int r = 0; r < 4; r++)

        {

            if (r == row)

                HAL_GPIO_WritePin(GPIOA, row_pins[r], GPIO_PIN_RESET);

            else

                HAL_GPIO_WritePin(GPIOA, row_pins[r], GPIO_PIN_SET);

        }



        // Check each column

        for (int col = 0; col < 3; col++)

        {

            if (HAL_GPIO_ReadPin(GPIOA, col_pins[col]) == GPIO_PIN_RESET)

            {

                // Wait for key release

                while (HAL_GPIO_ReadPin(GPIOA, col_pins[col]) == GPIO_PIN_RESET);

                return keys[row][col];

            }

        }

    }



    return '\0'; // No key pressed

}


char display_buf[17] = {0};   // 16 chars + null terminator for 16x2 LCD
int buf_pos = 0;

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_TIM1_Init();
    HAL_TIM_Base_Start(&htim1); // Start TIM1 for microsecond delay
    lcd_init();

    lcd_put_cur(0, 0);
    lcd_send_string("Enter key:");

    while (1)
    {
        char key = get_keypad_input();
        if (key != '\0')
        {
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5); // Toggle onboard LED for visual feedback

            if (key == '#') // treat '#' as clear
            {
                buf_pos = 0;
                memset(display_buf, 0, sizeof(display_buf));
                lcd_clear();
                lcd_put_cur(0, 0);
                lcd_send_string("Enter key:");
            }
            else if (buf_pos < 16)
            {
                display_buf[buf_pos++] = key;
                lcd_put_cur(1, 0);          // print entered keys on row 2
                lcd_send_string(display_buf);
            }

            HAL_Delay(200); // debounce delay
        }
    }
}




static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    // Matrix keypad pin config (PA4-PA10) (PA0 instead of PA5 for now to test onboard LED)
    GPIO_InitTypeDef GPIO_InitStructA_rows = {0};
    GPIO_InitStructA_rows.Pin   = GPIO_PIN_4| GPIO_PIN_0 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStructA_rows.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructA_rows.Pull  = GPIO_PULLUP;
    GPIO_InitStructA_rows.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStructA_rows);

    GPIO_InitTypeDef GPIO_InitStructA_cols = {0};
    GPIO_InitStructA_cols.Pin   = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStructA_cols.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStructA_cols.Pull  = GPIO_PULLUP;
    GPIO_InitStructA_cols.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStructA_cols);

    GPIO_InitTypeDef GPIO_InitStructA_led = {0};
    GPIO_InitStructA_led.Pin   = GPIO_PIN_5;
    GPIO_InitStructA_led.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructA_led.Pull  = GPIO_NOPULL;
    GPIO_InitStructA_led.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStructA_led);


    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_I2C1_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStructI2C = {0};
    GPIO_InitStructI2C.Pin       = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStructI2C.Mode      = GPIO_MODE_AF_OD;   // open-drain, required for I2C
    GPIO_InitStructI2C.Pull      = GPIO_PULLUP;
    GPIO_InitStructI2C.Speed     = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStructI2C.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructI2C);
}

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.MSIState = RCC_MSI_ON;
    RCC_OscInitStruct.MSICalibrationValue = 0;
    RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6; // ~4MHz
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                 | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_MSI;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0);
}



