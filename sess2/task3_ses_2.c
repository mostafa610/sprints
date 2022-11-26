/*
 * FreeRTOS Kernel V10.2.0
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/* 
	NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
	The processor MUST be in supervisor mode when vTaskStartScheduler is 
	called.  The demo applications included in the FreeRTOS.org download switch
	to supervisor mode prior to main being called.  If you are not using one of
	these demo application projects then ensure Supervisor mode is used.
*/


/*
 * Creates all the demo application tasks, then starts the scheduler.  The WEB
 * documentation provides more details of the demo application tasks.
 * 
 * Main.c also creates a task called "Check".  This only executes every three 
 * seconds but has the highest priority so is guaranteed to get processor time.  
 * Its main function is to check that all the other tasks are still operational.
 * Each task (other than the "flash" tasks) maintains a unique count that is 
 * incremented each time the task successfully completes its function.  Should 
 * any error occur within such a task the count is permanently halted.  The 
 * check task inspects the count of each task to ensure it has changed since
 * the last time the check task executed.  If all the count variables have 
 * changed all the tasks are still executing error free, and the check task
 * toggles the onboard LED.  Should any task contain an error at any time 
 * the LED toggle rate will change from 3 seconds to 500ms.
 *
 */

/* Standard includes. */
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "lpc21xx.h"
#include "semphr.h"
#include "queue.h"

/* Peripheral includes. */
#include "serial.h"
#include "GPIO.h"


/*-----------------------------------------------------------*/

/* Constants to setup I/O and processor. */
#define mainBUS_CLK_FULL	( ( unsigned char ) 0x01 )

/* Constants for the ComTest demo application tasks. */
#define mainCOM_TEST_BAUD_RATE	( ( unsigned long ) 115200 )


/*
 * Configure the processor for use with the Keil demo board.  This is very
 * minimal as most of the setup is managed by the settings in the project
 * file.
 */
SemaphoreHandle_t xSemaphore;
QueueHandle_t xQueue1;
typedef struct massage{
	int32_t size;
	char massage[20];
}massage;
pinState_t button_1_status = PIN_IS_LOW;
pinState_t button_2_status = PIN_IS_LOW;

static void prvSetupHardware( void );
/*-----------------------------------------------------------*/

void vTaskButton_1_event( void * pvParameters ) {	
	massage button1_mass={10,"b1_rising "};
	massage rec;
	for( ;; ) {
		// check for positive edge
		if(button_1_status == PIN_IS_LOW){
			while(button_1_status == PIN_IS_LOW){
				button_1_status =  GPIO_read(PORT_0, PIN0);
			}
			// rising edge occeured
			button1_mass.size = 10;
			strcpy(button1_mass.massage, "b1_rising ");	
			// will be blocked till it can send
			xQueueSend( xQueue1, ( void * ) &button1_mass,   ( TickType_t ) portMAX_DELAY);
			
		} else {//check for faling edge
			while(button_1_status == PIN_IS_HIGH){
				button_1_status =  GPIO_read(PORT_0, PIN0);
			}
			button1_mass.size = 11;
			strcpy(button1_mass.massage, "b1_falling ");
			xQueueSend( xQueue1, ( void * ) &button1_mass,  ( TickType_t ) portMAX_DELAY );
		}
		vTaskDelay( 100 );			
	}
}

void vTaskButton_2_event( void * pvParameters ) {	
  massage button2_mass;
	for( ;; ) {
		// check for positive edge
		if(button_2_status == PIN_IS_LOW){
			while(button_2_status == PIN_IS_LOW){
				button_2_status =  GPIO_read(PORT_0, PIN1);
			}
			button2_mass.size = 10;
			strcpy(button2_mass.massage, "b2_rising ");
			// rising edge occeured
			xQueueSend( xQueue1, ( void * ) &button2_mass, ( TickType_t ) portMAX_DELAY);
		} else {//check for faling edge
			while(button_2_status == PIN_IS_HIGH){
				button_2_status =  GPIO_read(PORT_0, PIN1);
			}
			button2_mass.size = 11;
			strcpy(button2_mass.massage, "b2_falling");
			xQueueSend( xQueue1, ( void * ) &button2_mass,  ( TickType_t ) portMAX_DELAY);
		}
		vTaskDelay( 100 );			
	}
}

void vTaskMassenger( void * pvParameters ) {	
   massage massenger  = {10, "massenger "};
	for( ;; ) {
		// check for positive edge
		xQueueSend( xQueue1, ( void * ) &massenger,  ( TickType_t ) portMAX_DELAY );
		vTaskDelay( 100 );			
	}
}

void vTaskWriter( void * pvParameters ) {	
   massage massagex;
	for( ;; ) {
		 if( xQueueReceive( xQueue1, &( massagex ), ( TickType_t ) portMAX_DELAY ) == pdPASS ) {
			 vSerialPutString(massagex.massage, massagex.size);
			 vTaskDelay( 20 );
		}
  }			
		vTaskDelay( 50 );			
}


BaseType_t xReturned;
TaskHandle_t xHandle_vButton_1 = NULL;
TaskHandle_t xHandle_vButton_2 = NULL;
TaskHandle_t xHandle_vMassenger = NULL;
TaskHandle_t xHandle_vWriter = NULL;

int main( void )
{
		/* Setup the hardware for use with the Keil demo board. */

massage mass;
		prvSetupHardware();
		
			/* Create Tasks here */
		xSemaphore = xSemaphoreCreateCounting(20, 0);
		xQueue1 =  xQueueCreate( 100, sizeof(massage));
	  if( xSemaphore == NULL || xQueue1== NULL ){
			assert(0);
		}
		xReturned = xTaskCreate(
											vTaskButton_1_event,       /* Function that implements the task. */
											"toggle_led",          /* Text name for the task. */
											100,      /* Stack size in words, not bytes. */
											( void * )0 ,    /* Parameter passed into the task. */
											0,/* Priority at which the task is created. */
											&xHandle_vButton_1 );      /* Used to pass out the created task's handle. */
		xReturned = xTaskCreate(
											vTaskButton_2_event,       /* Function that implements the task. */
											"read status",          /* Text name for the task. */
											100,      /* Stack size in words, not bytes. */
											( void * )0 ,    /* Parameter passed into the task. */
											0,/* Priority at which the task is created. */
											&xHandle_vButton_2 );      /* Used to pass out the created task's handle. */
											
		xReturned = xTaskCreate(
									vTaskMassenger,       /* Function that implements the task. */
									"read status",          /* Text name for the task. */
									100,      /* Stack size in words, not bytes. */
									( void * )0 ,    /* Parameter passed into the task. */
									0,/* Priority at which the task is created. */
									&xHandle_vMassenger );      /* Used to pass out the created task's handle. */
											
		xReturned = xTaskCreate(
								vTaskWriter,       /* Function that implements the task. */
								"read status",          /* Text name for the task. */
								100,      /* Stack size in words, not bytes. */
								( void * )0 ,    /* Parameter passed into the task. */
								0,/* Priority at which the task is created. */
								&xHandle_vWriter );      /* Used to pass out the created task's handle. */
		
		/* Now all the tasks have been started - start the scheduler.

		NOTE : Tasks run in system mode and the scheduler runs in Supervisor mode.
		The processor MUST be in supervisor mode when vTaskStartScheduler is 
		called.  The demo applications included in the FreeRTOS.org download switch
		to supervisor mode prior to main being called.  If you are not using one of
		these demo application projects then ensure Supervisor mode is used here. */
		vTaskStartScheduler();

		/* Should never reach here!  If you do then there was not enough heap
		available for the idle task to be created. */
		for( ;; );
}
/*-----------------------------------------------------------*/

static void prvSetupHardware( void )
{
	/* Perform the hardware setup required.  This is minimal as most of the
	setup is managed by the settings in the project file. */

	/* Configure UART */
	xSerialPortInitMinimal(mainCOM_TEST_BAUD_RATE);

	/* Configure GPIO */
	GPIO_init();

	/* Setup the peripheral bus to be the same as the PLL output. */
	VPBDIV = mainBUS_CLK_FULL;
}
/*-----------------------------------------------------------*/


