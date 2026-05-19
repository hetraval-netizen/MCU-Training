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
#include "console.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define OPERATING_FREQUENCY 80000000
#define BAUD_RATE 115200
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

void led_init(void){
    // Enable clock for GPIOA
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN; 
    
    // Set PA5 to General Purpose Output Mode (01)
    GPIOA->MODER &= ~GPIO_MODER_MODE5;   // Clear both bits for Pin 5
    GPIOA->MODER |= GPIO_MODER_MODE5_0;  // Set Pin 5 to Output

    GPIOA->MODER &= ~GPIO_MODER_MODE8;   // Clear both bits for Pin 8
    GPIOA->MODER |= GPIO_MODER_MODE8_0;  // Set Pin 8 to Output
}

void uart_init(void){
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN; // Enable clock for USART2

    // Set alternate function for PA2 (Pin 2)
    GPIOA->MODER &= ~GPIO_MODER_MODE2;      // Clear bits for Pin 2
    GPIOA->MODER |= GPIO_MODER_MODE2_1;     // Set to Alternate Function (10)

    // Set alternate function for PA3 (Pin 3)
    GPIOA->MODER &= ~GPIO_MODER_MODE3;      // Clear bits for Pin 3
    GPIOA->MODER |= GPIO_MODER_MODE3_1;     // Set to Alternate Function (10)

    // --- Set Alternate function register to UART TX (PA2 -> AF7) ---
    // Clear the 4 bits for Pin 2 (AFSEL2)
    GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL2; 
    // Shift the number '7' into the Pin 2 position
    GPIOA->AFR[0] |= (7UL << GPIO_AFRL_AFSEL2_Pos); 

    // --- Set Alternate function register to UART RX (PA3 -> AF7) ---
    // Clear the 4 bits for Pin 3 (AFSEL3)
    GPIOA->AFR[0] &= ~GPIO_AFRL_AFSEL3; 
    // Shift the number '7' into the Pin 3 position
    GPIOA->AFR[0] |= (7UL << GPIO_AFRL_AFSEL3_Pos);

    // UART Hardware Configuration
    USART2->CR1 = 0;
    USART2->BRR = OPERATING_FREQUENCY / BAUD_RATE;

    USART2->CR1 |= USART_CR1_RXNEIE; // Enable RXNE interrupt
    USART2->CR1 |= USART_CR1_TE;     // Enable transmitter
    USART2->CR1 |= USART_CR1_RE;     // Enable receiver

    NVIC_EnableIRQ(USART2_IRQn);

    USART2->CR1 |= USART_CR1_UE;     // Enable USART2
}

void timer_init(void) {
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN; // Enable clock for TIM2

    TIM2->PSC = 8000 - 1;

    TIM2->ARR = 5000 - 1;

    TIM2->DIER |= TIM_DIER_UIE;

    NVIC_EnableIRQ(TIM2_IRQn);
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
  led_init();
  uart_init();
  timer_init();
  console_init();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    GPIOA->ODR ^= GPIO_ODR_OD5; // Toggle PA5
    HAL_Delay(500); // Delay for 500 ms
    GPIOA->ODR ^= GPIO_ODR_OD5; // Toggle PA5
    HAL_Delay(500); // Delay for 500 ms
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
