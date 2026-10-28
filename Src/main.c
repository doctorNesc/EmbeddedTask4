/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"
#include <stdio.h>

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);


/* Function processing DMA Rx data. Counts how many capital and small letters are in sentence.
 * Result is supposed to be stored in global variable of type "letter_count_" that is defined in "main.h"
 *
 * @param1 - received sign
 */
void proccesDmaData(uint8_t sign);


/* Space for your global variables. */
#define MAX_STRING_LEN   35      /* chars allowed after '#' */
volatile uint32_t ms_tick = 0;     /* incremented every 1 ms */
	// type your global variables here:
letter_count_ letter_count;                 /* result, type from main.h */
static char   rx_string[MAX_STRING_LEN + 1];/* received chars + '\0' */
static volatile uint8_t string_ready = 0;   /* set here, cleared in main loop */

int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
  NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

  /* Configure the system clock */
  SystemClock_Config();
  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();

  /* Space for your local variables, callback registration ...*/
  USART2_RegisterCallback(proccesDmaData);
  	  //type your code here:
  LL_SYSTICK_EnableIT();
  while (1)
  {
	  static char     tx_msg[120];        /* static: DMA reads it after PutBuffer returns */
	  static uint32_t last_status_time = 0;
	  static uint8_t  status_pending = 0;

	  if((ms_tick - last_status_time) >= 2000)
	  {
		  last_status_time += 2000;
		  status_pending = 1;
	  }

	  /* 2. Send only when the Tx DMA channel is free */
	  if(!LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_7))
	  {
		  if(string_ready)                       /* valid string has priority */
		  {
			  string_ready = 0;
			  int len = sprintf(tx_msg,
					  "Valid string: %s, lower-case: %d, upper-case: %d\r\n",
					  rx_string, letter_count.lower_cnt, letter_count.upper_cnt);
			  USART2_PutBuffer((uint8_t*)tx_msg, len);
		  }
		  else if(status_pending)
		  {
			  status_pending = 0;
			  uint16_t occupied = USART2_GetRxBufferLoad();
			  uint16_t load_x10 = (uint32_t)occupied * 1000 / DMA_USART2_BUFFER_SIZE;
			  int len = sprintf(tx_msg,
					  "Buffer capacity: %d bytes, occupied memory: %d bytes, load [in %%]: %d.%d%%\r\n",
					  DMA_USART2_BUFFER_SIZE, occupied, load_x10 / 10, load_x10 % 10);
			  USART2_PutBuffer((uint8_t*)tx_msg, len);
		  }
	  }  }
  /* USER CODE END 3 */
}


void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);

  if(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_0)
  {
  Error_Handler();  
  }
  LL_RCC_HSI_Enable();

   /* Wait till HSI is ready */
  while(LL_RCC_HSI_IsReady() != 1)
  {
    
  }
  LL_RCC_HSI_SetCalibTrimming(16);
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI)
  {
  
  }
  LL_Init1msTick(8000000);
  LL_SYSTICK_SetClkSource(LL_SYSTICK_CLKSOURCE_HCLK);
  LL_SetSystemCoreClock(8000000);
}

/*
 * Implementation of function processing data received via USART.
 */
void proccesDmaData(uint8_t sign)
{
	typedef enum { WAITING, RECEIVING } state_t;
	static state_t state = WAITING;
	static uint8_t length = 0;               /* chars stored after '#' */
	static uint8_t lower = 0, upper = 0;

	switch(state)
	{
	case WAITING:
		if(sign == '#')
		{
			length = 0;
			lower = 0;
			upper = 0;
			state = RECEIVING;
		}
		/* any other char is ignored */
		break;

	case RECEIVING:
		if(sign == '#')                       /* new start: restart */
		{
			length = 0;
			lower = 0;
			upper = 0;
		}
		else if(sign == '$')                  /* end: string is valid */
		{
			rx_string[length] = '\0';
			letter_count.lower_cnt = lower;
			letter_count.upper_cnt = upper;
			string_ready = 1;
			state = WAITING;
		}
		else
		{
			rx_string[length++] = (char)sign;
			if(sign >= 'a' && sign <= 'z') lower++;
			else if(sign >= 'A' && sign <= 'Z') upper++;

			/* 35 chars received and none was '$' -> discard */
			if(length >= MAX_STRING_LEN)
			{
				state = WAITING;
			}
		}
		break;
	}
}

void Error_Handler(void)
{

}

#ifdef  USE_FULL_ASSERT

void assert_failed(char *file, uint32_t line)
{ 

}

#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
