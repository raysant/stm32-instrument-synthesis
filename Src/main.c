/*
 * Copyright (C) 2019 Ray Santana
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "main.h"


static void USBH_user_process(USBH_HandleTypeDef *phost, uint8_t event_id);
static void system_clock_config(void);

extern uint8_t MIDI_rx_buffer[];

USBH_HandleTypeDef hUSB_host;


int main(void) {
  /* STM32F4xx HAL library initialization */
  HAL_Init();

  /* Configure the system clock to 84 MHz */
  system_clock_config();
  
  /* Initialize user button and LEDs */
  BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_GPIO);
  BSP_LED_Init(LED3);
  BSP_LED_Init(LED4);

  srand(8675309);
  /* Don't start until user button is pressed */
  while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) {}
  HAL_Delay(100);
  while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {}

  /* Initialize and start USB host */
  USBH_Init(&hUSB_host, USBH_user_process, 0);
  USBH_RegisterClass(&hUSB_host, USBH_MIDI_CLASS);
  USBH_Start(&hUSB_host);

  /* Initialize audio peripheral and musical instrument */
  instrument_player_init();

  while (1) {
    instrument_player_play();

    USBH_Process(&hUSB_host);
  }
}

/* Handle USB connects and disconnects */
static void USBH_user_process(USBH_HandleTypeDef *phost, uint8_t event_id) {
  switch (event_id) {
    case HOST_USER_DISCONNECTION:
      USBH_MIDI_Stop(phost);
      BSP_LED_Off(LED4);
      break;

    case HOST_USER_CLASS_ACTIVE:
      /* Fill MIDI message buffer once at the start */
      USBH_MIDI_Receive(phost, &MIDI_rx_buffer[0], RX_BUFFER_SIZE);
      BSP_LED_On(LED4);
      break;

    default:
      break;
  }
}

static void system_clock_config(void) {
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();
  
  /* The voltage scaling allows optimizing the power consumption when the device is 
     clocked below the maximum system frequency, to update the voltage scaling value 
     regarding system frequency refer to product datasheet. */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
  
  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }
 
  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK |
                                 RCC_CLOCKTYPE_HCLK |
                                 RCC_CLOCKTYPE_PCLK1 |
                                 RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;  
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
    Error_Handler();
  }
}

void Error_Handler(void) {
  BSP_LED_On(LED3);
  while(1) {
    /* Error -> do nothing */
  }
}
