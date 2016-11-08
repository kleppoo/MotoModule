/* Includes ------------------------------------------------------------------*/
#include <string.h>

#include "stm32f1xx_hal.h"
#include "swtimer.h"
#include "lowlevelinit.h"
#include "at_handler.h"
#include "modem.h"

/* Private Constants ---------------------------------------------------------*/
#define MODEM_RETRY_FOREVER     0
#define MODEM_RESP_OK           0x01
#define MODEM_RESP_PROMPT       0x02

/* Private types -------------------------------------------------------------*/

typedef enum
{
    MODEM_STATE_GET_CMD = 0,
    MODEM_STATE_SEND_CMD,
}modem_state_t;

typedef enum
{
    MODEM_CMD_NIL = 0,
    MODEM_CMD_ECHO_OFF,
    MODEM_CMD_GPS_ON,
    MODEM_CMD_AGPS_ON,
    MODEM_CMD_CCID_REQ,
    MODEM_CMD_CPIN_REQ,
    MODEM_CMD_CPIN_ENTR,
    MODEM_CMD_CREG,
    MODEM_CMD_CSQ,
    MODEM_CMD_DEFAULT,
}modem_cmd_t;

typedef struct
{
    modem_cmd_t cmd;
    uint8_t* p_atStr;
    uint8_t* p_respStr;
    uint8_t respFlag;
    uint8_t retryCnt;
    uint32_t timeout;
    void (*pfn_respCb)(uint8_t*);
    void (*pfn_errorCb)(uint16_t);
    void (*pfn_retryCmpltCb)(void);
    modem_cmd_t cmdNext;
}modem_cmd_table_t;

/* Private function prototypes -----------------------------------------------*/
static modem_cmd_table_t* MODEM_GetCmdTable(modem_cmd_t cmd);

//static void MODEM_CmdCCIDReq_Resp_Cb(uint8_t* p_str);
static void MODEM_CmdCCIDReq_RetryCmplt_Cb(void);

static void MODEM_CmdCPIN_Req_Resp_Cb(uint8_t* p_str);

static void MODEM_CmdCPIN_Entr_Error_Cb(uint16_t error);

static void MODEM_CmdCREG_Resp_Cb(uint8_t* p_str);
static void MODEM_CmdCSQ_Resp_Cb(uint8_t* p_str);

static void MODEM_ProcessReceivedData(void);
//static bool MODEM_PushCmdToPriorityQ(modem_cmd_t cmd);
//static bool MODEM_PopCmdFromPriorityQ(modem_cmd_t* p_cmdOut);

/* Private variables ---------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/
bool MODEM_InitFlag;
//int8_t MODEM_CmdPriorityQTop;
uint8_t MODEM_State;
uint8_t MODEM_CmdRetryCnt;
uint8_t MODEM_ResponseAwaitFlag;
modem_cmd_t MODEM_Cmd;
//modem_cmd_t MODEM_CmdPriorityQ[20];
modem_cmd_table_t* p_MODEM_CmdTable;
swtimer_t MODEM_TimerCmdTimeout;
swtimer_t MODEM_TimerReadCSQ;

static const modem_cmd_table_t MODEM_CmdTable[] = {
    {
        MODEM_CMD_ECHO_OFF, 
        (uint8_t*)"ATE0\r", (uint8_t*)NULL, 
        MODEM_RESP_OK, 
        MODEM_RETRY_FOREVER,
        100,
        NULL, 
        NULL,
        NULL,
       /* MODEM_CMD_CCID_REQ	*/		MODEM_CMD_GPS_ON
    },
  // wlaczenie GPS
    {
        MODEM_CMD_GPS_ON, 						// nazwa komenty
        (uint8_t*)"AT+GPS = 1\r", (uint8_t*)NULL, 		// komenda
        MODEM_RESP_OK, 				// czy modem odpwie i potwierdzi
        1,										// ilosc powtórzen
        100,										// czas oczekiwania po wyslaniu komendy
        NULL, 
        NULL,
        NULL,
        MODEM_CMD_CCID_REQ							// nastepna komenda
    },    
    // wlaczenie AGPS
        {
        MODEM_CMD_AGPS_ON, 						// nazwa komenty
        (uint8_t*)"AT+AGPS = 1\r", (uint8_t*)NULL, 		// komenda
        MODEM_RESP_OK, 				// czy modem odpwie i potwierdzi
        1,										// ilosc powtórzen
        100,										// czas oczekiwania po wyslaniu komendy
        NULL, 
        NULL,
        NULL,
        MODEM_CMD_CCID_REQ							// nastepna komenda
    },  
    
    {
        MODEM_CMD_CCID_REQ, 
        (uint8_t*)"AT+CCID\r", (uint8_t*)"+SCID: SIM", 
        MODEM_RESP_OK | MODEM_RESP_PROMPT, 
        3,
        3000,
        NULL, 
        NULL,
        MODEM_CmdCCIDReq_RetryCmplt_Cb,
        MODEM_CMD_CPIN_REQ
    },
    
    {
        MODEM_CMD_CPIN_REQ, 
        (uint8_t*)"AT+CPIN?\r", (uint8_t*)NULL, 
        MODEM_RESP_OK | MODEM_RESP_PROMPT, 
        1,
        500,
        MODEM_CmdCPIN_Req_Resp_Cb, 
        NULL,
        NULL,
        MODEM_CMD_DEFAULT
    },
    
    {
        MODEM_CMD_CPIN_ENTR, 
        (uint8_t*)"AT+CPIN=\"2134\"\r", (uint8_t*)NULL, 
        MODEM_RESP_OK, 
        MODEM_RETRY_FOREVER,
        500,
        NULL,
        MODEM_CmdCPIN_Entr_Error_Cb,
        NULL,
        MODEM_CMD_CREG
    },
    
    {
        MODEM_CMD_CREG, 
        (uint8_t*)"AT+CREG?\r", (uint8_t*)NULL, 
        MODEM_RESP_OK | MODEM_RESP_PROMPT,
        MODEM_RETRY_FOREVER,
        2000, 
        MODEM_CmdCREG_Resp_Cb, 
        NULL,
        NULL,
        MODEM_CMD_NIL
    },
    
    {
        MODEM_CMD_CSQ, 
        (uint8_t*)"AT+CSQ\r", (uint8_t*)NULL, 
        MODEM_RESP_OK | MODEM_RESP_PROMPT,
        MODEM_RETRY_FOREVER,
        10000, 
        MODEM_CmdCSQ_Resp_Cb, 
        NULL,
        NULL,
        MODEM_CMD_NIL
    },
};

#define MODEM_INITCMDTABLE_SIZE     (sizeof(MODEM_CmdTable) / sizeof(MODEM_CmdTable[0]))

/* Extern variables ----------------------------------------------------------*/

void MODEM_Init(void)
{    
    MODEM_CmdRetryCnt = 0;
    //MODEM_CmdPriorityQTop = -1;
    MODEM_Cmd = MODEM_CMD_ECHO_OFF;
    MODEM_State = MODEM_STATE_GET_CMD;
    MODEM_InitFlag = false;
    
    SWTimer_Stop(&MODEM_TimerCmdTimeout);
    // Request Signal quality for every 5secs
    SWTimer_Start(&MODEM_TimerReadCSQ, 5000);
    
    AT_HandlerInit();
}

void MODEM_Job(void)
 {
    modem_cmd_t cmd;
    
    // Process the modem AT response
    MODEM_ProcessReceivedData();
    
    switch(MODEM_State)
    {
        case MODEM_STATE_GET_CMD:
            
            cmd = MODEM_CMD_NIL;
        
            if(MODEM_InitFlag == true)
            {
                if(SWTimer_GetStatus(&MODEM_TimerReadCSQ) == SWTIMER_ELAPSED)
                {
                    cmd = MODEM_CMD_CSQ;
                    SWTimer_Start(&MODEM_TimerReadCSQ, 5000);
                }
            }
        
            if(cmd == MODEM_CMD_NIL)
            {
                cmd = MODEM_Cmd;
            }
            
            if(cmd != MODEM_CMD_NIL)
            {
                p_MODEM_CmdTable = MODEM_GetCmdTable(cmd);
                MODEM_State = MODEM_STATE_SEND_CMD;
            }
            break;
            
        case MODEM_STATE_SEND_CMD:   
            
            if(p_MODEM_CmdTable != NULL)
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
                        MODEM_Cmd = p_MODEM_CmdTable->cmdNext;
                    }
                    
                    MODEM_CmdRetryCnt = 0;
                    SWTimer_Stop(&MODEM_TimerCmdTimeout);
                    MODEM_State = MODEM_STATE_GET_CMD;
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
                MODEM_State = MODEM_STATE_GET_CMD;
            }
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

static void MODEM_CmdCCIDReq_RetryCmplt_Cb(void)
{
    LED_Set(LED_RED, LED_ON);
}

static void MODEM_CmdCPIN_Req_Resp_Cb(uint8_t* p_str)
{
    if(strstr((const char*)p_str, (const char*)"+CPIN:READY") != 0)
    {
        MODEM_Cmd = MODEM_CMD_CREG;
        
        // dont wait for error response if actual response to command is available
        MODEM_ResponseAwaitFlag &= ~MODEM_RESP_PROMPT;
    }
    else if(strstr((const char*)p_str, (const char*)"+CPIN:") != 0)
    {
        MODEM_Cmd = MODEM_CMD_CPIN_ENTR;
        
        // dont wait for error response if actual response to command is available
        MODEM_ResponseAwaitFlag &= ~MODEM_RESP_PROMPT;
    }
}

static void MODEM_CmdCPIN_Entr_Error_Cb(uint16_t error)
{
    LED_Set(LED_RED, LED_ON);
}

static void MODEM_CmdCREG_Resp_Cb(uint8_t* p_str)
{
    if((p_str[9] == '5') || (p_str[9] == '1'))
    {
        MODEM_InitFlag = true;
        
        MODEM_Cmd = p_MODEM_CmdTable->cmdNext;
        
        // dont wait for error response if actual response to command is available
        MODEM_ResponseAwaitFlag &= ~MODEM_RESP_PROMPT;
    }
}

static void MODEM_CmdCSQ_Resp_Cb(uint8_t* p_str)
{
    if(strstr((const char*)p_str, (const char*)"+CSQ") != 0)
    {
        MODEM_Cmd = p_MODEM_CmdTable->cmdNext;
        
        // dont wait for error response if actual response to command is available
        MODEM_ResponseAwaitFlag &= ~MODEM_RESP_PROMPT;
    }
}

static modem_cmd_table_t* MODEM_GetCmdTable(modem_cmd_t cmd)
{
    uint16_t i;
    modem_cmd_table_t* p_cmdTable;
    modem_cmd_table_t* p_returnCmdTable;
    
    i = MODEM_INITCMDTABLE_SIZE;
    p_cmdTable = (modem_cmd_table_t*)&MODEM_CmdTable[0];
    p_returnCmdTable = NULL;
    
    while(i--)
    {
        if(p_cmdTable->cmd == cmd)
        {
            p_returnCmdTable = p_cmdTable;
            break;
        }
        p_cmdTable++;
    }
    
    return p_returnCmdTable;
}

#if 0
static bool MODEM_PushCmdToPriorityQ(modem_cmd_t cmd)
{
    bool status;
    
    if (MODEM_CmdPriorityQTop == (sizeof(MODEM_CmdPriorityQ) - 1))
    {
        status = false;
    }
    else
    {   
        status = true;
        ++MODEM_CmdPriorityQTop;
        MODEM_CmdPriorityQ[MODEM_CmdPriorityQTop] = cmd;
    }
    
    return status;
}

static bool MODEM_PopCmdFromPriorityQ(modem_cmd_t* p_cmdOut)
{
    bool status;
    
    if (MODEM_CmdPriorityQTop == -1)
    {   
        status = false;
    }
    else
    {   
        status = true;
        *p_cmdOut = MODEM_CmdPriorityQ[MODEM_CmdPriorityQTop];
        --MODEM_CmdPriorityQTop;
    }
    
    return status;
}
#endif
