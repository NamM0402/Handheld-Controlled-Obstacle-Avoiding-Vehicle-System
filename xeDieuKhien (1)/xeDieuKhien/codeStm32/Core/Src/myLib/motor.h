/*
 * motor.h
 */

#ifndef _MOTOR_H_
#define _MOTOR_H_

#include "stm32f4xx_hal.h"

#include "tim.h"
#include "gpio.h"

#define PWM_RESOLUTION 8400  // ARR + 1

void TIM1_SetDuty_CH2(uint8_t duty_percent);
void TIM1_SetDuty_CH4(uint8_t duty_percent);
void motor_init();
void PWM_RampUpDown(void);
void Forward();
void Backward();
void TurnLeft();
void TurnRight();

void blynkLed(uint16_t count);

#endif /* SRC_MYLIB_MOTOR_H_ */
