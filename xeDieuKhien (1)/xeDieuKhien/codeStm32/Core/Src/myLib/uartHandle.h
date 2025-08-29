

#ifndef _UARTHANDLE_H_
#define _UARTHANDLE_H_
#include "stm32f4xx_hal.h"
#include "gpio.h"
#include "usart.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

#define APP_FRAME_START '@'
#define APP_FRAME_STOP '#'

typedef struct
{
	char char_data_rec;
	char arr_data_rec[300];
	bool data_done_frame;
	uint16_t arr_data_index;
	bool en_get_char_data;
}Uart_debug;
extern Uart_debug my_debug;
void uartAppHandle(Uart_debug *myUart);
void uartAppClear(Uart_debug *myUart);
bool trim(char *source,char *des,char key_start[30],char key_stop[30]);
bool trim_with_length(char *source,char *des,char key_start[30],unsigned int length);





#endif /* SRC_MYLIB_UARTHANDLE_H_ */
