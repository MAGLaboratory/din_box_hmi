/*
 * init.c
 *
 *  Created on: Mar 27, 2024
 *      Author: brandon
 */
#include "init.h"

/*********************************************************************
 * @fn      IIC_Init
 *
 * @brief   Initializes the IIC peripheral.
 *
 * @return  none
 */
void IIC_Init(u32 bound, u16 address)
{
	GPIO_InitTypeDef GPIO_InitStructure =
	{ 0 };
	I2C_InitTypeDef I2C_InitTSturcture =
	{ 0 };

	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_I2C1, ENABLE);

	GPIOC->BSHR = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init( GPIOC, &GPIO_InitStructure);

	GPIOC->BSHR = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init( GPIOC, &GPIO_InitStructure);

	I2C_InitTSturcture.I2C_ClockSpeed = bound;
	I2C_InitTSturcture.I2C_Mode = I2C_Mode_I2C;
	I2C_InitTSturcture.I2C_DutyCycle = I2C_DutyCycle_16_9;
	I2C_InitTSturcture.I2C_OwnAddress1 = address;
	I2C_InitTSturcture.I2C_Ack = I2C_Ack_Enable;
	I2C_InitTSturcture.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_Init( I2C1, &I2C_InitTSturcture);

	I2C_Cmd( I2C1, ENABLE);
}

/*********************************************************************
 * @fn      UART_Init
 *
 * @brief   Initializes the UART peripheral
 *
 * @return  none
 */
void UART_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure = { 0 };
	USART_InitTypeDef USART_InitStructure = { 0 };

	RCC_APB2PeriphClockCmd(
		RCC_APB2Periph_USART1 |
		RCC_APB2Periph_GPIOD |
		RCC_APB2Periph_AFIO, ENABLE);

	// pin 1 with PD6 is used as the UART half-duplex pin
	// this pin is high by default.
	GPIOD->BSHR = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	GPIO_PinRemapConfig(GPIO_PartialRemap2_USART1, ENABLE);

	USART_InitStructure.USART_BaudRate = 38400;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl 
			= USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;

	USART_Init(USART1, &USART_InitStructure);
	USART_Cmd(USART1, ENABLE);

	USART_HalfDuplexCmd(USART1, ENABLE);

	// only RX interrupt for now
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

	NVIC_EnableIRQ(USART1_IRQn);
}

/*********************************************************************
 * @fn      APP_GPIO_Init
 *
 * @brief   Initializes GPIOs (buttons) for the application
 *
 * @return  none
 */
void APP_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(
			RCC_APB2Periph_GPIOA |
			RCC_APB2Periph_GPIOC , ENABLE);

	// Pin 1 is handled by the UART function
	
	// Pin 2 is the VSS pin

	// Pin 3: DIR (PA2)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// Pin 4 is the VDD pin
	//
	// Pin 5 is handled by the I2C function
	//
	// Pin 6 is handled by the I2C function
	//
	// Pin 7: OUT (PC4)
	// TODO: PWM?
	// Pin 7 is on T1CH4
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
}

/*********************************************************************
 * @fn      TIME_Init
 *
 * @brief   Initializes the timer for the 4 kHz system clock.
 *
 * The system clock should be 4 kHz in order to satisfy the minimum 250 us
 * common denominator from T_1.5 and T_3.5 on modbus.
 * {system clock} / {desired fcy} = {scaler}
 * 24e6 / 4e3 = 6 000
 * {prescaler} * {period} = {scale}
 * 60 * 100 / 6 000
 *
 * @return  none
 */
void TIME_Init(void)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

	TIM_DeInit(TIM1);

	// configure timebase
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Period = 100U - 1U;
	TIM_TimeBaseInitStructure.TIM_Prescaler = 60U - 1U;
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0U;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStructure);

	TIM_ARRPreloadConfig(TIM1, ENABLE);
	TIM_InternalClockConfig(TIM1);
	TIM_SelectOutputTrigger(TIM1, TIM_TRGOSource_Update);

	// configure interrupts
	TIM_ClearFlag(TIM1, TIM_FLAG_Update);
	TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);
	NVIC_EnableIRQ(TIM1_UP_IRQn);
}
