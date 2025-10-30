// system parameter bitfield indices
#define C_CH455_ADDR_SP (0x48)
#define C_CH455_SP_KOFF (7u)
#define C_CH455_SP_INTENS (4u)
//  enables the interrupt pin
#define C_CH455_SP_7SEG (3u)
#define C_CH455_SP_SLEEP (2u)
#define C_CH455_SP_ENA (0u)

#define C_MY_CH455_SP ((2u << C_CH455_SP_INTENS) | (1 << C_CH455_SP_ENA))

// output data
#define C_CH455_ADDR_W_IO0 (0x68)
#define C_CH455_ADDR_W_IO1 (0x6A)
#define C_CH455_ADDR_W_IO2 (0x6C)
#define C_CH455_ADDR_W_IO3 (0x6E)

// input data
#define C_CH455_ADDR_I (0x4F)
#define C_CH455_I_KP (0x40)
#define C_CH455_I_UP  (0x1C)
#define C_CH455_I_LFT (0x36)
#define C_CH455_I_ENT (0x24)
#define C_CH455_I_RHT (0X16)
#define C_CH455_I_DWN (0x26)
#define C_CH455_I_BAK (0x1E)

// loop constants
#define C_SLOWER_CYCLE (4U)

// modbus timer constants 
#define C_MODBUS_CLEAR (4U)

// breakout board
//#define BOB = 1
// PCB
#define HMI_PCB = 1

#if defined(BOB) && defined(HMI_PCB)
#error "Only one board is supposed to be defined"
#endif // BOB && HMI_PCB

#if !defined(BOB) && !defined(HMI_PCB)
#error "At least one board type must be defined"
#endif

// debug pins
#if defined(BOB)
#define M_DBG_0_ACT() GPIOD->BSHR = GPIO_Pin_0;
#define M_DBG_0_INA() GPIOD->BSHR = GPIO_Pin_0 << 16U;
#define M_DBG_1_ACT() GPIOC->BSHR = GPIO_Pin_0;
#define M_DBG_1_INA() GPIOC->BSHR = GPIO_Pin_0 << 16U;
#define M_DBG_2_ACT() GPIOC->BSHR = GPIO_Pin_3;
#define M_DBG_2_INA() GPIOC->BSHR = GPIO_Pin_3 << 16U;
#define M_DBG_3_ACT() GPIOC->BSHR = GPIO_Pin_5;
#define M_DBG_3_INA() GPIOC->BSHR = GPIO_Pin_5 << 16U;
#define M_DBG_4_ACT() GPIOC->BSHR = GPIO_Pin_6;
#define M_DBG_4_INA() GPIOC->BSHR = GPIO_Pin_6 << 16U;
#define M_DBG_5_ACT() GPIOC->BSHR = GPIO_Pin_7;
#define M_DBG_5_INA() GPIOC->BSHR = GPIO_Pin_7 << 16U;
#define M_DBG_6_ACT() GPIOD->BSHR = GPIO_Pin_2;
#define M_DBG_6_INA() GPIOD->BSHR = GPIO_Pin_2 << 16U;
#define M_DBG_7_ACT() GPIOD->BSHR = GPIO_Pin_3;
#define M_DBG_7_INA() GPIOD->BSHR = GPIO_Pin_3 << 16U;
#endif // BOB

// debug output functions
#if defined(HMI_PCB)
#define M_T1_START()
#define M_T1_END()
#define M_USART_START()
#define M_USART_END()
#define M_LOOP_OVER()
#define M_MAIN_START()
#define M_MAIN_END()
#endif // HMI_PCB

#if defined(BOB)
#define M_T1_START() M_DBG_2_ACT()
#define M_T1_END() M_DBG_2_INA()
#define M_USART_START() M_DBG_1_ACT()
#define M_USART_END() M_DBG_1_INA()
#define M_LOOP_OVER() M_DBG_7_ACT()
#define M_MAIN_START() M_DBG_0_ACT()
#define M_MAIN_END() M_DBG_0_INA()
#endif // BOB