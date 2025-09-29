/********************************** (C) COPYRIGHT *******************************
 * File Name          : ch32v00x_it.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2023/12/22
 * Description        : Main Interrupt Service Routines.
*********************************************************************************
* Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
* Attention: This software (modified or not) and binary are used for 
* microcontroller manufactured by Nanjing Qinheng Microelectronics.
*******************************************************************************/
#include <ch32v00x_it.h>

void NMI_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM1_UP_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

/*********************************************************************
 * @fn      NMI_Handler
 *
 * @brief   This function handles NMI exception.
 *
 * @return  none
 */
void NMI_Handler(void)
{
  while (1)
  {
  }
}

/*********************************************************************
 * @fn      HardFault_Handler
 *
 * @brief   This function handles Hard Fault exception.
 *
 * @return  none
 */
void HardFault_Handler(void)
{
  while (1)
  {
  }
}

void TIM1_UP_IRQHandler(void)
{
	uint32_t shift = 0x80000000U;
	uint32_t mask = shift - 1U;
	uint16_t reload = (1U << 8U) - 1U;
    TIM_ClearFlag(TIM1, TIM_FLAG_Update);
    while (shift > 1U)
    {
    	mask = shift - 1U;
    	if (SEC_MASK & shift)
    	{
    		// perform the operation once per "shift" cycle
    		// the easiest way to do this is to mask it and compare it with the mask value
    		// it is also possible to compare this with 0
    		if ((t1_count & mask) == mask)
    		{
    			// adding to the reload register makes the period longer and
    			// therefore makes the clock slower
    			if (SEC_ADD)
    			{
    				reload += 1U;
    			}
    			else
    			{
    				reload -= 1U;
				}
    		}
    	}
    	shift >>= 1U;
    }
    TIM_SetAutoreload(TIM1, reload);
    t1_count += 1U;
}


