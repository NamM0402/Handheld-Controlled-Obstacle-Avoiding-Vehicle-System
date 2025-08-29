/*
 * motor.c
 *
 *  Created on: May 26, 2025
 *      Author: maiva
 */

#include "motor.h"

void TIM1_SetDuty_CH2(uint8_t duty_percent)
{
	duty_percent=100-duty_percent;
    if (duty_percent > 100) duty_percent = 100;
    TIM1->CCR2 = (PWM_RESOLUTION * duty_percent) / 100;
}

void TIM1_SetDuty_CH4(uint8_t duty_percent)
{
	duty_percent=100-duty_percent;
    if (duty_percent > 100) duty_percent = 100;
    TIM1->CCR4 = (PWM_RESOLUTION * duty_percent) / 100;
}


void motor_init()
{
  //tien
  HAL_GPIO_WritePin(motor_left_dir_GPIO_Port, motor_left_dir_Pin, 0); //trái quay tiến
  HAL_GPIO_WritePin(motor_right_dir_GPIO_Port, motor_right_dir_Pin, 1); //phải quay tiến

  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);

  // Set PWM duty
  TIM1_SetDuty_CH2(0);  //duty stop
  TIM1_SetDuty_CH4(0);  //duty stop
}


void PWM_RampUpDown(void)
{
    uint8_t duty=30;
	TIM1_SetDuty_CH2(duty);
	TIM1_SetDuty_CH4(duty);

    // Đảm bảo về 0 sau khi kết thúc
    //TIM1_SetDuty_CH2(0);
    //TIM1_SetDuty_CH4(0);
}

void Forward()
{
    // Cả 2 bánh tiến
    HAL_GPIO_WritePin(motor_left_dir_GPIO_Port, motor_left_dir_Pin, GPIO_PIN_RESET);   // trái tiến (EN=0)
    HAL_GPIO_WritePin(motor_right_dir_GPIO_Port, motor_right_dir_Pin, GPIO_PIN_SET);   // phải tiến (EN=1)
    PWM_RampUpDown();
}

void Backward()
{
    // Cả 2 bánh lùi
    HAL_GPIO_WritePin(motor_left_dir_GPIO_Port, motor_left_dir_Pin, GPIO_PIN_SET);     // trái lùi (EN=1)
    HAL_GPIO_WritePin(motor_right_dir_GPIO_Port, motor_right_dir_Pin, GPIO_PIN_RESET); // phải lùi (EN=0)
    PWM_RampUpDown();
}

void TurnLeft()
{
    // Bánh trái lùi, bánh phải tiến
    HAL_GPIO_WritePin(motor_left_dir_GPIO_Port, motor_left_dir_Pin, GPIO_PIN_SET);     // trái lùi (EN=1)
    HAL_GPIO_WritePin(motor_right_dir_GPIO_Port, motor_right_dir_Pin, GPIO_PIN_SET);   // phải tiến (EN=1)
    PWM_RampUpDown();
}

void TurnRight()
{
    // Bánh trái tiến, bánh phải lùi
    HAL_GPIO_WritePin(motor_left_dir_GPIO_Port, motor_left_dir_Pin, GPIO_PIN_RESET);   // trái tiến (EN=0)
    HAL_GPIO_WritePin(motor_right_dir_GPIO_Port, motor_right_dir_Pin, GPIO_PIN_RESET); // phải lùi (EN=0)
    PWM_RampUpDown();
}

void blynkLed(uint16_t count)
{
	for(int i=0;i<count;i++)
	{
		HAL_GPIO_WritePin(LED_MODE_GPIO_Port, LED_MODE_Pin, 1);
		HAL_Delay(50);
		HAL_GPIO_WritePin(LED_MODE_GPIO_Port, LED_MODE_Pin, 0);
	}
}























