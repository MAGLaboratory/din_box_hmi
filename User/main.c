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
 * 			  PA1 (1) -- Minutes
 * 			  PA2 (3) -- Hours
 * 			  (both buttons -> seconds)
 *
 * 			  PC4 (7) -- 32.768 kHz oscillator
 *
 */

#include "debug.h"
#include "init.h"

/* Constants */

#define C_CH422_SP_SLEEP_OFF (7u)
#define C_CH422_SP_OD_EN_OFF (4u)
#define C_CH422_SP_A_SCAN_OFF (2u)
#define C_CH422_SP_IO_OE_OFF (0u)
#define C_CH422_SP_ADDR (0x48u)

#define C_CH422_OC_ADDR (0x46u)

#define C_CH422_W_IO0_ADDR (0x70)
#define C_CH422_W_IO1_ADDR (0x72)
#define C_CH422_W_IO2_ADDR (0x74)
#define C_CH422_W_IO3_ADDR (0x76)

#define C_CH422_R_IO_ADDR (0x4D)

#define C_I_SEC_ONES (5U)
#define C_I_SEC_TENS (4U)
#define C_I_MIN_ONES (3U)
#define C_I_MIN_TENS (2U)
#define C_I_HR_ONES (1U)
#define C_I_HR_TENS (0U)

#define C_NUM_DIG (4U)

#define C_CPS (1U << 8U)
#define C_B_HR (1U << 1U)
#define C_B_MIN (1U << 0U)
#define C_B_DEB (4U)
#define C_B_OFF (1U)
#define C_B_NUM (2U)
#define C_B_MASK ((1U << (C_B_NUM + 1)) - 1U)
#define C_LONG_PRESS (C_CPS * 3U)
#define C_SHORT_INTVL (1U << 6U)
#define C_REALLY_LONG (C_CPS * 60U * 60U * 1U)

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

const u8 SEC_ADD __attribute__((section(".text.consts"))) = 1U;
const u32 SEC_MASK __attribute__((section(".text.consts"))) = 0x00005800;

/* Macros */
#define C_MY_CH422_SYSTEM_PARAMETER \
(0 << C_CH422_SP_OD_EN_OFF |\
1 << C_CH422_SP_A_SCAN_OFF |\
1 << C_CH422_SP_IO_OE_OFF)

#define RE_MIN (((last_c_b_vec & C_B_MIN) == 0) && ((con_btn_vec & C_B_MIN) != 0))
#define RE_HOUR (((last_c_b_vec & C_B_HR) == 0) && ((con_btn_vec & C_B_HR) !=0))
#define MIN ((con_btn_vec & C_B_MIN) != 0)
#define HOUR ((con_btn_vec & C_B_HR) != 0)
#define BTN_TIMER_EXPIRE (t1_count - re_btn_count >= btn_dly_target)
#define HR_EVT (RE_HOUR || (HOUR && BTN_TIMER_EXPIRE))
#define MIN_EVT (RE_MIN || (MIN && BTN_TIMER_EXPIRE))

#define BCD_SEC (bcd[5])

/* Types */
typedef enum
{
	h_sm_idle = 0,
	h_sm_single,
	h_sm_double,
	h_sm_triple
} h_sm_t;

// seconds state machine
typedef enum
{
	s_sm_idle = 0,
	s_sm_hold,
	s_sm_disp
} s_sm_t;

typedef union
{
	struct
	{
		u8 btn_ct :6;
		s_sm_t state :2;
	} e;
	u8 b;
} ssm_t;

/* Global variables */
// todo: BCD to bitfields
u8 bcd[6] = { 0 };
u8 chd[4] = { 0 };
u8 last_upd_bcd = 0;
u8 last_chd[4] = { 0 };
u8 hr_dir = 0;
volatile u32 t1_count = 0;
ssm_t ssmOut;
u32 ssm_ct;

h_sm_t hr_sm = h_sm_idle;

u32 last_exloop_count = 0;
u32 last_sec = 0; // there are a number of t1 counts per second
u8 exloop_overrun = 0;
u8 btn_vec = 0; // used by the debouncer
u8 last_btn_vec = 0; // used by the debouncer
u8 con_btn_vec = 0;	// used by the rising edge logic
u8 last_c_b_vec = 0; // used by the rising edge logic
u32 deb_btn_count = 0; // used by the debouncer
u32 re_btn_count = 0; // used by the button down timer
u32 btn_dly_target = 0; // used by the button down timer
u32 hr_dir_count = 0U - C_REALLY_LONG; // used by the DST hour reversal functionality
u8 auto_seconds = 0; // automatic seconds display
u8 pwr_loss_ind = 1U; // indicate that the power was lost on initial startup
u8 twelve_hour = 0;
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
		if (t1_count > start_time)
		{
			I2C_GenerateSTOP(I2C1, ENABLE);
			return;
		}
	}

	I2C_GenerateSTART( I2C1, ENABLE);

	while( !I2C_CheckEvent( I2C1, I2C_EVENT_MASTER_MODE_SELECT ) )
	{
		if (t1_count > start_time)
		{
			I2C_GenerateSTOP(I2C1, ENABLE);
			return;
		}
	}
	I2C_Send7bitAddress( I2C1, addr, I2C_Direction_Transmitter);

	while( !I2C_CheckEvent( I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED ) )
	{
		if (t1_count > start_time)
		{
			I2C_GenerateSTOP(I2C1, ENABLE);
			return;
		}
	}

	I2C_SendData( I2C1, data);

	while( !I2C_CheckEvent( I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED ) )
	{
		if (t1_count > start_time)
		{
			break;
		}
	}
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
		addr = C_CH422_W_IO0_ADDR;
		break;
	case 1:
		addr = C_CH422_W_IO1_ADDR;
		break;
	case 2:
		addr = C_CH422_W_IO2_ADDR;
		break;
	case 3:
		addr = C_CH422_W_IO3_ADDR;
		break;
	default:
		return;
	}
	IIC_TX(addr, chd);
}

/*********************************************************************
 * @fn      nof_hour
 *
 * @brief   Natural Overflow Hour
 *
 * @return  none
 */
void nof_hour(void)
{
	if (bcd[C_I_HR_ONES] >= 10U)
	{
		bcd[C_I_HR_TENS] += 1U;
		bcd[C_I_HR_ONES] -= 10U;
	}

	if (twelve_hour == 0)
	{
		if (bcd[C_I_HR_TENS] >= 2 && bcd[C_I_HR_ONES] >= 4)
		{
			bcd[C_I_HR_TENS] = 0U;
			bcd[C_I_HR_ONES] = 0U;
		}
	}
	else
	{
		// 13 -> 01
		if (bcd[C_I_HR_TENS] >= 1 && bcd[C_I_HR_ONES] >= 3)
		{
			bcd[C_I_HR_TENS] -= 1U;
			bcd[C_I_HR_ONES] -= 2U;
		}
		// 20 -> 08
		else if (bcd[C_I_HR_TENS] == 2U && bcd[C_I_HR_ONES] == 0)
		{
			bcd[C_I_HR_TENS] = 0;
			bcd[C_I_HR_ONES] = 8U;
		}
		// 21 -> 09
		else if (bcd[C_I_HR_TENS] == 2U && bcd[C_I_HR_ONES] == 1U)
		{
			bcd[C_I_HR_TENS] = 0;
			bcd[C_I_HR_ONES] = 9U;
		}
		// 00 -> 12
		else if (bcd[C_I_HR_TENS] == 0 && bcd[C_I_HR_ONES] == 0)
		{
			bcd[C_I_HR_TENS] = 1U;
			bcd[C_I_HR_ONES] = 2U;
		}
	}
}

/*********************************************************************
 * @fn      nof_min_sec
 *
 * @brief   Natural Overflow Minutes and Seconds
 *
 * This function implements natural overflow of minutes and seconds.
 * The function is mostly implemented using the language of minutes, but most
 * of the logic works the same for seconds.
 * @return  none
 */
void nof_min_sec(u8 sec, u8 inh)
{
	if (bcd[C_I_MIN_ONES + sec * 2U] >= 10U)
	{
		bcd[C_I_MIN_TENS + sec * 2U] += 1U;
		bcd[C_I_MIN_ONES + sec * 2U] -= 10U;
	}

	// 60 minutes, 60 seconds
	if (bcd[C_I_MIN_TENS + sec * 2U] >= 6U)
	{
		// increment the next digit or not
		if (!inh)
		{
			bcd[C_I_HR_ONES + sec * 2U] += 1U;
		}
		bcd[C_I_MIN_TENS + sec * 2U] = 0;
	}
}

/*********************************************************************
 * @fn      dec_hr
 *
 * @brief   Decrement Hour (by one)
 *
 * @return  none
 */
void dec_hr(void)
{
	if (twelve_hour == 0)
	{
		if (bcd[C_I_HR_ONES] == 0)
		{
			if (bcd[C_I_HR_TENS] == 0) // 00 -> 23
			{
				bcd[C_I_HR_TENS] = 2;
				bcd[C_I_HR_ONES] = 3;
			}
			else if (bcd[C_I_HR_TENS] == 1) // 10 -> 09
			{
				bcd[C_I_HR_TENS] = 0;
				bcd[C_I_HR_ONES] = 9;
			}
			else if (bcd[C_I_HR_TENS] == 2) // 20 -> 19
			{
				bcd[C_I_HR_TENS] = 1;
				bcd[C_I_HR_ONES] = 9;
			}
		}
		else
		{
			// base case, no tens underflow
			bcd[1] -= 1;
		}
	}
	else
	{
		// 01 -> 12
		if (bcd[C_I_HR_TENS] == 0 && bcd[C_I_HR_ONES] == 1U)
		{
			bcd[C_I_HR_TENS] = 1U;
			bcd[C_I_HR_ONES] = 2U;
		}
		// 10 -> 09
		else if(bcd[C_I_HR_TENS] == 1U && bcd[C_I_HR_ONES] == 0U)
		{
			bcd[C_I_HR_TENS] = 0;
			bcd[C_I_HR_ONES] = 9U;
		}
		else
		{
			bcd[C_I_HR_ONES] -= 1U;
		}
	}
}

/*********************************************************************
 * @fn      change_hr
 *
 * @brief   changes the hour based on the hour direction variable
 *
 * @return  none
 */
void change_hr(void)
{
	if (hr_dir == 0)
	{
		bcd[1] += 1;
		nof_hour();
	}
	else
	{
		dec_hr();
	}
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
	u8 i = 0;
	SystemCoreClockUpdate();

	APP_GPIO_Init();

	TIME_Init();

	//printf("IIC Host mode\r\n");
	IIC_Init(40000, C_CH422_SP_ADDR);

	IIC_TX(C_CH422_SP_ADDR, C_MY_CH422_SYSTEM_PARAMETER);

	// start time
	TIM_Cmd(TIM1, ENABLE);
	while (1U)
	{
		u8 disp_sec = 0;

		// main loop timer overflow
		if (t1_count - last_exloop_count != 0U)
		{
			exloop_overrun = 1U;
		}

		// wait
		while (t1_count - last_exloop_count == 0U)
		{
			__WFI();
		}

		// button vector filling
		// (it is a bit more efficient because the bits are correctly ordered in hardware)
		btn_vec = ~(GPIO_ReadInputData(GPIOA) >> C_B_OFF) & C_B_MASK;

		// debouncing
		if (btn_vec != last_btn_vec)
		{
			deb_btn_count = t1_count;
		}
		if (t1_count - deb_btn_count >= C_B_DEB)
		{
			deb_btn_count = t1_count - C_B_DEB;
			con_btn_vec = btn_vec;
		}

		// rising edge to button press tracking
		if (RE_MIN || RE_HOUR)
		{
			re_btn_count = t1_count;
			btn_dly_target = C_LONG_PRESS;
			pwr_loss_ind = 0U;
		}

		// natural overflow
		// this takes the excess sections computed later in the loop
		// and rolls it over to other digits in the loop
		// seconds
		nof_min_sec(1, 0);
		// minutes
		nof_min_sec(0, 0);
		// hours
		nof_hour();

		// seconds button state machine
		// transition
		switch (ssmOut.e.state)
		{
			case s_sm_idle:
			if (HOUR && MIN)
			{
				ssm_ct = t1_count;
				ssmOut.e.state = s_sm_hold;
				ssmOut.e.btn_ct = 1U;
			}
			break;
			case s_sm_hold:
			if (HOUR && MIN && BTN_TIMER_EXPIRE)
			{
				re_btn_count = t1_count - C_LONG_PRESS;
				last_sec = t1_count;
				bcd[C_I_SEC_ONES] = 0;
				bcd[C_I_SEC_TENS] = 0;
			}
			if (t1_count - ssm_ct >= C_LONG_PRESS)
			{
				ssm_ct = t1_count - C_LONG_PRESS;
			}
			// either button is released
			if (!HOUR || !MIN)
			{
				if (t1_count - ssm_ct >= C_LONG_PRESS)
				{
					ssmOut.e.state = s_sm_idle;
				}
				else
				{
					ssmOut.e.state = s_sm_disp;
				}
			}
			break;
			case s_sm_disp:
			// the seconds button is a combination of both hours and minutes buttons
			if (RE_HOUR && RE_MIN)
			{
				// set to a really big number to avoid overflowing
				if (ssmOut.e.btn_ct < ((1U << 6U) - 1U))
				{
					ssmOut.e.btn_ct += 1U;
				}
				ssm_ct = t1_count;
			}
			else if (RE_HOUR || RE_MIN)
			{
				ssmOut.e.state = s_sm_idle;
				ssmOut.e.btn_ct = 0;
			}
			// timeout seconds display
			// and apply selected setting
			if (t1_count - ssm_ct >= C_LONG_PRESS)
			{
				ssmOut.e.state = s_sm_idle;
				if (ssmOut.e.btn_ct == 2U)
				{
					auto_seconds ^= 1U;
				}
				if (ssmOut.e.btn_ct == 4U)
				{
					twelve_hour ^= 1U;
				}
				ssmOut.e.btn_ct = 0;
			}
			break;
		}

		// processing of the seconds button takes precedence over
		// other buttons because the seconds button is essentially
		// a combination of both hour and minute buttons
		if (ssmOut.e.state != s_sm_idle)
		{
			disp_sec = 1U;
		}
		else
		{
			// process "hour events" which have their own
			// state machine
			if (HR_EVT)
			{
				auto_seconds = 0;
				switch (hr_sm)
				{
				case h_sm_idle:
					change_hr();
					hr_dir_count = t1_count;
					hr_sm = h_sm_single;
					break;
				case h_sm_single:
					// reverse hour direction
					hr_dir ^= 1U;
					change_hr();
					change_hr();
					hr_dir_count = t1_count;
					hr_sm = h_sm_double;
					break;
				case h_sm_double:
					// set hour direction to increment
					if (hr_dir)
					{
						hr_dir = 0;
						change_hr();
						change_hr();
						change_hr();
					}
					else
					{
						change_hr();
					}
					hr_sm = h_sm_triple;
					hr_dir_count = t1_count;
					break;
				case h_sm_triple:
					// subsequent hour increments
					change_hr();
					hr_dir_count = t1_count;
				}
				if (BTN_TIMER_EXPIRE)
				{
					btn_dly_target = C_SHORT_INTVL;
					re_btn_count = t1_count;
				}
			}

			// process minutes
			if (MIN_EVT)
			{
				bcd[C_I_MIN_ONES] += 1U;
				nof_min_sec(0, 1U);
				auto_seconds = 0;
				if (BTN_TIMER_EXPIRE)
				{
					btn_dly_target = C_SHORT_INTVL;
					re_btn_count = t1_count;
				}
			}
		}

		// process the only transitions in the hour state machine that are based on time
		if (t1_count - hr_dir_count >= C_REALLY_LONG)
		{
			// overflow prevention? is it necessary?
			hr_dir_count = t1_count - C_REALLY_LONG;
			hr_sm = h_sm_idle;
		}

		// encode the 7-segment display data
		// display seconds.
		// the decimal points display the seconds button count for clock configuration
		// "auto seconds" display seconds for the first 0 to 4 (fine total) seconds
		if (disp_sec || (auto_seconds && (BCD_SEC < 5U)))
		{
			for (i = 0; i < C_NUM_DIG; i++)
			{
				chd[i] = (ssmOut.e.btn_ct == (5U - i)) ? 0x80 : 0;
			}
			chd[2U] |= char_lut_fun(bcd[C_I_SEC_TENS] & 0xF);
			chd[3U] |= char_lut_fun(bcd[C_I_SEC_ONES] & 0xF);
		}
		else
		{
			if (pwr_loss_ind && (BCD_SEC & 0x1))
			{
				for (i = 0; i < C_NUM_DIG; i++)
				{
					chd[i] = 0;
				}
			}
			else
			{
				for (i = 0; i < C_NUM_DIG; i++)
				{
					// leading 0 is omitted in twelve hour mode
					if (twelve_hour != 0 && i == C_I_HR_TENS && bcd[i] == 0)
					{
						chd[i] = 0;
					}
					else
					{
						chd[i] = char_lut_fun(bcd[i] & 0xF);
					}
				}
				chd[1U] |= (exloop_overrun ? 0x80 : 0);
				chd[3U] |= (auto_seconds ? 0x80 : 0);
			}
		}

		// send 7-segment display data
		for (i = 0; i < 4U; i++)
		{
			// if the character changed or our "clock pointer"
			if (last_chd[i] != chd[i] || (BCD_SEC == i && BCD_SEC != last_upd_bcd))
			{
				write_digit(i, chd[i]);
				last_chd[i] = chd[i];
				last_upd_bcd = BCD_SEC;
			}
		}

		// increment the seconds based on t1
		if (t1_count - last_sec >= C_CPS)
		{
			BCD_SEC += 1U;
			last_sec += C_CPS;
		}

		// store the "last" values
		last_btn_vec = btn_vec;
		last_c_b_vec = con_btn_vec;
		// increment by one to indicate one execution cycle
		last_exloop_count += 1U;
	}
}
