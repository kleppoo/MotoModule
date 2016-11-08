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
void ATH_Init(void);
uint8_t ATH_Transmit(uint8_t* p_inBuff, uint16_t len);
bool ATH_GetTransmitStatus(void);
uint16_t ATH_Receive(uint8_t* p_outBuff);
     
/* Private Constants ---------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __AT_HANDLER_H */
