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
void USART1_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));

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
	M_T1_START();
    TIM_ClearFlag(TIM1, TIM_FLAG_Update);
    t1_count += 1U;
	// modbus timer implementation
	if (modbus_arm == true 
			&& (t1_count - modbus_timer) >= C_MODBUS_CLEAR)

	{
		PetitRxBufferReset(&Petit);
		modbus_arm = false;
	}
	M_T1_END();
	return;
}

/*
 * USART1_IRQHandler
 */
void USART1_IRQHandler(void)
{
	M_USART_START();
	pu8_t tmp;
	if (USART1->STATR & USART_STATR_TC)
	{
		// clear bit
		USART1->STATR = ~USART_STATR_TC;
		// disable the interrupt or add more data
		if (PetitTxBufferPop(&Petit, &tmp) != 0u)
		{
			USART1->DATAR = tmp;
		}
		else
		{
			PetitPortDirRx();
		}
	}
	if (USART1->STATR & USART_STATR_RXNE)
	{
		USART1->STATR = ~USART_STATR_RXNE;
		tmp = USART1->DATAR;
		PetitRxBufferInsert(&Petit, tmp);
	}
	M_USART_END();
	return;
}
