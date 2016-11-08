/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __AT_HANDLER_H
#define __AT_HANDLER_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
void AT_HandlerInit(void);
uint8_t AT_HandlerTransmit(uint8_t* p_inBuff, uint16_t len);
uint16_t AT_HandlerReceive(uint8_t* p_outBuff);
     
/* Private Constants ---------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __AT_HANDLER_H */
