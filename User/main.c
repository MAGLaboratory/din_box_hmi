/*
 *  main.c
 *
 *  This file was adopted a long time ago and spent a lot of time sitting in
 *  prototyping, so an accurate date of its modification is not available.
 *  Author: kirisaki
 */

/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2023/12/22
 * Description        : Main program body.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for 
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/

/*
 *@Note
 *This project is designed for the ch32v003JxMx SOP-8 package and the ch422 in
 *the SOP-16 package.
 *      Hardware connection:
 *            PC2 (6) -- SCL (5)
 *            PC1 (5) -- SDA (6)
 *
 * 			  PA1 (1) -- UTX_2
 * 			  PA2 (3) -- Dir
 * 			  (both buttons -> seconds)
 *
 * 			  PC4 (7) -- Relay Output
 *
 */

#include "debug.h"
#include "init.h"
#include "consts.h"
#include <PetitModbus.h>

/* Constants */

// Character LUT
// 0 1 2 3
// 4 5 6 7
// 8 9 A b
// C d E F
const u8 char_lut[] __attribute__((section(".text.consts")))=
{
	0b00111111, 0b00000110, 0b01011011, 0b01001111,
	0b01100110, 0b01101101,	0b01111101, 0b00000111,
	0b01111111, 0b01101111, 0b01110111, 0b01111100,
	0b00111001, 0b01011110, 0b01111001, 0b01110001
};

/* Macros */

/* Types */

/* Global variables */
volatile u32 t1_count = 0;
u32 last_t1_count = 0;
u8 loop_overrun = 0;
T_PETIT_MODBUS Petit;
volatile u8 modbus_arm = false;
volatile u32 modbus_timer;

/*********************************************************************
 * @fn      IIC_TX
 *
 * @brief   Transmits through I2C
 *
 * @return  none
 */
void IIC_TX(u8 addr, u8 data)
{
	u32 start_time = t1_count;
	while( I2C_GetFlagStatus( I2C1, I2C_FLAG_BUSY ) != RESET )
	{
		if (t1_count - start_time > 1U)
		{
			I2C_GenerateSTOP(I2C1, ENABLE);
			return;
		}
	}

	I2C_GenerateSTART( I2C1, ENABLE);

	while( !I2C_CheckEvent( I2C1, I2C_EVENT_MASTER_MODE_SELECT ) )
	{
		if (t1_count - start_time > 1U)
		{
			I2C_GenerateSTOP(I2C1, ENABLE);
			return;
		}
	}
	I2C_Send7bitAddress( I2C1, addr, I2C_Direction_Transmitter);

	while( !I2C_CheckEvent( I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED ) )
	{
		if (t1_count -  start_time > 1U)
		{
			I2C_GenerateSTOP(I2C1, ENABLE);
			return;
		}
	}

	I2C_SendData( I2C1, data);

	while( !I2C_CheckEvent( I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED ) )
	{
		if (t1_count -  start_time > 1U)
		{
			break;
		}
	}
	I2C_GenerateSTOP( I2C1, ENABLE);
}

/*********************************************************************
 * @fn      IIC_RX
 *
 * @brief   Receives through I2C
 *
 * @return  none
 */
void IIC_RX(u8 addr, u8 *data)
{
	u32 start_time = t1_count;
	while( I2C_GetFlagStatus( I2C1, I2C_FLAG_BUSY ) != RESET )
	{
		if (t1_count - start_time > 1U)
		{
			I2C_GenerateSTOP(I2C1, ENABLE);
			return;
		}
	}

	I2C_GenerateSTART( I2C1, ENABLE);

	while( !I2C_CheckEvent( I2C1, I2C_EVENT_MASTER_MODE_SELECT ) )
	{
		if (t1_count - start_time > 1U)
		{
			I2C_GenerateSTOP(I2C1, ENABLE);
			return;
		}
	}
	I2C_Send7bitAddress( I2C1, addr, I2C_Direction_Receiver);

	while (!I2C_CheckEvent (I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED))
	{
		if (t1_count - start_time > 1U)
		{
			I2C_GenerateSTOP(I2C1, ENABLE);
			return;
		}
	}
	while (I2C_GetFlagStatus(I2C1, I2C_FLAG_RXNE) == RESET)
	{
		if (t1_count - start_time > 1U)
		{
			I2C_GenerateSTOP(I2C1, ENABLE);
			return;
		}
	}
	I2C_AcknowledgeConfig(I2C1, DISABLE);

	*data = I2C_ReceiveData(I2C1);

	I2C_GenerateSTOP( I2C1, ENABLE);
}
/*********************************************************************
 * @fn      char_lut_fun
 *
 * @brief   Looks up the BCD in the character LUT
 *
 * @return  none
 */
u8 char_lut_fun(u8 d)
{
	if (d < 16)
	{
		return char_lut[d];
	}
	else
	{
		return 0;
	}
}

/*********************************************************************
 * @fn      write_digit
 *
 * @brief   Outputs the character data to the ch422
 *
 * @return  none
 */
void write_digit(u8 digit, u8 chd)
{
	u8 addr = 0;
	switch (digit)
	{
	case 0:
		addr = C_CH455_ADDR_W_IO0;
		break;
	case 1:
		addr = C_CH455_ADDR_W_IO1;
		break;
	case 2:
		addr = C_CH455_ADDR_W_IO2;
		break;
	case 3:
		addr = C_CH455_ADDR_W_IO3;
		break;
	default:
		return;
	}
	IIC_TX(addr, chd);
}

void PetitPortDirTx(void)
{
	GPIOA->BSHR = GPIO_Pin_2;
}

void PetitPortDirRx(void)
{
	GPIOA->BSHR = GPIO_Pin_2 << 16U;
}

void PetitUserTxBegin(pu8_t data)
{
	PetitPortDirTx();
	// output the first octet
	USART1->DATAR = data;
	USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
}

void PetitT15TimerStart(void)
{
	modbus_arm = true;
	modbus_timer = 0U;
}

void PetitT15TimerStop(void)
{
	modbus_arm = false;
}

/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main(void)
{
	u8 cur_disp[4U];
	SystemCoreClockUpdate();

	APP_GPIO_Init();

	TIME_Init();

	UART_Init();

	PETIT_MODBUS_Init(&Petit);
	Petit.Timer_Start = &PetitT15TimerStart;
	Petit.Timer_Stop = &PetitT15TimerStop;
	Petit.Tx_Begin = &PetitUserTxBegin;

	// start time
	TIM_Cmd(TIM1, ENABLE);
	TIM_ClearFlag(TIM1, TIM_FLAG_Update);

	//printf("IIC Host mode\r\n");
	IIC_Init(400000u, C_CH455_ADDR_SP);

	IIC_TX(C_CH455_ADDR_SP, C_MY_CH455_SP);
	while (1U)
	{
		u8 r = 0;
		u8 iic_act = false;

		// main loop timer overflow
		if (t1_count - last_t1_count != 0U)
		{
			loop_overrun = 1U;
		}

		// wait
		while (t1_count - last_t1_count == 0U)
		{
			__WFI();
		}

		// read keypresses every 4ms
		// get the key input every 4ms
		if ((t1_count & ((1U << 5U) - 1U)) == ((1U << 5U) - 1U))
		{
			IIC_RX(C_CH455_ADDR_I, &r);
			PetitInputRegisters[0U] = r;
			iic_act = true;
		}
		
		// process modbus
		PETIT_MODBUS_Process(&Petit);

		for(u8 i = 0; i < 4U; i++)
		{
			if (iic_act == true)
			{
				break;
			}
			// find out what to display (encoded in 7-segment)
			u8 disp = i & 0x1 ? PetitRegisters[i >> 1u] & ((1 << 8U) - 1U)
					: PetitRegisters[i >> 1u] >> 8U;
			// the display is updated on two conditions
			// the "clock" hits the display digit once a second
			// the requested display does not match what is currently displayed
			if ((t1_count & ((1U << 12U) - 1U)) == (i << 10U) 
					|| disp != cur_disp[i])
			{
				write_digit(i, disp);
				iic_act = true;
				cur_disp[i] = disp;
			}
		}

		// coil output
		// the upper 16 bits are for bit clear
		// the lower 16 are for bit setting
		GPIOC->BSHR = PetitCoils[0U] & 0x1U ? GPIO_Pin_4 : GPIO_Pin_4 << 16U;

		// increment by one to indicate one execution cycle
		last_t1_count += 1U;
	}
}
