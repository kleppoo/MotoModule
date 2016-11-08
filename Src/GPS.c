/* Includes ------------------------------------------------------------------*/
#include <string.h>

#include "stm32f1xx_hal.h"
#include "swtimer.h"
#include "lowlevelinit.h"
#include "at_handler.h"
#include "modem.h"
#include "GPS.h"
extern struct_GPS	GPS_DATA;

void GPS_Job (void)
{
	GPS_ProcessReceivedData();

}
/*
volatile struct_GPS	GPS_DATA;
static volatile uint32_t ExeTimeStamp;
typedef struct  {
	uint8_t	GGA[80];
	uint8_t	RMC[80];
	uint8_t	GSV[80];
	uint8_t	VTG[80];
	uint8_t	GSA[80];
	
	uint8_t	TIME[10];		// hh:mm:ss		
	uint8_t	DATE[10];			// 			
	uint8_t	LONGITUDE[16];		
	uint8_t	LATITUDE[16];
	uint8_t	ACTIVE;				// A - aktywne poz, 	
	uint8_t	SPEED[10];				//sdj.xx km/h
	uint8_t	HIGH[10];				// tsdj.xx  wysokosc w metrach
} struct_GPS;
*/


void GPS_ProcessReceivedData(void)
{
	uint8_t buff[120];
	uint16_t len;
	uint16_t i;
	uint16_t error;
	uint8_t * pch;
	
	
	len = GPS_HandlerReceive(buff);
	
	 if(len != 0 )
    {
        i = 0;
        while(i < len)
        {
            ITM_SendChar(buff[i++]);		//do debugowania ale nie wiem jak to wykorzystac
        }
	   if (buff[0]=='$' && buff[1]=='G' && buff[2]=='P')
	   {
		   	 // Ramka      GGA
		   if (buff[3]=='G' && buff[4]=='G' && buff[5]=='A')
		   {   
			  strlcpy((char*)GPS_DATA.GGA,(const char*)buff,len);
		   }
		   if (buff[3]=='G' && buff[4]=='S' && buff[5]=='A') 
		   {   
			  strlcpy((char*)GPS_DATA.GSA,(const char*)buff,len);
		   }
		   if (buff[3]=='G' && buff[4]=='S' && buff[5]=='V')
		   {   
			  strlcpy((char*)GPS_DATA.GSV,(const char*)buff,len);
		   }
		   if (buff[3]=='R' && buff[4]=='M' && buff[5]=='C')
		   {   	// kopiowanie calej ramiki 
				strlcpy((char*)GPS_DATA.RMC,(const char*)buff,len);
				// wyodrebnienia poszcegolnych pol
				pch = (uint8_t *)strtok ((char *)buff,(char *)","); 				// wydzielenie pierwszego przed przecinkiem czyli $GPRMC
				while (pch != NULL)
				{
					// funkcja strtok moze zle dzialac bo nie ma w ciagu buff znacznika konca stringu??!!
					uint8_t u8_conuter= 0;
					pch = (uint8_t *)strtok (NULL,",");
					u8_conuter++;
					switch(u8_conuter)
					{
//http://www.gpsinformation.org/dale/nmea.htm#RMC
						case (0):
						break;
						case(1):
							//hhmmss	godzina UTC
						break;
						case(2):
							// A - active v = void
						break;
						case(3):
							// Latitude
						break;
						case(4):
							// N or S
						break;
						case(5):
							//Longitude
						break;
						case(6):
							// E or W
						break;
						case(7):
							//speed over ground in knots
						break;
						case(8):
							// Track angle in degrees True
						break;
						case(9):
							//  ddmmrr	date
						break;
						case(10):
							// Magnetic Variation
						break;
						case(11):
							//check sume
						break;
						case(12):
//							return(0);	nie powinno wystapic!!!
						break;
					}
				} 
		   }
		   if (buff[3]=='V' && buff[4]=='T' && buff[5]=='G')
		   {   
			  strlcpy((char*)GPS_DATA.VTG,(const char*)buff,len);
		   }
	   }
	   else
	   {
		// odebrano nieprawidlowa ramke
	   }
	   
    }

}

void GPS_Init(void)
{
	GPS_HandlerInit();
}
