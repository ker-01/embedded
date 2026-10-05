#include "stm32l4xx_hal.h"
#include "LCD1602.h"
#include <string.h>
#include <stdio.h>

I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart2;

volatile uint8_t button0_pressed_flag = 0;
volatile uint8_t button1_pressed_flag = 0;
volatile uint8_t button2_pressed_flag = 0;

volatile uint8_t button0_edge_pending = 0;
volatile uint8_t button1_edge_pending = 0;
volatile uint8_t button2_edge_pending = 0;

volatile uint32_t button0_edge_tick = 0;
volatile uint32_t button1_edge_tick = 0;
volatile uint32_t button2_edge_tick = 0;

static uint32_t last_toggle_tick0 = 0;
static uint32_t last_toggle_tick1 = 0;
static uint32_t last_toggle_tick2 = 0;


const char *valid_ids[] = {
    "22203659",
    "22203660",
    "22210852", 
};

char admin[] = "22203658";

int candidate_index = 0;
int option_index = 0;

const char candidates[] = {'A', 'B', 'C', 'D', 'E', 'F'};

#define NUM_VALID_IDS (sizeof(valid_ids) / sizeof(valid_ids[0]))
#define NUM_CANDIDATES (sizeof(candidates) / sizeof(candidates[0]))
#define NUM_ADMIN_OPTIONS 3

char vote_counts[NUM_CANDIDATES] = {0}; // Array to hold votes for each candidate
char already_voted_IDS[NUM_VALID_IDS][9] = {0}; // Array to hold IDs that have already voted
int number_of_voters = 0; // Counter for the number of voters who have voted

static void MX_USART2_UART_Init(void)
{
    __HAL_RCC_USART2_CLK_ENABLE();

    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    HAL_UART_Init(&huart2);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    switch (GPIO_Pin)
    {
        case GPIO_PIN_4:
            button0_edge_pending = 1;
            button0_edge_tick = HAL_GetTick();
            break;
            
        case GPIO_PIN_1:
            button1_edge_pending = 1;
            button1_edge_tick = HAL_GetTick();
            break;

        case GPIO_PIN_5:
            button2_edge_pending = 1;
            button2_edge_tick = HAL_GetTick();
            break;

 
        default:
            break;
    }

    }


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
    const char keys[4][3] = {
        {'1', '2', '3'},
        {'4', '5', '6'},
        {'7', '8', '9'},
        {'*', '0', '#'}
    };
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

    int check_valid_id(const char *id)
    {
        if (id[0] == '\0') return 0;                       // empty input
        if (strcmp(id, admin) == 0) return 2;

        for (int i = 0; i < number_of_voters; i++)
            if (strcmp(id, already_voted_IDS[i]) == 0) return 3;

        for (size_t i = 0; i < NUM_VALID_IDS; i++)
            if (strcmp(id, valid_ids[i]) == 0) return 1;

        return 0;
    }

static int button0_check(void)
// go up
{
    if (button0_edge_pending)
    
    {
        button0_edge_pending = 0;
     
        if (button0_edge_tick - last_toggle_tick0 > 150) // ignore bounces within 200ms
            {
                button0_pressed_flag += 1;
                last_toggle_tick0 = button0_edge_tick;
                if (candidate_index < NUM_CANDIDATES - 1)
                {
                    candidate_index++;
                }
                else
                {
                    candidate_index = candidate_index; // stay at the last candidate if already at the end
                }
                return 1;
            }

     }
     return 0;
}

static void button0_check_admin(void)
{
    if (button0_edge_pending)
    {
        button0_edge_pending = 0;
        if (button0_edge_tick - last_toggle_tick0 > 150) // ignore bounces within 200ms
        {
            button0_pressed_flag += 1;
            last_toggle_tick0 = button0_edge_tick;
            if (option_index < NUM_ADMIN_OPTIONS - 1)
            {
                option_index++;
            }
            else
            {
                option_index = option_index; // stay at the last option if already at the end
            }
        }
    }
}

static int button1_check(void)
{
    if (button1_edge_pending)
    {
        button1_edge_pending = 0;
        if (button1_edge_tick - last_toggle_tick1 > 150) // ignore bounces within 200ms
        {
            button1_pressed_flag += 1;
            last_toggle_tick1 = button1_edge_tick;
            if (candidate_index > 0)
            {
                candidate_index--;
            }
            else
            {
                candidate_index = candidate_index; // stay at the first candidate if already at the start
            }
            return 1;
        }
    }
    return 0;
}

static void button1_check_admin(void)
{
    if (button1_edge_pending)
    {
        button1_edge_pending = 0;
        if (button1_edge_tick - last_toggle_tick1 > 150) // ignore bounces within 200ms
        {
            button1_pressed_flag += 1;
            last_toggle_tick1 = button1_edge_tick;
            if (option_index >  0) 
            {
                option_index--;
            }
            else
            {
                option_index = option_index; // stay at the first option if already at the start
            }
        }
    }
}

static int button2_check(void)
{
    if (button2_edge_pending)
    {
        button2_edge_pending = 0;
        if (button2_edge_tick - last_toggle_tick2 > 150) // ignore bounces within 200ms
        {
            button2_pressed_flag += 1;
            last_toggle_tick2 = button2_edge_tick;
            // Handle button 2 press event here
            return 1; // Return 1 to indicate button press
        }
    }
    return 0; // Return 0 if no press detected
}


static void show_id_prompt(void)
{
    lcd_clear();
    lcd_put_cur(0, 0);
    lcd_send_string("Enter ID:");
    lcd_put_cur(1, 0);
}

int authenticate_voter(char *id_out)
{
    char id_input[17] = {0};   // 16 chars + null terminator for 16x2 LCD
    int buf_pos = 0;
    char welcome[17];
    snprintf(welcome, sizeof(welcome), "Enter ID:");
    lcd_send_string(welcome);
    lcd_put_cur(1, 0); // Move cursor to second line
    while (1)
    {   

        char key = get_keypad_input();
        if (key != '\0')
        {
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5); // Toggle onboard LED for visual feedback
            if (key == '#') // treat '#' as clear
            {
                buf_pos = 0;
                memset(id_input, 0, sizeof(id_input));
                lcd_clear();
                lcd_put_cur(0, 0);
                show_id_prompt();
                // lcd_send_string("Enter key:");
            }
            else if (key == '*') // treat '*' as enter
            {
                id_input[buf_pos] = '\0'; // Null-terminate the string
                int id_type = check_valid_id(id_input);
                if (id_type == 1)
                {
                    strcpy(id_out, id_input);
                    lcd_clear();
                    lcd_put_cur(0, 0);
                    lcd_send_string("Access Granted");
                    HAL_Delay(1000); // Wait for 1 second before returning
                    return 1; // Return 1 for successful authentication
                }
                else if (id_type == 2)
                {
                    lcd_clear();
                    lcd_put_cur(0, 0);
                    lcd_send_string("Admin Access Granted");
                    HAL_Delay(1000); // Wait for 1 second before entering admin menu
                    return 2; // Return 2 for admin authentication
                }
                else if (id_type == 3)
                {
                    lcd_clear();
                    lcd_put_cur(0, 0);
                    lcd_send_string("Already Voted");
                    HAL_Delay(1000);

                    return 3; // Return 3 for already voted
                }
                else
                {
                    lcd_clear();
                    lcd_put_cur(0, 0);
                    lcd_send_string("Access Denied");
                    HAL_Delay(1000);
                    lcd_clear();
                    return 0; // Return 0 for failed authentication
                }
                buf_pos = 0; // Reset buffer position for next input
                memset(id_input, 0, sizeof(id_input)); // Clear the buffer
            }
            else if (buf_pos < 16)
            {
                id_input[buf_pos++] = key;
                lcd_put_cur(1, 0);          // print entered keys on row 2
                lcd_send_string(id_input);
            }
        }
    }
    
}



void display_menu(int initialize) //0 is default, 1 is initialise
{
    if (button0_check() || button1_check() || initialize)
    {
        if (initialize)
        {
            candidate_index = 0; // Reset to the first candidate when initializing
        }
        lcd_clear();
        lcd_put_cur(0, 8);
        lcd_send_string("B0: ^");
        lcd_put_cur(1, 8);
        lcd_send_string("B1: v");
        lcd_put_cur(0, 0);
        lcd_send_string(">");
        char buffer[17] = {0}; // Buffer to hold the candidate character and null terminator
        snprintf(buffer, sizeof(buffer), "%c", candidates[candidate_index]);
        lcd_send_string(buffer);
    
        if (candidate_index+1 < NUM_CANDIDATES)
        {
            lcd_put_cur(1, 0);
            lcd_send_string(" ");
            snprintf(buffer, sizeof(buffer), "%c", candidates[(candidate_index + 1)]);
            lcd_send_string(buffer);
        }
    }
    
}

void admin_menu(void)
{
    lcd_clear();
    lcd_put_cur(0, 0);
    static char admin_options[3][17] = {
        "1. View Votes",
        "2. Reset Votes",
        "3. Exit"
    };
    while(1)
    {
        lcd_send_string(">");
        lcd_send_string(admin_options[option_index]);
        if (option_index + 1 < 3)
        {
            lcd_put_cur(1, 0);
            lcd_send_string(" ");
            lcd_send_string(admin_options[option_index + 1]);
        }
        button0_check_admin();
        button1_check_admin();
        int select = button2_check();
        if (select)
        {
            if (option_index == 0) // View Votes
            {
                lcd_clear();
                lcd_put_cur(0, 0);
                char buffer[17];
                for (int i = 0; i < NUM_CANDIDATES; i++)
                {
                    snprintf(buffer, sizeof(buffer), "%c: %d", candidates[i], vote_counts[i]);
                    lcd_put_cur(i % 2, 0); // Display on row 0 or 1
                    lcd_send_string(buffer);
                    if (i % 2 == 1 || i == NUM_CANDIDATES - 1) // Wait for user to press button to continue
                    {
                        while (!button2_check())
                        {
                            HAL_Delay(100);
                        }
                        lcd_clear();
                    }
                }
            }
            else if (option_index == 1) // Reset Votes
            {
                memset(vote_counts, 0, sizeof(vote_counts));
                lcd_clear();
                lcd_put_cur(0, 0);
                lcd_send_string("Votes Reset");
                HAL_Delay(2000); // Display for 2 seconds
            }
            else if (option_index == 2) // Exit
            {
                return; // Exit admin menu
            }
        }
    }


}

void print_vote_counts(void)
{
    char msg[50];
    for (int i = 0; i < NUM_CANDIDATES; i++)
    {
        snprintf(msg, sizeof(msg),
                 "Candidate %c: %d votes\r\n",
                 candidates[i],
                 vote_counts[i]);

        HAL_UART_Transmit(&huart2,
                          (uint8_t *)msg,
                          strlen(msg),
                          HAL_MAX_DELAY);
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_TIM1_Init();
    MX_USART2_UART_Init();
    HAL_TIM_Base_Start(&htim1); // Start TIM1 for microsecond delay
    lcd_init();

    lcd_put_cur(0, 0);
    // lcd_send_string("Enter key:");

    

    while (1)
    // #button 1 = scroll uo
    //button 2 = scroll down
    // button 3 = select
    

    {   

        char id_input[17] = {0};
        int authenticated = authenticate_voter(id_input);
        int vote_submitted = 0;

        if (authenticated == 1)
        {
            lcd_clear();
            lcd_put_cur(0,0);
            char vote_prompt[17];
            snprintf(vote_prompt, sizeof(vote_prompt), "Submit Vote");
            lcd_send_string(vote_prompt);
            HAL_Delay(1000); // Wait for 1 second before accepting vote
            lcd_clear();
            display_menu(1); // Initialize the menu display
            while (!vote_submitted)
            {
                display_menu(0);
                HAL_Delay(100);
                if (button2_check())
                {
                 lcd_clear();
                 lcd_put_cur(0, 0);
                 char buffer[17] = {0}; // Buffer to hold the candidate character and null terminator
                 snprintf(buffer, sizeof(buffer), "Vote for: %c", candidates[candidate_index]);
                 lcd_send_string(buffer);
                 lcd_put_cur(1, 0);
                 lcd_send_string("Press to confirm");
                 uint32_t time = HAL_GetTick(); // 5 seconds timeout
                 while (1)
                    {
                        if (button2_check())
                        {
                            lcd_clear();
                            lcd_put_cur(0, 0);
                            lcd_send_string("Vote Submitted");
                            vote_counts[candidate_index]++; // Increment the vote count for the selected candidate
                            snprintf(already_voted_IDS[number_of_voters], sizeof(already_voted_IDS[number_of_voters]), "%s", id_input); // Store the ID of the voter who has voted
                            number_of_voters++; // Increment the number of voters who have voted
                            HAL_Delay(2000); // Wait for 2 seconds before returning to authentication
                            vote_submitted = 1; // Set the flag to exit the voting loop
                            lcd_clear();
                            lcd_put_cur(0, 0);
                            print_vote_counts();
                            break; // Exit the inner loop to return to authentication
                        }

                        if (HAL_GetTick() - time > 5000) // 5 seconds timeout
                        {
                            lcd_clear();
                            lcd_put_cur(0, 0);
                            lcd_send_string("Vote Cancelled");
                            HAL_Delay(2000); // Wait for 2 seconds before returning to authentication
                            vote_submitted = 1; // Set the flag to exit the voting loop
                            lcd_clear();
                            lcd_put_cur(0, 0);
                            
                            break; // Exit the inner loop to return to authentication
                        }
                    }
            
                 // Handle button 2 press event
                }
            }

        }
        if (authenticated == 2)
        {
            lcd_clear();
            lcd_put_cur(0, 0);
            lcd_send_string("Admin Access");
            HAL_Delay(1000); // Wait for 1 second before entering admin menu
            admin_menu();
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

    GPIO_InitTypeDef GPIO_InitStructButtons_A = {0};
    GPIO_InitStructButtons_A.Pin   = GPIO_PIN_1;;
    GPIO_InitStructButtons_A.Mode  = GPIO_MODE_IT_FALLING;  // interrupt on falling edge
    GPIO_InitStructButtons_A.Pull  = GPIO_PULLUP;
    GPIO_InitStructButtons_A.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStructButtons_A);

    GPIO_InitTypeDef GPIO_InitStructButtons_B = {0};
    GPIO_InitStructButtons_B.Pin   = GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStructButtons_B.Mode  = GPIO_MODE_IT_FALLING;  // interrupt on falling edge
    GPIO_InitStructButtons_B.Pull  = GPIO_PULLUP;
    GPIO_InitStructButtons_B.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructButtons_B);

    HAL_NVIC_SetPriority(EXTI1_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);

    
    HAL_NVIC_SetPriority(EXTI4_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI4_IRQn);

    HAL_NVIC_SetPriority(EXTI9_5_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

    GPIO_InitTypeDef GPIO_InitStructUSART2 = {0};

    GPIO_InitStructUSART2.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStructUSART2.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStructUSART2.Pull = GPIO_NOPULL;
    GPIO_InitStructUSART2.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStructUSART2.Alternate = GPIO_AF7_USART2;

    HAL_GPIO_Init(GPIOA, &GPIO_InitStructUSART2);
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





