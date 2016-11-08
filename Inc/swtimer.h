/* 
 * File:   swtimer.h
 * Author: Ganeshkumar
 *
 * Created on 15. Oktober 2015, 11:40
 */

#ifndef __SWTIMER_H
#define	__SWTIMER_H

#ifdef	__cplusplus
extern "C" {
#endif
    
/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
    
/* Exported types ------------------------------------------------------------*/
typedef enum
{
    SWTIMER_STOPPED = 0,
    SWTIMER_RUNNING,
    SWTIMER_ELAPSED,
}swtimer_status_t;

/// Defintion for software timer
typedef struct
{
    uint32_t timeStamp;
    uint32_t delay;
}swtimer_t;

/* Exported functions --------------------------------------------------------*/
void SWTimer_Initialize(void);
void SWTimer_Start(swtimer_t *p_swTimer, uint32_t delay);
void SWTimer_Stop(swtimer_t *p_swTimer);
swtimer_status_t SWTimer_GetStatus(swtimer_t *p_swTimer);

#ifdef	__cplusplus
}
#endif

#endif	/* __SWTIMER_H */

