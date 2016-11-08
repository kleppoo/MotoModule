
/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"
#include "swtimer.h"

/* Extern variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/**
 * 
 * @param timerId
 * @param ms
 */
void SWTimer_Start(swtimer_t *p_swTimer, uint32_t delay)
{
    // copy the duration
    p_swTimer->delay = delay;
    // copy the current systick to the timestamp, this will be used to check whether the
    // software is elapsed or still running
    p_swTimer->timeStamp = HAL_GetTick();
}

/**
 * 
 * @param timerId
 */
void SWTimer_Stop(swtimer_t *p_swTimer)
{
    // Reset the delay to 0 which internally means the software timer is stopped
    p_swTimer->delay = 0;
    // reset the timestamp to 0
    p_swTimer->timeStamp = 0;
}

/**
 * 
 * @param timerId
 * @return 
 */
swtimer_status_t SWTimer_GetStatus(swtimer_t *p_swTimer)
{
    swtimer_status_t status;
    uint32_t tick;

    // get the current systick
    tick = HAL_GetTick();

    // Check if the software timer is still running
    if(p_swTimer->delay)
    {
        // Check if the systick is overflowed or not
        if(tick < p_swTimer->timeStamp)
        {
            // Do the overflow correction
            // Check if the software timer is elapsed or not
            if(((0xFFFFFFFF - p_swTimer->timeStamp) + tick) > p_swTimer->delay)
            {
                // software timer is elapsed
                status = SWTIMER_ELAPSED;
            }
            else
            {
                // software timer is still running
                status = SWTIMER_RUNNING;
            }
        }
        else
        {
            // Check if the software timer is elapsed or not
            if((tick - p_swTimer->timeStamp) > p_swTimer->delay)
            {
                // software timer is elapsed
                status = SWTIMER_ELAPSED;
            }
            else
            {
                // software timer is still running
                status = SWTIMER_RUNNING;
            }
        }
    }
    else
    {
        // software timer is stopped
        status = SWTIMER_STOPPED;
    }

    // return the status of the software timer
    return status;
}
