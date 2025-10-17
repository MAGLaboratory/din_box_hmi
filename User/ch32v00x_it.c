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
#include <PetitModbus.h>
#include <PetitModbusPort.h>

extern T_PETIT_MODBUS Petit;

void NMI_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void HardFault_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void TIM1_UP_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void USART1_IRQHandler(void) __attribute__((interrput("WCH-Interrupt-fast")));

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

/*
 * timer 1 IRQ handler for the system ticking
 */

void TIM1_UP_IRQHandler(void)
{
    TIM_ClearFlag(TIM1, TIM_FLAG_Update);
    t1_count += 1U;
}

void USART1_IRQHandler(void)
{
	pu8_t tmp;
	if (USART_GetITStatus(USART1, USART_IT_TXE) == SET)
	{
		// disable the interrupt or add more data
		if (PetitTxBufferPop(&Petit, &tmp) != 0u)
		{
			USART1->DATAR = tmp;
		}
		else
		{
			// let the hardware complete sending if there is no more data
			// but disable this interrupt so it does not fire again
			USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
		}
	}
	if (USART_GetITStatus(USART1, USART_IT_TC) == SET)
	{
		// transmission complete
		USART_ClearITPendingBit(USART1, USART_IT_TC);
		PetitPortDirRx();
	}
	if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
	{
		// only cleared by reading from the receive data register
		PetitRxBufferInsert(&Petit, USART1->DATAR);
	}
}
