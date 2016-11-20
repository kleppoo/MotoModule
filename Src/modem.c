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

#define CSQ_REQ_INTERVAL        7000
#define DATA_SEND_INTERVAL      5000

/* Private types -------------------------------------------------------------*/

typedef enum
{
    MODEM_STATE_GET_CMD = 0,
    MODEM_STATE_SEND_CMD,
}modem_state_t;

typedef enum
{
    MODEM_CMD_NIL = 0,
    MODEM_CMD_RESET,
    MODEM_CMD_ATZ,
    MODEM_CMD_CMEE,
    MODEM_CMD_ECHO_OFF,
    MODEM_CMD_CCID_REQ,
    MODEM_CMD_CPIN_REQ,
    MODEM_CMD_CPIN_ENTR,
    MODEM_CMD_CREG,
    MODEM_CMD_CSQ,
    MODEM_CMD_CGATT_REQ,
    MODEM_CMD_CGATT_DEATTACH,
    MODEM_CMD_CGATT_ATTACH,
    MODEM_CMD_CGDCONT,
    MODEM_CMD_CSTT,
    MODEM_CMD_CGACT_REQ,
    MODEM_CMD_CGACT_DEACT,
    MODEM_CMD_CGACT_ACT,
    MODEM_CMD_CIFSR,
    MODEM_CMD_CDNSGIP,
    MODEM_CMD_CIICR,
    MODEM_CMD_CIPSTART,
    MODEM_CMD_CIPSEND_START_PH1,
    MODEM_CMD_CIPSEND_PH1,
    MODEM_CMD_CIPSEND_START_PH2,
    MODEM_CMD_CIPSEND_PH2,
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
    void (*pfn_cmdFrameCb)(modem_cmd_t);
    void (*pfn_respCb)(uint8_t*);
    void (*pfn_errorCb)(uint16_t);
    void (*pfn_retryCmpltCb)(void);
    modem_cmd_t cmdNext;
}modem_cmd_table_t;

/* Private function prototypes -----------------------------------------------*/
static void MODEM_Reset(void);

static modem_cmd_table_t* MODEM_GetCmdTable(modem_cmd_t cmd);

static void MODEM_CmdFrame_Cb(modem_cmd_t cmd); 

static void MODEM_CmdCCIDReq_RetryCmplt_Cb(void);

static void MODEM_CmdCPIN_Req_Resp_Cb(uint8_t* p_str);

static void MODEM_CmdCPIN_Entr_Error_Cb(uint16_t error);

static void MODEM_CmdCREG_Resp_Cb(uint8_t* p_str);
static void MODEM_CmdCSQ_Resp_Cb(uint8_t* p_str);

static void MODEM_CmdCGATT_Req_Resp_Cb(uint8_t* p_str);

static void MODEM_CmdCGACT_Req_Resp_Cb(uint8_t* p_str);
//static void MODEM_CmdCGACT_ACT_Error_Cb(uint16_t error);

static void MODEM_CmdCIFSR_RetryCmplt_Cb(void);

static void MODEM_CmdCIPSEND_RetryCmplt_Cb(void);

static void MODEM_CmdCDNSGIP_Resp_Cb(uint8_t* p_str);

static void MODEM_ProcessReceivedData(void);
//static bool MODEM_PushCmdToPriorityQ(modem_cmd_t cmd);
//static bool MODEM_PopCmdFromPriorityQ(modem_cmd_t* p_cmdOut);

/* Private variables ---------------------------------------------------------*/

/* Global variables ----------------------------------------------------------*/
bool MODEM_InitFlag;
bool MODEM_LOAD_RespRcvd;
bool MODEM_ON_RespRcvd;
//int8_t MODEM_CmdPriorityQTop;
uint8_t MODEM_State;
uint8_t MODEM_CmdRetryCnt;
uint8_t MODEM_ServerData1[30] = "## IMEI: 359586015829802, A;";
uint8_t MODEM_ServerData2[75] = "IMEI: 359586015829802, tracker, 000,000,000.13554900601, L,;";
uint8_t MODEM_CmdFrameBuff[100];
uint8_t MODEM_Onetpl_IpAddr[16];
uint8_t MODEM_ResponseAwaitFlag;
uint16_t MODEM_SignalQ;
modem_cmd_t MODEM_Cmd;
//modem_cmd_t MODEM_CmdPriorityQ[20];
modem_cmd_table_t* p_MODEM_CmdTable;
swtimer_t MODEM_TimerCmdTimeout;
swtimer_t MODEM_TimerReadCSQ;
swtimer_t MODEM_TimerSendData;

const modem_cmd_table_t MODEM_CmdTable[] = {
    {
        .cmd = MODEM_CMD_CMEE, 
        .p_atStr = (uint8_t*)"AT+CMEE=2\r", 
        .p_respStr = (uint8_t*)NULL, 
        .respFlag = MODEM_RESP_OK, 
        .retryCnt = MODEM_RETRY_FOREVER,
        .timeout = 100,
        .pfn_cmdFrameCb = NULL, 
        .pfn_respCb = NULL, 
        .pfn_errorCb = NULL,
        .pfn_retryCmpltCb = NULL,
        .cmdNext = MODEM_CMD_ECHO_OFF
    },
    
    {
        .cmd = MODEM_CMD_ECHO_OFF, 
        .p_atStr = (uint8_t*)"ATE0\r", 
        .p_respStr = (uint8_t*)NULL, 
        .respFlag = MODEM_RESP_OK, 
        .retryCnt = MODEM_RETRY_FOREVER,
        .timeout = 100,
        .pfn_cmdFrameCb = NULL, 
        .pfn_respCb = NULL, 
        .pfn_errorCb = NULL,
        .pfn_retryCmpltCb = NULL,
        .cmdNext = MODEM_CMD_CCID_REQ
    },
    
    {
        .cmd = MODEM_CMD_CCID_REQ, 
        .p_atStr = (uint8_t*)"AT+CCID\r", 
        .p_respStr = (uint8_t*)"+SCID: SIM", 
        .respFlag = MODEM_RESP_OK | MODEM_RESP_PROMPT, 
        .retryCnt = 3,
        .timeout = 3000,
        .pfn_cmdFrameCb = NULL,
        .pfn_respCb = NULL, 
        .pfn_errorCb = NULL,
        .pfn_retryCmpltCb = MODEM_CmdCCIDReq_RetryCmplt_Cb,
        .cmdNext = MODEM_CMD_CPIN_REQ
    },
    
    {
        .cmd = MODEM_CMD_CPIN_REQ, 
        .p_atStr = (uint8_t*)"AT+CPIN?\r", 
        .p_respStr = (uint8_t*)NULL, 
        .respFlag = MODEM_RESP_OK | MODEM_RESP_PROMPT, 
        .retryCnt = 1,
        .timeout = 500,
        .pfn_cmdFrameCb = NULL, 
        .pfn_respCb = MODEM_CmdCPIN_Req_Resp_Cb, 
        .pfn_errorCb = NULL,
        .pfn_retryCmpltCb = NULL,
        .cmdNext = MODEM_CMD_DEFAULT
    },
    
    {
        .cmd = MODEM_CMD_CPIN_ENTR, 
        .p_atStr = (uint8_t*)"AT+CPIN=\"2134\"\r", 
        .p_respStr = (uint8_t*)NULL, 
        .respFlag = MODEM_RESP_OK, 
        .retryCnt = MODEM_RETRY_FOREVER,
        .timeout = 500,
        .pfn_cmdFrameCb = NULL,
        .pfn_respCb = NULL, 
        .pfn_errorCb = MODEM_CmdCPIN_Entr_Error_Cb,
        .pfn_retryCmpltCb = NULL,
        .cmdNext = MODEM_CMD_CREG
    },
    
    {
        .cmd = MODEM_CMD_CREG, 
        .p_atStr = (uint8_t*)"AT+CREG?\r", 
        .p_respStr = (uint8_t*)NULL, 
        .respFlag = MODEM_RESP_OK | MODEM_RESP_PROMPT,
        .retryCnt = MODEM_RETRY_FOREVER,
        .timeout = 2000, 
        .pfn_cmdFrameCb = NULL, 
        .pfn_respCb = MODEM_CmdCREG_Resp_Cb, 
        .pfn_errorCb = NULL,
        .pfn_retryCmpltCb = NULL,
        .cmdNext = MODEM_CMD_CGATT_REQ
    },

    {
        .cmd = MODEM_CMD_CGATT_REQ, 
        .p_atStr = (uint8_t*)"AT+CGATT?\r", 
        .p_respStr = (uint8_t*)NULL, 
        .respFlag = MODEM_RESP_OK | MODEM_RESP_PROMPT,
        .retryCnt = MODEM_RETRY_FOREVER,
        .timeout = 2000, 
        .pfn_cmdFrameCb = NULL, 
        .pfn_respCb = MODEM_CmdCGATT_Req_Resp_Cb, 
        .pfn_errorCb = NULL,
        .pfn_retryCmpltCb = NULL,
        .cmdNext = MODEM_CMD_DEFAULT
    },
    
    {
        .cmd = MODEM_CMD_CGATT_DEATTACH, 
        .p_atStr = (uint8_t*)"AT+CGATT=0\r", 
        .p_respStr = (uint8_t*)NULL,
        .respFlag = MODEM_RESP_OK,
        .retryCnt = MODEM_RETRY_FOREVER,
        .timeout = 5000, 
        .pfn_cmdFrameCb = NULL, 
        .pfn_respCb = NULL, 
        .pfn_errorCb = NULL,
        .pfn_retryCmpltCb = NULL,
        .cmdNext = MODEM_CMD_CGATT_REQ
    },
    
    {
        .cmd = MODEM_CMD_CGATT_ATTACH, 
        .p_atStr = (uint8_t*)"AT+CGATT=1\r", 
        .p_respStr = (uint8_t*)NULL,
        .respFlag = MODEM_RESP_OK,
        .retryCnt = MODEM_RETRY_FOREVER,
        .timeout = 5000, 
        .pfn_cmdFrameCb = NULL, 
        .pfn_respCb = NULL, 
        .pfn_errorCb = NULL,
        .pfn_retryCmpltCb = NULL,
        .cmdNext = MODEM_CMD_CGDCONT
    },
    
    {
        .cmd = MODEM_CMD_CGDCONT, 
        .p_atStr = (uint8_t*)"AT+CGDCONT=1,\"IP\",\"internet\"\r", 
        .p_respStr = (uint8_t*)NULL, 
        .respFlag = MODEM_RESP_OK,
        .retryCnt = MODEM_RETRY_FOREVER,
        .timeout = 200, 
        .pfn_cmdFrameCb = NULL, 
        .pfn_respCb = NULL, 
        .pfn_errorCb = NULL,
        .pfn_retryCmpltCb = NULL,
        .cmdNext = MODEM_CMD_CSTT
    },
    
    {
        .cmd = MODEM_CMD_CSTT, 
        .p_atStr = (uint8_t*)"AT+CSTT=\"internet\",\"internet\",\"internet\"\r", 
        .p_respStr = (uint8_t*)NULL, 
        .respFlag = MODEM_RESP_OK,
        .retryCnt = MODEM_RETRY_FOREVER,
        .timeout = 200, 
        .pfn_cmdFrameCb = NULL, 
        .pfn_respCb = NULL, 
        .pfn_errorCb = NULL,
        .pfn_retryCmpltCb = NULL,
        .cmdNext = MODEM_CMD_CGACT_REQ
    },
    
    {
        .cmd = MODEM_CMD_CGACT_REQ, 
        .p_atStr = (uint8_t*)"AT+CGACT?\r", 
        .p_respStr = (uint8_t*)NULL, 
        .respFlag = MODEM_RESP_OK | MODEM_RESP_PROMPT,
        .retryCnt = MODEM_RETRY_FOREVER,
        .timeout = 500, 
        .pfn_cmdFrameCb = NULL, 
        .pfn_respCb = MODEM_CmdCGACT_Req_Resp_Cb, 
        .pfn_errorCb = NULL,
        .pfn_retryCmpltCb = NULL,
        .cmdNext = MODEM_CMD_DEFAULT
    },
    
    {
        .cmd = MODEM_CMD_CGACT_DEACT, 
        .p_atStr = (uint8_t*)"AT+CGACT=0,1\r", 
        .p_respStr = (uint8_t*)NULL, 
        .respFlag = MODEM_RESP_OK,
        .retryCnt = MODEM_RETRY_FOREVER,
        .timeout = 5000, 
        .pfn_cmdFrameCb = NULL, 
        .pfn_respCb = NULL, 
        .pfn_errorCb = NULL,
        .pfn_retryCmpltCb = NULL,
        .cmdNext = MODEM_CMD_CGACT_REQ
    },
    
    {
        .cmd = MODEM_CMD_CGACT_ACT, 
        .p_atStr = (uint8_t*)"AT+CGACT=1,1\r", 
        .p_respStr = (uint8_t*)NULL, 
        .respFlag = MODEM_RESP_OK,
        .retryCnt = MODEM_RETRY_FOREVER,
        .timeout = 5000, 
        .pfn_cmdFrameCb = NULL, 
        .pfn_respCb = NULL, 
        .pfn_errorCb = NULL,
        .pfn_retryCmpltCb = NULL,
        .cmdNext = MODEM_CMD_CGACT_REQ
    },
    
    {
        .cmd = MODEM_CMD_CIFSR, 
        .p_atStr = (uint8_t*)"AT+CIFSR\r", 
        .p_respStr = (uint8_t*)".", 
        .respFlag = MODEM_RESP_OK | MODEM_RESP_PROMPT,
        .retryCnt = 1,
        .timeout = 5000, 
        .pfn_cmdFrameCb = NULL, 
        .pfn_respCb = NULL, 
        .pfn_errorCb = NULL,
        .pfn_retryCmpltCb = MODEM_CmdCIFSR_RetryCmplt_Cb,
        .cmdNext = MODEM_CMD_CIPSTART
    },
    
    {
        .cmd = MODEM_CMD_CDNSGIP, 
        .p_atStr = (uint8_t*)"AT+CDNSGIP=www.onet.pl\r", 
        .p_respStr = (uint8_t*)NULL, 
        .respFlag = MODEM_RESP_OK | MODEM_RESP_PROMPT,
        .retryCnt = MODEM_RETRY_FOREVER,
        .timeout = 5000, 
        .pfn_cmdFrameCb = NULL, 
        .pfn_respCb = MODEM_CmdCDNSGIP_Resp_Cb, 
        .pfn_errorCb = NULL,
        .pfn_retryCmpltCb = NULL,
        .cmdNext = MODEM_CMD_DEFAULT
    },
    
    {
        .cmd = MODEM_CMD_CIPSTART, 
        .p_atStr = (uint8_t*)"AT+CIPSTART=\"TCP\",\"217.149.242.242\",12103\r", 
        .p_respStr = (uint8_t*)"CONNECT OK", 
        .respFlag = MODEM_RESP_OK | MODEM_RESP_PROMPT,
        .retryCnt = MODEM_RETRY_FOREVER,
        .timeout = 5000, 
        .pfn_cmdFrameCb = NULL, 
        .pfn_respCb = NULL, 
        .pfn_errorCb = NULL,
        .pfn_retryCmpltCb = NULL,
        .cmdNext = MODEM_CMD_CIPSEND_START_PH1
    },
    
    {
        .cmd = MODEM_CMD_CIPSEND_START_PH1, 
        .p_atStr = MODEM_CmdFrameBuff,
        .p_respStr = (uint8_t*)">", 
        .respFlag = MODEM_RESP_PROMPT,
        .retryCnt = 1,
        .timeout = 500, 
        .pfn_cmdFrameCb = MODEM_CmdFrame_Cb, 
        .pfn_respCb = NULL, 
        .pfn_errorCb = NULL,
        .pfn_retryCmpltCb = MODEM_CmdCIPSEND_RetryCmplt_Cb,
        .cmdNext = MODEM_CMD_CIPSEND_PH1
    },
    
    {
        .cmd = MODEM_CMD_CIPSEND_PH1, 
        .p_atStr = MODEM_ServerData1,//(uint8_t*)"## IMEI: 359586015829802, A;", 
        .p_respStr = (uint8_t*)NULL, 
        .respFlag = MODEM_RESP_OK,
        .retryCnt = 1,
        .timeout = 5000, 
        .pfn_cmdFrameCb = NULL, 
        .pfn_respCb = NULL, 
        .pfn_errorCb = NULL,
        .pfn_retryCmpltCb = MODEM_CmdCIPSEND_RetryCmplt_Cb,
        .cmdNext = MODEM_CMD_NIL
    },
    
    {
        .cmd = MODEM_CMD_CIPSEND_START_PH2, 
        .p_atStr = MODEM_CmdFrameBuff,
        .p_respStr = (uint8_t*)">", 
        .respFlag = MODEM_RESP_PROMPT,
        .retryCnt = 1,
        .timeout = 500, 
        .pfn_cmdFrameCb = MODEM_CmdFrame_Cb, 
        .pfn_respCb = NULL, 
        .pfn_errorCb = NULL,
        .pfn_retryCmpltCb = MODEM_CmdCIPSEND_RetryCmplt_Cb,
        .cmdNext = MODEM_CMD_CIPSEND_PH2
    },
    
    {
        .cmd = MODEM_CMD_CIPSEND_PH2, 
        .p_atStr = MODEM_ServerData2,//(uint8_t*)"IMEI: 359586015829802, tracker, 000,000,000.13554900601, L,;",
        .p_respStr = (uint8_t*)NULL, 
        .respFlag = MODEM_RESP_OK,
        .retryCnt = 1,
        .timeout = 5000, 
        .pfn_cmdFrameCb = NULL, 
        .pfn_respCb = NULL, 
        .pfn_errorCb = NULL,
        .pfn_retryCmpltCb = MODEM_CmdCIPSEND_RetryCmplt_Cb,
        .cmdNext = MODEM_CMD_NIL
    },
    
    {
        .cmd = MODEM_CMD_CSQ, 
        .p_atStr = (uint8_t*)"AT+CSQ\r", 
        .p_respStr = (uint8_t*)NULL, 
        .respFlag = MODEM_RESP_OK | MODEM_RESP_PROMPT,
        .retryCnt = MODEM_RETRY_FOREVER,
        .timeout = 3000, 
        .pfn_cmdFrameCb = NULL, 
        .pfn_respCb = MODEM_CmdCSQ_Resp_Cb, 
        .pfn_errorCb = NULL,
        .pfn_retryCmpltCb = NULL,
        .cmdNext = MODEM_CMD_NIL
    },
};

#define MODEM_INITCMDTABLE_SIZE     (sizeof(MODEM_CmdTable) / sizeof(MODEM_CmdTable[0]))

/* Extern variables ----------------------------------------------------------*/

void MODEM_Init(void)
{    
    MODEM_CmdRetryCnt = 0;
    MODEM_SignalQ = 99;
//    MODEM_CmdPriorityQTop = -1;
    MODEM_Cmd = MODEM_CMD_CMEE;
    MODEM_State = MODEM_STATE_GET_CMD;
    MODEM_InitFlag = false;
    MODEM_LOAD_RespRcvd = false;
    MODEM_ON_RespRcvd = false;
    
    SWTimer_Stop(&MODEM_TimerCmdTimeout);
    SWTimer_Stop(&MODEM_TimerSendData);
    SWTimer_Stop(&MODEM_TimerReadCSQ);
    
    MODEM_Reset();
    ATH_Init();
}

void MODEM_Reset(void)
{    
    swtimer_t timer;
    
    SWTimer_Stop(&timer);
	
	
	HAL_GPIO_WritePin(GSM_PWR_GPIO_Port, GSM_PWR_Pin, GPIO_PIN_SET);
     SWTimer_Start(&timer, 2200);

    while(SWTimer_GetStatus((&timer)) != SWTIMER_ELAPSED);
    
    HAL_GPIO_WritePin(GSM_RST_GPIO_Port, GSM_RST_Pin, GPIO_PIN_SET);
    SWTimer_Start(&timer, 3000);

    while(SWTimer_GetStatus((&timer)) != SWTIMER_ELAPSED);

    HAL_GPIO_WritePin(GSM_RST_GPIO_Port, GSM_RST_Pin, GPIO_PIN_RESET);
    SWTimer_Start(&timer, 3500);

    while(SWTimer_GetStatus((&timer)) != SWTIMER_ELAPSED);
}

void MODEM_Job(void)
{
    modem_cmd_t cmd;
    
    switch(MODEM_State)
    {
        case MODEM_STATE_GET_CMD:
            
            cmd = MODEM_CMD_NIL;
        
            if((MODEM_InitFlag == true) && (SWTimer_GetStatus(&MODEM_TimerReadCSQ) == SWTIMER_ELAPSED))
            {
                cmd = MODEM_CMD_CSQ;
                SWTimer_Start(&MODEM_TimerReadCSQ, CSQ_REQ_INTERVAL);
            }
            
            if((MODEM_ON_RespRcvd == true) && (SWTimer_GetStatus(&MODEM_TimerSendData) == SWTIMER_ELAPSED))
            {
                cmd = MODEM_CMD_CIPSEND_START_PH1;
                MODEM_ON_RespRcvd = false;
            }
            else if(MODEM_LOAD_RespRcvd == true)
            {
                cmd = MODEM_CMD_CIPSEND_START_PH2;
                SWTimer_Start(&MODEM_TimerSendData, DATA_SEND_INTERVAL); 
                MODEM_LOAD_RespRcvd = false;
            }

            if(cmd == MODEM_CMD_NIL)
            {
                cmd = MODEM_Cmd;
            }
            
            if(cmd != MODEM_CMD_NIL)
            {
                p_MODEM_CmdTable = MODEM_GetCmdTable(cmd);
                MODEM_State = MODEM_STATE_SEND_CMD;
                
                if(cmd == MODEM_CMD_CGATT_REQ)
                {
                    LED_Set(LED_RED, LED_OFF);
                }
                
                if((MODEM_InitFlag == false) && (cmd == MODEM_CMD_CIPSEND_START_PH1))
                {
                    MODEM_InitFlag = true;
                    SWTimer_Start(&MODEM_TimerReadCSQ, CSQ_REQ_INTERVAL);
                    SWTimer_Start(&MODEM_TimerSendData, DATA_SEND_INTERVAL);
                }
            }
            break;
            
        case MODEM_STATE_SEND_CMD:   
            
            if(p_MODEM_CmdTable != NULL)
            {
                if((SWTimer_GetStatus((&MODEM_TimerCmdTimeout)) == SWTIMER_STOPPED) &&
                    (ATH_GetTransmitStatus() == true))
                {
                    if(p_MODEM_CmdTable->pfn_cmdFrameCb != NULL)
                    {
                        p_MODEM_CmdTable->pfn_cmdFrameCb(p_MODEM_CmdTable->cmd);
                    }
                    ATH_Transmit((uint8_t*)p_MODEM_CmdTable->p_atStr, strlen((const char*)p_MODEM_CmdTable->p_atStr));
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
                    p_MODEM_CmdTable = NULL;
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
                                p_MODEM_CmdTable = NULL;
                                MODEM_State = MODEM_STATE_GET_CMD;
                            }
                        }
                    }
                }
            }
            else
            {
                p_MODEM_CmdTable = NULL;
                MODEM_State = MODEM_STATE_GET_CMD;
            }
            break;
    }
    
    // Process the modem AT response
    MODEM_ProcessReceivedData();
}

static void MODEM_ProcessReceivedData(void)
{
    uint8_t buff[300];
    uint16_t len;
//    uint16_t i;
    uint16_t error;
    
    len = ATH_Receive(buff);
    
    if(len != 0)
    {
//        i = 0;
//        while(i < len)
//        {
//            ITM_SendChar(buff[i++]);
//        }
//        ITM_SendChar('\r');
//        ITM_SendChar('\n');
        
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
        else if(MODEM_ResponseAwaitFlag & MODEM_RESP_OK)
        {
            if(strstr((const char*)buff, "OK") != 0)
            {
                MODEM_ResponseAwaitFlag &= ~MODEM_RESP_OK;
            }
        }
        else
        {
            // Do nothing
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
                    MODEM_State = MODEM_STATE_GET_CMD;
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
                    MODEM_State = MODEM_STATE_GET_CMD;
                }
            }
        }
        else if(strstr((const char*)buff, (const char*)"+CIPRCV") != 0)
        {
            if(strstr((const char*)buff, (const char*)"LOAD") != 0)
            {
                MODEM_LOAD_RespRcvd = true;
            }
            else if(strstr((const char*)buff, (const char*)"ON") != 0)
            {
                MODEM_ON_RespRcvd = true;
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
        MODEM_Cmd = p_MODEM_CmdTable->cmdNext;
        
        // dont wait for error response if actual response to command is available
        MODEM_ResponseAwaitFlag &= ~MODEM_RESP_PROMPT;
    }
}

static void MODEM_CmdCSQ_Resp_Cb(uint8_t* p_str)
{
    if(strstr((const char*)p_str, (const char*)"+CSQ") != 0)
    {
        if(p_str[7] == ',')
        {
            MODEM_SignalQ = p_str[6] - 0x30;
        }
        else
        {
            MODEM_SignalQ = ((p_str[6] - 0x30) * 10) + (p_str[7] - 0x30);
        }
        
        if(MODEM_SignalQ  == 99)
        {
            MODEM_Cmd = p_MODEM_CmdTable->cmdNext;
        }
        else
        {
            MODEM_Cmd = MODEM_Cmd;
        }
        
        // dont wait for error response if actual response to command is available
        MODEM_ResponseAwaitFlag &= ~MODEM_RESP_PROMPT;
    }
}

static void MODEM_CmdCGATT_Req_Resp_Cb(uint8_t* p_str)
{
    if(strstr((const char*)p_str, (const char*)"+CGATT:1") != 0)
    {
        MODEM_Cmd = MODEM_CMD_CGDCONT;
        
        // dont wait for error response if actual response to command is available
        MODEM_ResponseAwaitFlag &= ~MODEM_RESP_PROMPT;
    }
    else if(strstr((const char*)p_str, (const char*)"+CGATT:0") != 0)
    {
        MODEM_Cmd = MODEM_CMD_CGATT_ATTACH;
        
        // dont wait for error response if actual response to command is available
        MODEM_ResponseAwaitFlag &= ~MODEM_RESP_PROMPT;
    }
}

static void MODEM_CmdCGACT_Req_Resp_Cb(uint8_t* p_str)
{
    if(strstr((const char*)p_str, (const char*)"+CGACT: 1") != 0)
    {
        MODEM_Cmd = MODEM_CMD_CIFSR;
        
        // dont wait for error response if actual response to command is available
        MODEM_ResponseAwaitFlag &= ~MODEM_RESP_PROMPT;
    }
    else if(strstr((const char*)p_str, (const char*)"+CGACT: 0") != 0)
    {
        MODEM_Cmd = MODEM_CMD_CGACT_ACT;
        
        // dont wait for error response if actual response to command is available
        MODEM_ResponseAwaitFlag &= ~MODEM_RESP_PROMPT;
    }
}

static void MODEM_CmdCIFSR_RetryCmplt_Cb(void)
{
    MODEM_Cmd = MODEM_CMD_CGACT_DEACT;
}

static void MODEM_CmdCDNSGIP_Resp_Cb(uint8_t* p_str)
{
    if(strstr((const char*)p_str, (const char*)"+CDNSGIP") != 0)
    {
        strcpy((char*)MODEM_Onetpl_IpAddr, (const char*)p_str);
        MODEM_Cmd = MODEM_CMD_CIPSTART;
        
        // dont wait for error response if actual response to command is available
        MODEM_ResponseAwaitFlag &= ~MODEM_RESP_PROMPT;
    }
}

static void MODEM_CmdCIPSEND_RetryCmplt_Cb(void)
{
    MODEM_LOAD_RespRcvd = false;
    MODEM_ON_RespRcvd = true;
    SWTimer_Start(&MODEM_TimerSendData, DATA_SEND_INTERVAL);
    MODEM_Cmd = MODEM_CMD_NIL;
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

static void MODEM_CmdFrame_Cb(modem_cmd_t cmd)
{
    switch((uint16_t)cmd)
    {
        case MODEM_CMD_CIPSEND_START_PH1:
            sprintf((char*)MODEM_CmdFrameBuff, (const char*)"AT+CIPSEND=%d\r", strlen((const char*)MODEM_ServerData1));
            break;
        
        case MODEM_CMD_CIPSEND_PH1:
            sprintf((char*)MODEM_CmdFrameBuff, "%s", (const char*)MODEM_ServerData1);
            break;
        
        case MODEM_CMD_CIPSEND_START_PH2:
            sprintf((char*)MODEM_CmdFrameBuff, (const char*)"AT+CIPSEND=%d\r", strlen((const char*)MODEM_ServerData2));
            break;
        
        case MODEM_CMD_CIPSEND_PH2:
            sprintf((char*)MODEM_CmdFrameBuff, "%s", (const char*)MODEM_ServerData2);
            break;
    }
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

void MODEM_SetServerData1(char* p_str)
{
    strcpy((char*)MODEM_ServerData1, (const char*)p_str);
}

void MODEM_SetServerData2(char* p_str)
{
    strcpy((char*)MODEM_ServerData2, (const char*)p_str);
}
