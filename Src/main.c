/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : Main program body
  ******************************************************************************
  *
  * COPYRIGHT(c) 2016 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "cmsis_os.h"
#include "macros.h"
#include "drv.h"
#include "thread.h"
#include "memory.h"
#include "phy.h"
#include "stm32f4xx.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
osThreadId tid_blinkLED;
//osThreadId tid_sendSerial;
osThreadId tid_checkButton;
char Buf[1024];

// Required for HAL_GetTick function
extern uint32_t os_time;

uint8_t TXData[] = "This is a test "
    "data from me please dont go\r\nThis should show up on your terminal "
    "without any errors. If so, please check whether your connection is OK or"
    " it may be had some issues. Please fix it before try to communicate with"
    " our system. Thank you.\r\n\r\n";
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
uint32_t HAL_GetTick(void) {
  return os_time;
}

void blinkLED(void const *argument) {
  while (1) {
    __GPIO_WRITE(GPIOA, 5, GPIO_PIN_SET);
    osSignalWait(0x0001, osWaitForever);
    __GPIO_WRITE(GPIOA, 5, GPIO_PIN_RESET);
    osSignalWait(0x0001, osWaitForever);
    
    //sprintf(Buf, "%x\r\n", serbuff);
    //HAL_UART_Transmit(&huart2, (uint8_t *) Buf, strlen(Buf), 0xffffffff);
  }
}

/* void sendSerial(void const *argument) {
	uint8_t *ptr;
	uint16_t blen;
	
  while(1) {
    osSignalWait(1, osWaitForever);
		ptr = Buffer;
		blen = BufferLen;
		while(blen--) {
			HAL_UART_Transmit(&huart2, ptr++, 1, 1);
		}
    //sprintf(Buf, "%x ", printval);
    //HAL_UART_Transmit(&huart2, (uint8_t *) Buf, strlen(Buf), 0xf);
  }
}*/

void checkButton(void const *argument) {
  while (1) {
    if (!__GPIO_READ(GPIOC, 13)) {
      PHY_API_SendStart(TXData, sizeof(TXData) - 1);
      osDelay(200);
    }
  }
}

osThreadDef (blinkLED, osPriorityNormal, 1, 0);
//osThreadDef (sendSerial, osPriorityNormal, 1, 0);
osThreadDef (checkButton, osPriorityNormal, 1, 0);
/* USER CODE END 0 */

int main(void)
{

  /* USER CODE BEGIN 1 */
  osKernelInitialize();
  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_USART2_UART_Init();
  MX_TIM4_Init();
  MX_TIM3_Init();

  /* USER CODE BEGIN 2 */
#ifdef  USE_FULL_ASSERT
  // Board - Serial identification
  sprintf(Buf, "\x0cNUCLEO-F446 Debug Terminal\r\nVisible Light Communication Project\r\n---\r\n\r\n");
  HAL_UART_Transmit(&huart2, (uint8_t *) Buf, strlen(Buf), 0xffff);
#endif
  // Initialize Optical Driver
  DRV_Init();
  // Initialize Memory
  MEM_Init();
  // Initialize PHY layer
  PHY_Init();
  
  // Create threads
  tid_blinkLED = osThreadCreate (osThread(blinkLED), NULL);
  //tid_sendSerial = osThreadCreate (osThread(sendSerial), NULL);
  tid_checkButton = osThreadCreate (osThread(checkButton), NULL);
  // Initialize from thread module
  THR_Init();
  // Start thread execution
  osKernelStart();

  // Run codes
  DRV_RX_Start();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    osDelay(500);
    osSignalSet(tid_blinkLED, 0x0001);

#ifdef  USE_FULL_ASSERT
    // Check for os
    assert_user((osKernelRunning()), "RTX Kernel is not running.");
#endif
  }
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */

  /* USER CODE END 3 */

}

/** System Clock Configuration
*/
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

  __HAL_RCC_PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);

  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line, const char *msg)
{
  sprintf(Buf, "(%s:%d) %s\r\n", file, line, msg);
  HAL_UART_Transmit(&huart2, (uint8_t *) Buf, strlen(Buf), 0xff);
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */

}

#endif

/**
  * @}
  */ 

/**
  * @}
*/ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
