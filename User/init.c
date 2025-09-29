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

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init( GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
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
 * @fn      APP_GPIO_Init
 *
 * @brief   Initializes GPIOs (buttons) for the application
 *
 * @return  none
 */
void APP_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOD, ENABLE);

	// PA1 (minutes)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// PA2 (hours)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// PD6 just in case (pin shared with PA1)
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
}

/*********************************************************************
 * @fn      TIME_Init
 *
 * @brief   Initializes the timer for the 32,768Hz clock.
 *
 * This funciton initializes the timer for the 32,768Hz clock.
 * It uses the T1 edge detector for both rising and falling edges so
 * the timer actually increments at a rate of 65,536Hz.
 * This is divided by the period configured as 256 to create a system
 * tick on t1_count that increments at 256Hz.
 *
 * @return  none
 */
void TIME_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;

	// configure clock tree
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOC, ENABLE);

	// configure timer input
	GPIO_PinRemapConfig(GPIO_FullRemap_TIM1, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init( GPIOC, &GPIO_InitStructure);

	TIM_DeInit(TIM1);

	// configure timebase
	TIM_TimeBaseInitStructure.TIM_ClockDivision = 0U;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_Period = (1U << 8U) - 1U; // resets after this numbers
	TIM_TimeBaseInitStructure.TIM_Prescaler = 0U;
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0U;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStructure);

	// Auto Reload Register Shadow Register
	//TIM_ARRPreloadConfig(TIM1, ENABLE);

	// configure clock source
	TIM_TIxExternalClockConfig(TIM1, TIM_TIxExternalCLK1Source_TI1ED,
			TIM_ICPolarity_BothEdge, 0x0);

	// configure interrupts
	TIM_ClearFlag(TIM1, TIM_FLAG_Update);
	TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);
	NVIC_EnableIRQ(TIM1_UP_IRQn);
}
