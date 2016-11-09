/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GPS_H__
#define __GPS_H__

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stdio.h"
#include "stm32f1xx_hal.h"
/* Exported types ------------------------------------------------------------*/
typedef struct  {
	uint8_t	GGA[80];
	uint8_t	RMC[80];
	uint8_t	GSV[80];
	uint8_t	VTG[80];
	uint8_t	GSA[80];
	
	uint8_t	TIME[16];		// hh:mm:ss		
	uint8_t	DATE[10];			// 			
	uint8_t	LONGITUDE[16];
	uint8_t	N_or_S;
	uint8_t	LATITUDE[16];
	uint8_t	W_or_E;
	uint8_t	ACTIVE;				// A - aktywne poz, 	
	uint8_t	SPEED[10];				//sdj.xx km/h
	uint8_t	HIGH[10];				// tsdj.xx  wysokosc w metrach
} struct_GPS;
	 
	 
	 
/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions --------------------------------------------------------*/
void GPS_ProcessReceivedData(void);
 
void GPS_Init(void);
     
/* Private Constants ---------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __AT_HANDLER_H */





