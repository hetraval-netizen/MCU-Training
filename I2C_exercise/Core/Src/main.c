/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void i2c_init(void){
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    
    GPIOB->MODER &= ~(GPIO_MODER_MODE6 | GPIO_MODER_MODE7);
    GPIOB->MODER |= (GPIO_MODER_MODE6_1 | GPIO_MODER_MODE7_1);

    GPIOB->AFR[0] &= ~GPIO_AFRL_AFSEL6; // Clear the 4 bits for Pin 6 (AFSEL6)
    GPIOB->AFR[0] |= (4UL << GPIO_AFRL_AFSEL6_Pos);

    GPIOB->AFR[0] &= ~GPIO_AFRL_AFSEL7; // Clear the 4 bits for Pin 7 (AFSEL7)
    GPIOB->AFR[0] |= (4UL << GPIO_AFRL_AFSEL7_Pos);

    GPIOB->OTYPER |= (GPIO_OTYPER_OT6 | GPIO_OTYPER_OT7);

    // 6. Enable the clock for the I2C1 peripheral (attached to APB1 bus)
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C1EN;

    // 7. Make sure I2C1 is completely disabled before we change its settings
    I2C1->CR1 &= ~I2C_CR1_PE; 

    // 8. Set the I2C speed timing to 400 kHz Fast-Mode (Assuming an 80 MHz system clock)
    I2C1->TIMINGR = 0x90310309; 

    // 9. Turn the I2C1 peripheral ON
    I2C1->CR1 |= I2C_CR1_PE;
}

void oled_write_packet(uint8_t control_type, uint8_t payload) {
  // Wait until the I2C bus is free
  while (I2C1->ISR & I2C_ISR_BUSY);

  // Configure for 2 bytes, Auto-End, and generate START
  I2C1->CR2 = 0;
  I2C1->CR2 |= (0x3C << 1) | (2 << I2C_CR2_NBYTES_Pos) | I2C_CR2_AUTOEND | I2C_CR2_START;

  // Send Byte 1: The Control Byte
  while (!(I2C1->ISR & I2C_ISR_TXIS)) {
    if (I2C1->ISR & I2C_ISR_NACKF) {
      GPIOA->ODR |= (1 << 5);      // Turn ON the Green LED to flag an error!
      I2C1->ICR |= I2C_ICR_NACKCF; 
      return;                      
    }
  }
  I2C1->TXDR = control_type;

  // Send Byte 2: The payload
  while (!(I2C1->ISR & I2C_ISR_TXIS)) {
    if (I2C1->ISR & I2C_ISR_NACKF) {
      GPIOA->ODR |= (1 << 5);      // Turn ON the Green LED to flag an error!
      I2C1->ICR |= I2C_ICR_NACKCF; 
      return;                      
    }
  }
  I2C1->TXDR = payload;
  
  // Wait for AUTOEND to finish and clear the flag
  while (!(I2C1->ISR & I2C_ISR_STOPF));
  I2C1->ICR |= I2C_ICR_STOPCF;
}

void oled_send_cmd(uint8_t cmd) { 
    oled_write_packet(0x00, cmd); 
}

void oled_send_data(uint8_t data) { 
  oled_write_packet(0x40, data); // 0x40 tells the OLED "this is a pixel, not a command!"
}

void oled_init(void) {
  for(volatile uint32_t i = 0; i < 50000; i++);
  oled_send_cmd(0xAE); // Turn display OFF
  oled_send_cmd(0x8D); oled_send_cmd(0x14); // Turn Charge Pump ON 
  oled_send_cmd(0xAF); // Turn display ON
  oled_send_cmd(0xA4); // Read from RAM
}

void oled_clear(void) {
  for (uint8_t page = 0; page < 8; page++) {
      oled_send_cmd(0xB0 + page); // Set Page (0-7)
      oled_send_cmd(0x00);        // Set Lower Column
      oled_send_cmd(0x10);        // Set Higher Column
      
      for (uint8_t col = 0; col < 128; col++) {
          oled_send_data(0x00);   // Write 0s to clear all 128 columns
      }
  }
}

void oled_test_pattern(void) {
  for (uint8_t page = 0; page < 8; page++) {
      oled_send_cmd(0xB0 + page);
      oled_send_cmd(0x00);
      oled_send_cmd(0x10);
      
      for (uint8_t col = 0; col < 128; col++) {
          // Draw thick stripes: 16 columns ON, 16 columns OFF
          if ((col / 16) % 2 == 0) {
              oled_send_data(0xFF); // 8 solid vertical pixels
          } else {
              oled_send_data(0x00); // 8 blank vertical pixels
          }
      }
  }
}

void oled_draw_A(uint8_t page, uint8_t col) {
  // 1. The font bitmap for 'A' (5 columns wide)
  uint8_t letter_A[5] = {0x7E, 0x11, 0x11, 0x11, 0x7E};

  // 2. Set the starting position on the screen
  oled_send_cmd(0xB0 + page);                 // Set Page (row 0-7)
  
  // The SSD1306 requires the column address in two halves (Lower and Upper)
  oled_send_cmd(0x00 + (col & 0x0F));         // Extract the Lower 4 bits
  oled_send_cmd(0x10 + ((col >> 4) & 0x0F));  // Extract the Upper 4 bits

  // 3. Send the 5 bytes of pixel data
  for (uint8_t i = 0; i < 5; i++) {
      oled_send_data(letter_A[i]);
  }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  /* USER CODE BEGIN 2 */
  
  // Enable GPIOA clock and set PA5 to Output mode
  RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
  GPIOA->MODER &= ~GPIO_MODER_MODE5;
  GPIOA->MODER |= GPIO_MODER_MODE5_0;


  i2c_init();
  oled_init();
  oled_clear();         // This will create the sweeping black wipe
  oled_draw_A(0, 0);  // This should draw A on the top-left corner of the screen
  oled_draw_A(1, 0);  // This should draw A on the second row, first column of the screen
  oled_draw_A(2, 0);  // This should draw A on the third row, first column of the screen
  oled_draw_A(0, 6);  // This should draw A on the after the first A on the top row (6 columns to the right of the first A)
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
