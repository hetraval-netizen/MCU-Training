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

#include <stdio.h>
#include <string.h>

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

#define BUFFER_SIZE 32 // You can make this as big as you need!

volatile char rx_buffer[BUFFER_SIZE];
volatile int head = 0; // The Interrupt controls the head
volatile int tail = 0; // The Main loop controls the tail

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void uart_sendchar (char c) {
    // Wait until the previous bit is send and we are ready for next char
    // TXE Transmit empty bit is set in ISR register once the bit is moved to shift register

    while (!(USART2->ISR & (1UL << 7))) {
    }

    // Then drop the character in the TDR (Transmit data register)
    USART2->TDR = c;
}

void uart_sendstring (char *str) {
    while (*str != '\0'){
        uart_sendchar(*str);
        str++;
    }
}

char uart_getchar (void) {
    // 1. THE SHIELD: Clear any lingering hardware errors (Bits 0-3)
    if (USART2->ISR & 0x0F) {
        USART2->ICR = 0x0F;           // Clear the error flags
        uint32_t flush = USART2->RDR; // Empty the corrupted data from the mailbox
    }

    // 2. Wait until RXNE (Recieve not empty) bit is set
    while (!(USART2->ISR & (1UL << 5))){
    }

    // 3. Read and return the character
    return (char)USART2->RDR;
}

void USART2_IRQHandler(void) {
    if (USART2->ISR & (1UL << 5)){

        // 1. Drop the new character at the current head position
        rx_buffer[head] = (char)USART2->RDR;

        // 2. Advance the head. The modulo (%) makes it wrap back to 0!
        head = (head + 1) % BUFFER_SIZE;
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

  RCC->AHB2ENR |= (1UL << 0); // Enable clock for GPIOA
  RCC->APB1ENR1 |= (1UL << 17); // Enable clock for USART2

  // Set alternate function for PA2
  GPIOA->MODER &= ~(3UL << 4);
  GPIOA->MODER |= (2UL << 4);

  // Set alternate function for PA3
  GPIOA->MODER &= ~(3UL << 6);
  GPIOA->MODER |= (2UL << 6);

  // Set Alternate function register to UART TX
  GPIOA->AFR[0] &= ~(15UL << 8);
  GPIOA->AFR[0] |= (7UL << 8);

  // Set Alternate function register to UART RX

  GPIOA->AFR[0] &= ~(15UL << 12);
  GPIOA->AFR[0] |= (7UL << 12);

  USART2->CR1 = 0;
  USART2->BRR = 80000000 / 115200;

  USART2->CR1 |= (1UL << 5);
  USART2->CR1 |= (1UL << 3);
  USART2->CR1 |= (1UL << 2);

  NVIC_EnableIRQ(USART2_IRQn);

  USART2->CR1 |= (1UL << 0);

  uart_sendstring("===================System Init=================\r\n");
  uart_sendstring("Ready to recieve message, type something\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1){
        // If head and tail are not equal, we have unread data!
        if (head != tail) {

            // 1. Read the oldest unread character
            char current_char = rx_buffer[tail];

            // 2. Print it out
            uart_sendstring("Buffer popped: ");
            uart_sendchar(current_char);
            uart_sendstring("\r\n");

            // 3. Advance the tail to "consume" the character
            tail = (tail + 1) % BUFFER_SIZE;
        }

        // Add a fake delay here just to prove the buffer works!
        // Even if the main loop is slow, the interrupt will catch your fast typing.
        for (volatile int i = 0; i < 500000; i++);
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
