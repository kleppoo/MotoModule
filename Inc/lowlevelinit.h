/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __LOWLEVELINIT_H
#define __LOWLEVELINIT_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/
enum
{
    LED_TOGGLE = 0,
    LED_ON,
    LED_OFF
};

enum
{
    LED_RED = 0,
    LED_GREEN,
    LED_BLUE
};

/* Exported macro ------------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
void LOWLEVEL_Init(void);
void LED_Set(uint8_t led, uint8_t status);
void Error_Handler(void);
 void Refresh_IWDG(void);
/* Private Constants ---------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/     
     
#ifdef __cplusplus
}
#endif

#endif /* __LOWLEVELINIT_H */

