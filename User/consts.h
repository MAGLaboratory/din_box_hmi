
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
