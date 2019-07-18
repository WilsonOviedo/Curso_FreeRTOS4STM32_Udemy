/**
 ******************************************************************************
 * @file    main.c
 * @author  Ac6
 * @version V1.0
 * @date    01-December-2013
 * @brief   Default main function.
 ******************************************************************************
 */

//https://www.st.com/content/ccc/resource/technical/document/reference_manual/59/b9/ba/7f/11/af/43/d5/CD00171190.pdf/files/CD00171190.pdf/jcr:content/translations/en.CD00171190.pdf


#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "stm32f10x.h"
#include "stm32f1xx_nucleo.h"

#include "FreeRTOS.h"
#include "task.h"

//Global constant values
#define TRUE						1
#define FALSE						0

//Botton status
#define PRESSED 					TRUE
#define NOT_PRESSED 				FALSE

//Global variables values
uint8_t button_status_flag = NOT_PRESSED;

TaskHandle_t xTaskHandle1 = NULL;
TaskHandle_t xTaskHandle2 = NULL;

//Task functions prototypes
void led_task_handler(void *params);
void buttom_handler(void *params);
static void prvSetupHardware(void);
static void prvSetupUART(void);
void printMsg(char *msg);
static void prvSetupGpio();


//some macros
#define TRUE 1
#define FALSE 0

#define AVAILABLE TRUE
#define NOT_AVAILABLE FALSE

#ifdef USE_SEMIHOSTING
extern void initialise_monitor_handles();
#endif
//Global variable section
char msg[100] = "ASDasdsdaasd";
char usr_msg[250];
uint8_t UART_ACCES_KEY = AVAILABLE;

int main(void) {
#ifdef USE_SEMIHOSTING
	initialise_monitor_handles();
#endif

	DWT-> CTRL |= (1<<0); //Enable CYCCNT in DWT_CTRl

	//DBGMCU_Config(DBGMCU_SLEEP | DBGMCU_STOP | DBGMCU_STANDBY, ENABLE);
	//1. Reset RCC clock configuration to the default reset state
	//HSI ON, PLL OFF, HSE OFF, system clock = 16 MHz, cpu_clock 16 MHz

	RCC_DeInit();

	//2. Update the SystemCoreClock variable
	SystemCoreClockUpdate();

	prvSetupHardware();

	sprintf(usr_msg, "This is hello world application starting\n");
	printMsg(usr_msg);

	//Start recording
	SEGGER_SYSVIEW_Conf();
	SEGGER_SYSVIEW_Start();

	//3. Lest create 2 task, task-1 and task-2
	xTaskCreate(led_task_handler, "LED_TASK", configMINIMAL_STACK_SIZE, NULL, 1,NULL);

	//Start the scheduler
	vTaskStartScheduler();

	for (;;)
		;
}

//===============================Main Tasks==========================================
//Led task
void led_task_handler(void *params) {
	while (1) {
			if(button_status_flag == PRESSED){
				//turn on the led
				GPIO_WriteBit(GPIOA,GPIO_Pin_5,Bit_SET);
				SEGGER_SYSVIEW_Print("LED On");
			}else{
				//turn off the led
				GPIO_WriteBit(GPIOA,GPIO_Pin_5,Bit_RESET);
				SEGGER_SYSVIEW_Print("LED Off");
			}
	}
}
//Buttom task
void buttom_handler(void *params) {
		button_status_flag ^=1;
}

//Configs Task
static void prvSetupUART(void) {
	GPIO_InitTypeDef gpio_uart_pins;
	USART_InitTypeDef uart2_init;

	//1. Enable the UART and GPIOA  Peripheral clock
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	//PA3 Rx, PA2 Tx

	//2. Alternate function configuration of MCU pins to behave as UART2 TX and RX

	//zeroing each and every  member element of the structure
	memset(&gpio_uart_pins, 0, sizeof(gpio_uart_pins));

	gpio_uart_pins.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
	gpio_uart_pins.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &gpio_uart_pins);
	//3. AF mode settings
	GPIO_PinRemapConfig(GPIO_Remap_USART2, ENABLE);

	//4.USART parameter initializations

	//zeroing each and every  member element of the structure
	memset(&uart2_init, 0, sizeof(uart2_init));

	uart2_init.USART_BaudRate = 115200;
	uart2_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	uart2_init.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	uart2_init.USART_Parity = USART_Parity_No;
	uart2_init.USART_StopBits = USART_StopBits_1;
	uart2_init.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART2, &uart2_init);

	//5. Enable the UART2 peripheral
	USART_Cmd(USART2, ENABLE);
}



static void prvSetupHardware(void) {
	//Setup Button and LED
	prvSetupGpio();

	//setup UART2
	prvSetupUART();

}

//Setup GPIOs
static void prvSetupGpio(){
	//This function is board specific

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);

	GPIO_InitTypeDef led_init, button_init;

	led_init.GPIO_Mode=GPIO_Mode_Out_PP;
	led_init.GPIO_Pin= GPIO_Pin_5;
	led_init.GPIO_Speed=GPIO_Speed_2MHz;

	GPIO_Init(GPIOA,&led_init);

	button_init.GPIO_Mode=GPIO_Mode_IPD;
	button_init.GPIO_Pin= GPIO_Pin_13;
	button_init.GPIO_Speed=GPIO_Speed_2MHz;

	GPIO_Init(GPIOC,&button_init);

	//Interrupt configuration for the button (PC13)
	//1. System configurations for EXTI line (SYSCFG settings)
	GPIO_EXTILineConfig ( GPIO_PortSourceGPIOC , GPIO_PinSource13 );

	//2. EXTI line configuration 13, falling edge, interrup mode
	EXTI_InitTypeDef exti_init;
	exti_init.EXTI_Line=EXTI_Line13;
	exti_init.EXTI_Mode= EXTI_Mode_Interrupt;
	exti_init.EXTI_Trigger=EXTI_Trigger_Falling;
	exti_init.EXTI_LineCmd=ENABLE;
	EXTI_Init(&exti_init);

	//3. NVIC settings (IRQ settings for the selected EXTI line(13))
	NVIC_SetPriority(EXTI15_10_IRQn,5);
	NVIC_EnableIRQ(EXTI15_10_IRQn);


}

void EXTI15_10_IRQHandler(void){
	traceISR_ENTER();
	//1. clear the interrup pending bit of the EXTI line (13)
	EXTI_ClearITPendingBit(EXTI_Line13);
	buttom_handler(NULL);
	traceISR_EXIT();



}

void printMsg(char *msg) {
	for (uint32_t i = 0; i < strlen(msg); i++) {
		while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) != SET)
			;
		USART_SendData(USART2, msg[i]);
	}

}
