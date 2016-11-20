/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MODEM_H
#define __MODEM_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
void MODEM_Init(void);
void MODEM_Job(void);
void MODEM_SetServerData2(char* p_str);
void MODEM_SetServerData2(char* p_str);
     
/* Private Constants ---------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __AT_HANDLER_H */
