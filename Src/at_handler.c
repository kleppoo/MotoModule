/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "stm32f1xx_hal.h"
#include "at_handler.h"

/* Private variables ---------------------------------------------------------*/
static uint8_t AT_RxBuff[1024];
static uint8_t AT_TxBuff[512];
static uint16_t AT_RxBuffRdIdx;

static uint8_t GPS_RxBuff[1024];
static uint8_t GPS_TxBuff[512];
static uint16_t GPS_RxBuffRdIdx;

/* Extern variables ----------------------------------------------------------*/
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart1;

/* Private function prototypes -----------------------------------------------*/

void AT_HandlerInit(void)
{
    /* Disable the peripheral */
    __HAL_DMA_DISABLE(huart2.hdmarx);
    huart2.hdmarx->Instance->CMAR = (uint32_t)&AT_RxBuff;
    huart2.hdmarx->Instance->CPAR = (uint32_t)&huart2.Instance->DR;
    huart2.hdmarx->Instance->CNDTR = sizeof(AT_RxBuff);
    
    huart2.hdmatx->Instance->CMAR = (uint32_t)&AT_TxBuff;
    huart2.hdmatx->Instance->CPAR = (uint32_t)&huart2.Instance->DR;
    
    /* Enable the peripheral */
    __HAL_DMA_ENABLE(huart2.hdmarx);
    
    /* Enable the DMA transfer for the receiver request by setting the DMAR bit 
    in the UART CR3 register */
    SET_BIT(huart2.Instance->CR3, USART_CR3_DMAR);
    
    /* Enable the DMA transfer for transmit request by setting the DMAT bit
    in the UART CR3 register */
    SET_BIT(huart2.Instance->CR3, USART_CR3_DMAT);
    
    AT_RxBuffRdIdx = 0;
}

void GPS_HandlerInit(void)
{
    /* Disable the peripheral */
    __HAL_DMA_DISABLE(huart1.hdmarx);
    huart1.hdmarx->Instance->CMAR = (uint32_t)&GPS_RxBuff;
    huart1.hdmarx->Instance->CPAR = (uint32_t)&huart1.Instance->DR;
    huart1.hdmarx->Instance->CNDTR = sizeof(GPS_RxBuff);
    
    huart1.hdmatx->Instance->CMAR = (uint32_t)&GPS_TxBuff;
    huart1.hdmatx->Instance->CPAR = (uint32_t)&huart1.Instance->DR;
    
    /* Enable the peripheral */
    __HAL_DMA_ENABLE(huart1.hdmarx);
    
    /* Enable the DMA transfer for the receiver request by setting the DMAR bit 
    in the UART CR3 register */
    SET_BIT(huart1.Instance->CR3, USART_CR3_DMAR);
    
    /* Enable the DMA transfer for transmit request by setting the DMAT bit
    in the UART CR3 register */
    SET_BIT(huart1.Instance->CR3, USART_CR3_DMAT);
    
    GPS_RxBuffRdIdx = 0;
}





uint8_t AT_HandlerTransmit(uint8_t* p_inBuff, uint16_t len)
{
    uint8_t status;
    
    // Status by default set to failed
    status = 0;
    
    if(len < sizeof(AT_TxBuff))
    {
        if(huart2.hdmatx->Instance->CNDTR == 0)
        {
            /* Clear the TC flag in the SR register by writing 0 to it */
            CLEAR_BIT(huart2.Instance->SR, USART_SR_TC);

            memcpy((void*)AT_TxBuff, (const void*)p_inBuff, len);
            /* Disable the peripheral */
            __HAL_DMA_DISABLE(huart2.hdmatx);
  
            huart2.hdmatx->Instance->CNDTR = len;
            
            /* Enable the peripheral */
            __HAL_DMA_ENABLE(huart2.hdmatx); 
            
            status = 1;
        }
    }
    
    return status;
}

uint16_t AT_HandlerReceive(uint8_t* p_outBuff)
{
    uint8_t state;
    uint16_t rxBuffRdIdxMax;
    uint16_t i;
    uint16_t tempRxBuffRdIdx;
    
    state = 0;
    i = 0;
    rxBuffRdIdxMax = sizeof(AT_RxBuff) - huart2.hdmarx->Instance->CNDTR;
    tempRxBuffRdIdx = AT_RxBuffRdIdx;
    
    while(AT_RxBuffRdIdx != rxBuffRdIdxMax)
    {
        if((AT_RxBuff[AT_RxBuffRdIdx] == '\r') && ((state == 0) || (state == 2)))
        {
            state++;
        }        
        else if((AT_RxBuff[AT_RxBuffRdIdx] == '\n') && ((state == 1) || (state == 3)))
        {
            state++;
            
            if(i > 0)
            {
                if(AT_RxBuffRdIdx == sizeof(AT_RxBuff))
                {
                    AT_RxBuffRdIdx = 0;
                }
                break;
            }
        }
        else if(state == 2)
        {
            *p_outBuff = AT_RxBuff[AT_RxBuffRdIdx];
            p_outBuff++;
            i++;
        }
        else
        {
            
        }
        
        AT_RxBuffRdIdx++;
        
        if(AT_RxBuffRdIdx == sizeof(AT_RxBuff))
        {
            AT_RxBuffRdIdx = 0;
        }
        
        if(AT_RxBuffRdIdx == rxBuffRdIdxMax)
        {
            AT_RxBuffRdIdx = tempRxBuffRdIdx;
            i = 0;
            break;
        }
    }
    
    return i;
}


uint16_t GPS_HandlerReceive(uint8_t* p_outBuff)
{
    uint8_t state;
    uint16_t rxBuffRdIdxMax;
    uint16_t i;
    uint16_t tempRxBuffRdIdx;
    
    state = 0;
    i = 0;
    rxBuffRdIdxMax = sizeof(GPS_RxBuff) - huart1.hdmarx->Instance->CNDTR;
    tempRxBuffRdIdx = GPS_RxBuffRdIdx;
    
    while(GPS_RxBuffRdIdx != rxBuffRdIdxMax)
    {
        if((GPS_RxBuff[GPS_RxBuffRdIdx] == '\r') && ((state == 0) || (state == 2)))
        {
            state++;
        }        
        else if((GPS_RxBuff[GPS_RxBuffRdIdx] == '\n') && ((state == 1) || (state == 3)))
        {
            state++;
            if(i > 0)
            {
                if(GPS_RxBuffRdIdx == sizeof(GPS_RxBuff))
                {
                    GPS_RxBuffRdIdx = 0;
                }
                break;
            }
        }
        else if(state == 2)
        {
            *p_outBuff = GPS_RxBuff[GPS_RxBuffRdIdx];
            p_outBuff++;
            i++;
        }
        else
        {
            
        }
        
        GPS_RxBuffRdIdx++;
        
        if(GPS_RxBuffRdIdx == sizeof(GPS_RxBuff))
        {
            GPS_RxBuffRdIdx = 0;
        }
        
        if(GPS_RxBuffRdIdx == rxBuffRdIdxMax)
        {
            GPS_RxBuffRdIdx = tempRxBuffRdIdx;
            i = 0;
            break;
        }
    }
    return i;
}