/**
  ******************************************************************************
  * File Name          : mxconstants.h
  * Description        : This file contains the common defines of the application
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MXCONSTANT_H
#define __MXCONSTANT_H
  /* Includes ------------------------------------------------------------------*/

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private define ------------------------------------------------------------*/

#define GSM_RST_Pin             GPIO_PIN_0
#define GSM_RST_GPIO_Port       GPIOD
#define GSM_PWR_Pin             GPIO_PIN_1
#define GSM_PWR_GPIO_Port       GPIOD
#define ADC_3_Pin               GPIO_PIN_7
#define ADC_3_GPIO_Port         GPIOA
#define ADC_1_Pin               GPIO_PIN_0
#define ADC_1_GPIO_Port         GPIOB
#define ADC_2_Pin               GPIO_PIN_1
#define ADC_2_GPIO_Port         GPIOB
#define SPI2_NSS_Pin            GPIO_PIN_12
#define SPI2_NSS_GPIO_Port      GPIOB
#define GPS_RST_Pin             GPIO_PIN_8
#define GPS_RST_GPIO_Port       GPIOA
#define GPS_TX_Pin              GPIO_PIN_9
#define GPS_TX_GPIO_Port        GPIOA
#define GPS_RX_Pin              GPIO_PIN_10
#define GPS_RX_GPIO_Port        GPIOA
#define LED_B_Pin               GPIO_PIN_15
#define LED_B_GPIO_Port         GPIOA
#define LED_G_Pin               GPIO_PIN_3
#define LED_G_GPIO_Port         GPIOB
#define LED_R_Pin               GPIO_PIN_4
#define LED_R_GPIO_Port         GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

/**
  * @}
  */ 

/**
  * @}
*/ 

#endif /* __MXCONSTANT_H */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
