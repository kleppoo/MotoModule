/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <string.h>
#include "stm32f1xx_hal.h"
#include "at_handler.h"

/* Private variables ---------------------------------------------------------*/
uint8_t AT_RxBuff[1024];
uint8_t AT_TxBuff[512];
uint16_t AT_RxBuffRdIdx;

/* Extern variables ----------------------------------------------------------*/
extern UART_HandleTypeDef huart2;

/* Private function prototypes -----------------------------------------------*/

void ATH_Init(void)
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

uint8_t ATH_Transmit(uint8_t* p_inBuff, uint16_t len)
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

bool ATH_GetTransmitStatus(void)
{
    bool status;
    
    // Status by default set to failed
    status = false;

    if(huart2.hdmatx->Instance->CNDTR == 0)
    {   
        status = true;
    }
    
    return status;
}

uint16_t ATH_Receive(uint8_t* p_outBuff)
{
    uint16_t rxBuffRdIdxMax;
    uint16_t i;
    uint16_t tempRxBuffRdIdx;
    
    i = 0;
    rxBuffRdIdxMax = sizeof(AT_RxBuff) - huart2.hdmarx->Instance->CNDTR;
    tempRxBuffRdIdx = AT_RxBuffRdIdx;
    
    while(AT_RxBuffRdIdx != rxBuffRdIdxMax)
    {
        if(AT_RxBuff[AT_RxBuffRdIdx] == '\r')
        {
            if(i > 0)
            {
                if(AT_RxBuffRdIdx == sizeof(AT_RxBuff))
                {
                    AT_RxBuffRdIdx = 0;
                }
                break;
            }
        }        
        else if(AT_RxBuff[AT_RxBuffRdIdx] == '\n')
        {

        }
        else
        {
            p_outBuff[i] = AT_RxBuff[AT_RxBuffRdIdx];
            i++;
        }
        
        AT_RxBuffRdIdx++;
        
        if(AT_RxBuffRdIdx == sizeof(AT_RxBuff))
        {
            AT_RxBuffRdIdx = 0;
        }
        
        if(AT_RxBuffRdIdx == rxBuffRdIdxMax)
        {
            // Sepcial handling for CIPSEND
            if(p_outBuff[0] != '>')
            {
                AT_RxBuffRdIdx = tempRxBuffRdIdx;
                i = 0;
            }
            break;
        }
    }
    
    p_outBuff[i] = 0;
    return i;
}
