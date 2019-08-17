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


static void usbh_user_process(USBH_HandleTypeDef *phost, uint8_t event_id);
static void system_clock_config(void);

USBH_HandleTypeDef usb_host;


int main(void) {
  /* STM32F4xx HAL library initialization */
  HAL_Init();

  /* Configure the system clock to 84 MHz */
  system_clock_config();
  
  /* Initialize user button and LEDs */
  BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_GPIO);
  BSP_LED_Init(LED3);
  BSP_LED_Init(LED4);

  /* Make excitation signals deterministic */
  srand(8675309);

  /* Initialize and start USB host */
  USBH_Init(&usb_host, usbh_user_process, 0);
  USBH_RegisterClass(&usb_host, USBH_MIDI_CLASS);
  USBH_Start(&usb_host);

  /* Initialize audio peripheral and musical instrument */
  instrument_player_init();

  while (1) {
    instrument_player_play();

    USBH_Process(&usb_host);
  }
}

/* Handle USB connects and disconnects */
static void usbh_user_process(USBH_HandleTypeDef *phost, uint8_t event_id) {
  switch (event_id) {
    case HOST_USER_DISCONNECTION:
      usbh_midi_stop(phost);
      BSP_LED_Off(LED4);
      break;

    case HOST_USER_CLASS_ACTIVE:
      /* Fill MIDI message buffer once at the start */
      usbh_midi_receive(phost, &midi_rx_buffer[0], RX_BUFFER_SIZE);
      BSP_LED_On(LED4);
      break;

    default:
      break;
  }
}

static void system_clock_config(void) {
  RCC_ClkInitTypeDef rcc_clk_init;
  RCC_OscInitTypeDef rcc_osc_init;

  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();
  
  /* The voltage scaling allows optimizing the power consumption when the device is 
     clocked below the maximum system frequency, to update the voltage scaling value 
     regarding system frequency refer to product datasheet. */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
  
  /* Enable HSE Oscillator and activate PLL with HSE as source */
  rcc_osc_init.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  rcc_osc_init.HSEState = RCC_HSE_ON;
  rcc_osc_init.PLL.PLLState = RCC_PLL_ON;
  rcc_osc_init.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  rcc_osc_init.PLL.PLLM = 8;
  rcc_osc_init.PLL.PLLN = 336;
  rcc_osc_init.PLL.PLLP = RCC_PLLP_DIV4;
  rcc_osc_init.PLL.PLLQ = 7;
  if(HAL_RCC_OscConfig(&rcc_osc_init) != HAL_OK) {
    error_handler();
  }
 
  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  rcc_clk_init.ClockType = (RCC_CLOCKTYPE_SYSCLK |
                            RCC_CLOCKTYPE_HCLK |
                            RCC_CLOCKTYPE_PCLK1 |
                            RCC_CLOCKTYPE_PCLK2);
  rcc_clk_init.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  rcc_clk_init.AHBCLKDivider = RCC_SYSCLK_DIV1;
  rcc_clk_init.APB1CLKDivider = RCC_HCLK_DIV2;  
  rcc_clk_init.APB2CLKDivider = RCC_HCLK_DIV1;  
  if(HAL_RCC_ClockConfig(&rcc_clk_init, FLASH_LATENCY_2) != HAL_OK) {
    error_handler();
  }
}

void error_handler(void) {
  BSP_LED_On(LED3);
  while(1) {
    /* Error -> do nothing */
  }
}
