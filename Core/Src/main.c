/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
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
#include <ChorusEffect.h>
#include "main.h"
#include "adc.h"
#include "dac.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "usb_otg.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
#include "DelayEffect.h"
#include "TremoloEffect.h"
#include <math.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// data that user works with
#define DATA_SIZE 256

// the size of the entire buffer for both adc and dac
#define BUFFER_SIZE 512
#define SAMPLE_RATE 96021
#define PI 3.14159265359
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

// create ADC and DAC buffer
uint16_t adcData[BUFFER_SIZE];
uint16_t dacData[BUFFER_SIZE];
uint16_t adc2Data[3];

// pointers to the half buffer which should be processed
static volatile uint16_t* inBuffPtr;
static volatile uint16_t* outBuffPtr;

// data ready flag

uint8_t dataReady = 0;
uint8_t effectReady = 1;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


// when we enter this function, first half of the buffer is complete
// so we set the input buffer pointer at the beginning
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
	if(hadc == &hadc1) {
		inBuffPtr = &adcData[0];
		outBuffPtr = &dacData[0];

		dataReady = 1;
	}

}

// when we enter this function, first the second half of the buffer is complete
// so we set the input buffer pointer at the beginning
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	if(hadc == &hadc1) {
		inBuffPtr = &adcData[DATA_SIZE];
		outBuffPtr = &dacData[DATA_SIZE];

		dataReady = 1;
	}

}

enum Effect { CleanEf, DelayEf, SineEf, SquareEf, TriangleEf, ChorusEf };
uint8_t currentEffect =  ChorusEf;

const float INT16_TO_FLOAT = 1.0f / 32768.0f;

uint8_t Is_Tremolo() {
	return currentEffect == SineEf || currentEffect == SquareEf || currentEffect == TriangleEf;
}

void Init_Tremolo_Waveform() {
	if(currentEffect == SineEf) Tremolo_Set_Waveform(Sine);
	else if(currentEffect == SquareEf) Tremolo_Set_Waveform(Square);
	else if(currentEffect == TriangleEf) Tremolo_Set_Waveform(Triangle);
}

void processData()
{
	if(effectReady == 0) return;

	float volume = adc2Data[0]/4095.0f;
	float knob1 = adc2Data[1]/4095.0f;
	float knob2 = adc2Data[2]/4095.0f;

	if(currentEffect == CleanEf) {
		for(int i = 0; i < DATA_SIZE; i++) {
			 outBuffPtr[i] = (uint16_t) (volume * inBuffPtr[i]);
		}
	}
	else if(currentEffect == DelayEf) {
		Delay_Set_Params(knob1, knob2);
		for(int i = 0; i < DATA_SIZE; i++) {
			 outBuffPtr[i] = (uint16_t) (volume * Delay_Process(inBuffPtr[i]));
		}
	} else if(Is_Tremolo()) {
		Init_Tremolo_Waveform();

		static float in, out;
		for(int i = 0; i < DATA_SIZE; i++) {
			in = INT16_TO_FLOAT * inBuffPtr[i];
			out = Tremolo_Process(in, knob1, knob2) * 1.4f;
			outBuffPtr[i] =  (uint16_t) (volume * out * 32768.0f);
		}
	} else if(currentEffect == ChorusEf) {
		Chorus_Set_Params(knob1);
		static float in, out;
		for(int i = 0; i < DATA_SIZE; i++) {
			in = INT16_TO_FLOAT * inBuffPtr[i];
			out = in + Chorus_Process(in);
			outBuffPtr[i] = (uint16_t) (volume * out * 32768.0f);
		}
	}

	dataReady = 0;
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

/* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART3_UART_Init();
  MX_DMA_Init();
  MX_USB_OTG_HS_USB_Init();
  MX_ADC1_Init();
  MX_TIM6_Init();
  MX_DAC1_Init();
  MX_ADC2_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start(&htim1);
  HAL_TIM_Base_Start(&htim6);

  HAL_ADC_Start_DMA(&hadc1, (uint32_t *) adcData, BUFFER_SIZE);
  HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_1, (uint32_t *) dacData, BUFFER_SIZE, DAC_ALIGN_12B_R);

//  Delay_Init(SAMPLE_RATE);
  Chorus_Init(SAMPLE_RATE);
//  Tremolo_Init(SAMPLE_RATE);
//  Flanger_Init();
  HAL_ADC_Start_DMA(&hadc2, (uint32_t *) adc2Data, 3);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if(dataReady) {
		  processData();
	  }
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

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_DIRECT_SMPS_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Macro to configure the PLL clock source
  */
  __HAL_RCC_PLL_PLLSOURCE_CONFIG(RCC_PLLSOURCE_HSE);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_LSI
                              |RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 70;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInitStruct.PLL2.PLL2M = 1;
  PeriphClkInitStruct.PLL2.PLL2N = 16;
  PeriphClkInitStruct.PLL2.PLL2P = 1;
  PeriphClkInitStruct.PLL2.PLL2Q = 2;
  PeriphClkInitStruct.PLL2.PLL2R = 2;
  PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_3;
  PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
  PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
  PeriphClkInitStruct.AdcClockSelection = RCC_ADCCLKSOURCE_PLL2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if(GPIO_Pin == GPIO_PIN_13) {
		effectReady = 0;

		if(currentEffect == ChorusEf) {
			currentEffect = 0;
			Chorus_Free();
		} else if(currentEffect == DelayEf) {
			Delay_Free();
		} else if(currentEffect == 4) {
			Tremolo_Free();
		}

		currentEffect++;

		if(currentEffect == ChorusEf) {
			Chorus_Init(SAMPLE_RATE);
		}
		else if(currentEffect == DelayEf) {
			Delay_Init(SAMPLE_RATE);
		} else if(Is_Tremolo()) {
			Tremolo_Init(SAMPLE_RATE);
		}

		effectReady = 1;
	} else {
		__NOP();
	}
}

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

#ifdef  USE_FULL_ASSERT
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
