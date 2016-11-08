/* Includes ------------------------------------------------------------------*/
#include <string.h>

#include "stm32f1xx_hal.h"
#include "swtimer.h"
#include "lowlevelinit.h"
#include "at_handler.h"
#include "modem.h"

/* Private Constants ---------------------------------------------------------*/
#define MODEM_FOREVER_RETRY     0
#define MODEM_RESP_OK           0x01
#define MODEM_RESP_PROMPT       0x02

enum
{
    MODEM_STATE_INIT = 0,
    MODEM_STATE_NIL,
};

enum
{
    MODEM_CMD_NIL = 0,
    MODEM_CMD_ECHO_OFF,
    MODEM_CMD_CPIN_REQ,
    MODEM_CMD_CPIN_ENTR,
    MODEM_CMD_CREG,
    MODEM_CMD_DEFAULT,
};

/* Private types -------------------------------------------------------------*/

typedef struct
{
    uint8_t cmd;
    uint8_t* p_atStr;
    uint8_t* p_respStr;
    uint8_t respFlag;
    uint8_t retryCnt;
    uint32_t timeout;
    void (*pfn_respCb)(uint8_t*);
    void (*pfn_errorCb)(uint16_t);
    void (*pfn_retryCmpltCb)(void);
}modem_cmd_table_t;

/* Private function prototypes -----------------------------------------------*/
static void MODEM_RespCbCmdCPINReq(uint8_t* p_str);
static void MODEM_RetryCmpltCbCmdCPINReq(void);
static void MODEM_RespCbCmdCREG(uint8_t* p_str);
static void MODEM_ProcessReceivedData(void);

/* Private variables ---------------------------------------------------------*/
static uint8_t MODEM_State;
static uint8_t MODEM_CmdRetryCnt;
static uint8_t MODEM_StatePrio;
static uint8_t MODEM_ResponseAwaitFlag;
static swtimer_t MODEM_TimerCmdTimeout;
static modem_cmd_table_t* p_MODEM_CmdTable;
static modem_cmd_table_t* p_MODEM_CmdTableNext;

static const modem_cmd_table_t MODEM_InitCmdTable[] = {
    {
        MODEM_CMD_ECHO_OFF, 
        (uint8_t*)"ATE0\r", (uint8_t*)NULL, 
        MODEM_RESP_OK, 
        MODEM_FOREVER_RETRY,
        100,
        NULL, 
        NULL,
        NULL
    },
    {
        MODEM_CMD_CPIN_REQ, 
        (uint8_t*)"AT+CPIN?\r", (uint8_t*)NULL, 
        MODEM_RESP_OK | MODEM_RESP_PROMPT, 
        3,
        3000,
        MODEM_RespCbCmdCPINReq, 
        NULL,
        MODEM_RetryCmpltCbCmdCPINReq
    },
    {
        MODEM_CMD_CPIN_ENTR, 
        (uint8_t*)"AT+CPIN=\"2134\"\r", (uint8_t*)NULL, 
        MODEM_RESP_OK, 
        MODEM_FOREVER_RETRY,
        500,
        NULL,
        NULL,
        NULL
    },
    {
        MODEM_CMD_CREG, 
        (uint8_t*)"AT+CREG?\r", (uint8_t*)NULL, 
        MODEM_RESP_OK | MODEM_RESP_PROMPT,
        MODEM_FOREVER_RETRY,
        2000, 
        MODEM_RespCbCmdCREG, 
        NULL,
        NULL
    },
};

#define MODEM_INITCMDTABLE_SIZE     (sizeof(MODEM_InitCmdTable) / sizeof(MODEM_InitCmdTable[0]))

/* Extern variables ----------------------------------------------------------*/

    
void MODEM_Init(void)
{ 
    MODEM_State = MODEM_STATE_INIT;
    MODEM_StatePrio = MODEM_STATE_NIL;
    // As of now to avoid compiler warning
    MODEM_StatePrio = MODEM_StatePrio;
    
    MODEM_CmdRetryCnt = 0;
    
    p_MODEM_CmdTable = (modem_cmd_table_t*)MODEM_InitCmdTable;
    SWTimer_Stop(&MODEM_TimerCmdTimeout);
    
    AT_HandlerInit();
}

void MODEM_Job(void)
{
    MODEM_ProcessReceivedData();
    
    switch(MODEM_State)
    {       
        case MODEM_STATE_INIT:
            if((uint32_t)p_MODEM_CmdTable <= ((uint32_t)&MODEM_InitCmdTable[MODEM_INITCMDTABLE_SIZE - 1]))
            {
                if(SWTimer_GetStatus((&MODEM_TimerCmdTimeout)) == SWTIMER_STOPPED)
                {
                    AT_HandlerTransmit((uint8_t*)p_MODEM_CmdTable->p_atStr, strlen((const char*)p_MODEM_CmdTable->p_atStr));
                    SWTimer_Start(&MODEM_TimerCmdTimeout, p_MODEM_CmdTable->timeout);
                    MODEM_ResponseAwaitFlag = p_MODEM_CmdTable->respFlag;
                }
                else if(MODEM_ResponseAwaitFlag == 0x00)
                {
                    if(p_MODEM_CmdTable->pfn_respCb == NULL)
                    {
                        p_MODEM_CmdTable++;
                    }
                    else
                    {
                        p_MODEM_CmdTable = p_MODEM_CmdTableNext;
                    }
                    
                    MODEM_CmdRetryCnt = 0;
                    SWTimer_Stop(&MODEM_TimerCmdTimeout);
                }
                else if(SWTimer_GetStatus((&MODEM_TimerCmdTimeout)) == SWTIMER_ELAPSED)
                {
                    SWTimer_Stop(&MODEM_TimerCmdTimeout);
                    if(p_MODEM_CmdTable->retryCnt)
                    {
                        MODEM_CmdRetryCnt++;
                        if(MODEM_CmdRetryCnt >= p_MODEM_CmdTable->retryCnt)
                        {
                            if(p_MODEM_CmdTable->pfn_retryCmpltCb != NULL)
                            {
                                p_MODEM_CmdTable->pfn_retryCmpltCb();
                            }
                        }
                    }
                }
            }
            else
            {
                MODEM_State = MODEM_STATE_NIL;
            }
            break;
        
        default:
            break;
    }
    
    
}

static void MODEM_ProcessReceivedData(void)
{
    uint8_t buff[300];
    uint16_t len;
    uint16_t i;
    uint16_t error;
    
    len = AT_HandlerReceive(buff);
    
    if(len != 0)
    {
        i = 0;
        while(i < len)
        {
            ITM_SendChar(buff[i++]);
        }
        
        if(MODEM_ResponseAwaitFlag & MODEM_RESP_OK)
        {
            if(strstr((const char*)buff, "OK") != 0)
            {
                MODEM_ResponseAwaitFlag &= ~MODEM_RESP_OK;
            }
        }
        
        if(MODEM_ResponseAwaitFlag & MODEM_RESP_PROMPT)
        {
            if(p_MODEM_CmdTable != NULL)
            {
                if(p_MODEM_CmdTable->pfn_respCb != NULL)
                {
                    p_MODEM_CmdTable->pfn_respCb(buff);
                }
                else
                {
                    if(strstr((const char*)buff, (const char*)p_MODEM_CmdTable->p_respStr) != 0)
                    {   
                        // dont wait for error response if actual response to command is available
                        MODEM_ResponseAwaitFlag &= ~MODEM_RESP_PROMPT;
                    }
                }
            }
        }

        if(strstr((const char*)buff, (const char*)"+CME ERROR:") != 0)
        {
            if(buff[len - 3] == ':')
            {
                error = ((buff[len - 2] - 0x30) * 10) + (buff[len - 1] - 0x30);
            }
            else
            {
                error = ((buff[len - 3] - 0x30) * 100) + ((buff[len - 2]- 0x30) * 10) + (buff[len - 1] - 0x30);  
            }
         
            if(p_MODEM_CmdTable != NULL)
            {                
                if(p_MODEM_CmdTable->pfn_errorCb != NULL)
                {
                    p_MODEM_CmdTable->pfn_errorCb(error);
                }
            }
        }
        else if(strstr((const char*)buff, (const char*)"ERROR") != 0)
        {
            if(p_MODEM_CmdTable != NULL)
            {
                if(p_MODEM_CmdTable->pfn_errorCb != NULL)
                {
                    p_MODEM_CmdTable->pfn_errorCb(error);
                }
            }
        }
    }
}

static void MODEM_RespCbCmdCPINReq(uint8_t* p_str)
{
    if(strstr((const char*)p_str, (const char*)"+CPIN:READY") != 0)
    {
        p_MODEM_CmdTableNext = p_MODEM_CmdTable + 2;
        
        // dont wait for error response if actual response to command is available
        MODEM_ResponseAwaitFlag &= ~MODEM_RESP_PROMPT;
    }
    else if(strstr((const char*)p_str, (const char*)"+CPIN:") != 0)
    {
        p_MODEM_CmdTableNext = p_MODEM_CmdTable + 1;
        
        // dont wait for error response if actual response to command is available
        MODEM_ResponseAwaitFlag &= ~MODEM_RESP_PROMPT;
    }
}

static void MODEM_RetryCmpltCbCmdCPINReq(void)
{
    LED_Set(LED_RED, LED_ON);
}

static void MODEM_RespCbCmdCREG(uint8_t* p_str)
{
    if((p_str[9] == '5') || (p_str[9] == '1'))
    {
        p_MODEM_CmdTableNext = p_MODEM_CmdTable + 1;
        
        // dont wait for error response if actual response to command is available
        MODEM_ResponseAwaitFlag &= ~MODEM_RESP_PROMPT;
    }
}
