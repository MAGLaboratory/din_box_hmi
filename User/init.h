/*
 * init.h
 *
 *  Created on: Mar 27, 2024
 *      Author: brandon
 */

#ifndef USER_INIT_H_
#define USER_INIT_H_

#include "debug.h"

void IIC_Init(u32 bound, u16 address);
void APP_GPIO_Init(void);
void TIME_Init(void);

#endif /* USER_INIT_H_ */
