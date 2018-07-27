/*
===============================================================================
 Name        : FRTOS.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/
#include "chip.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "string.h"
#include <cr_section_macros.h>

#define UART_SELECTION 	LPC_UART1
#define IRQ_SELECTION 	UART1_IRQn
#define HANDLER_NAME 	UART1_IRQHandler

#define DEBUGOUT(...) printf(__VA_ARGS__)

#define OUTPUT		((uint8_t) 1)
#define INPUT		((uint8_t) 0)

/* Transmit and receive ring buffer sizes */
#define UART_SRB_SIZE 128	/* Send */
#define UART_RRB_SIZE 128	/* Receive */

//TX Y RX DE UART 1
#define 	TXD1	0,15
#define		RXD1	0,16

/* Transmit and receive buffers */
static uint8_t rxbuff[UART_RRB_SIZE], txbuff[UART_SRB_SIZE];

const char inst1[] = "LPC17xx/40xx UART example using ring buffers\r\n";
const char inst2[] = "Press a key to echo it back or ESC to quit\r\n";


/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

static volatile bool fIntervalReached;
static volatile bool fAlarmTimeMatched;
static volatile bool On0, On1;

/* Transmit and receive ring buffers */
STATIC RINGBUFF_T txring, rxring;

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

SemaphoreHandle_t Semaforo_RX;

QueueHandle_t Cola_RX;
QueueHandle_t Cola_TX;

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/*****************************************************************************
 * Public functions
 ****************************************************************************/

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* uC_StartUp */
void uC_StartUp (void)
{
	Chip_GPIO_Init (LPC_GPIO);
	Chip_GPIO_SetDir (LPC_GPIO, RXD1, INPUT);
	Chip_IOCON_PinMux (LPC_IOCON, RXD1, IOCON_MODE_INACT, IOCON_FUNC1);
	Chip_GPIO_SetDir (LPC_GPIO, TXD1, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, TXD1, IOCON_MODE_INACT, IOCON_FUNC1);
}


void HANDLER_NAME(void)
{
	BaseType_t Testigo=pdFALSE;


	/* Want to handle any errors? Do it here. */

	/* Use default ring buffer handler. Override this with your own
	   code if you need more capability. */
	//Chip_UART_IRQRBHandler(UART_SELECTION, &rxring, &txring);


	/* Handle transmit interrupt if enabled */
	if (UART_SELECTION->IER & UART_IER_THREINT)
	{
		Chip_UART_TXIntHandlerRB(UART_SELECTION, &txring);

		/* Disable transmit interrupt if the ring buffer is empty */
		if (RingBuffer_IsEmpty(&txring)) {
			Chip_UART_IntDisable(UART_SELECTION, UART_IER_THREINT);
		}
	}

	/* Handle receive interrupt */
	/* New data will be ignored if data not popped in time */
	if (UART_SELECTION->IIR & UART_IIR_INTID_RDA )
	{
		while (Chip_UART_ReadLineStatus(UART_SELECTION) & UART_LSR_RDR)
		{
			uint8_t ch = Chip_UART_ReadByte(UART_SELECTION);
			RingBuffer_Insert(&rxring, &ch);
			xSemaphoreGiveFromISR(Semaforo_RX, &Testigo);	//Devuelve si una de las tareas bloqueadas tiene mayor prioridad que la actual
		}
		portYIELD_FROM_ISR(Testigo);					//Si testigo es TRUE -> ejecuta el scheduler
	}
}


static void xTaskUART1Config(void *pvParameters)
{
	DEBUGOUT("PRUEBA UART..\n");	//Imprimo en la consola

	/* Setup UART for 9600 */
	Chip_UART_Init(UART_SELECTION);
	Chip_UART_SetBaud(UART_SELECTION, 9600);
	Chip_UART_ConfigData(UART_SELECTION, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(UART_SELECTION, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(UART_SELECTION);

	/* Before using the ring buffers, initialize them using the ring buffer init function */
	RingBuffer_Init(&rxring, rxbuff, 1, UART_RRB_SIZE);
	RingBuffer_Init(&txring, txbuff, 1, UART_SRB_SIZE);

	/* Reset and enable FIFOs, FIFO trigger level 3 (14 chars) */
	Chip_UART_SetupFIFOS(UART_SELECTION, (UART_FCR_FIFO_EN | UART_FCR_RX_RS |
							UART_FCR_TX_RS | UART_FCR_TRG_LEV3));

	/* Enable receive data and line status interrupt */
	Chip_UART_IntEnable(UART_SELECTION, (UART_IER_RBRINT | UART_IER_RLSINT));

	/* preemption = 1, sub-priority = 1 */
	//NVIC_SetPriority(IRQ_SELECTION, 1);
	NVIC_EnableIRQ(IRQ_SELECTION);

	vTaskDelete(NULL);	//Borra la tarea
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskCargarAnillo */
//Encargada de cargar el anillo a partir de la cola
static void vTaskCargarAnillo(void *pvParameters)
{
	uint8_t Receive=0;
	while (1)
	{
		xQueueReceive(Cola_TX, &Receive, portMAX_DELAY);
		Chip_UART_SendRB(UART_SELECTION, &txring, &Receive , 1);
	}
	vTaskDelete(NULL);	//Borra la tarea si sale del while
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskLeerAnillo */
//
static void vTaskLeerAnillo(void *pvParameters)
{
	uint8_t Receive=0;
	uint8_t Testigo=0;

	while (1)
	{
		xSemaphoreTake(Semaforo_RX, portMAX_DELAY);
		Testigo = RingBuffer_Pop(&rxring, &Receive);
		if(Testigo)
		{
			xSemaphoreGive(Semaforo_RX);
			xQueueSendToBack(Cola_RX, &Receive, portMAX_DELAY);
			DEBUGOUT("%c", Receive);	//Imprimo en la consola
		}
	}
	vTaskDelete(NULL);	//Borra la tarea si sale del while
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* main
*/
int main(void)
{
	const char inst1[] = "Hola manco\r\n";
	uint8_t i=0;

	uC_StartUp();

	SystemCoreClockUpdate();

	vSemaphoreCreateBinary(Semaforo_RX);			//Creamos el semaforo

	Cola_RX = xQueueCreate(UART_RRB_SIZE, sizeof(uint8_t));	//Creamos una cola

	Cola_TX = xQueueCreate(UART_SRB_SIZE, sizeof(uint8_t));	//Creamos una cola

	for(i=0;i<10;i++)
	{
		xQueueSendToBack(Cola_TX, &inst1[i], portMAX_DELAY);
	}

	xTaskCreate(vTaskLeerAnillo, (char *) "vTaskLeerAnillo",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	xTaskCreate(vTaskCargarAnillo, (char *) "vTaskCargarAnillo",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	xTaskCreate(xTaskUART1Config, (char *) "xTaskUART1Config",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 2UL),
				(xTaskHandle *) NULL);

	/* Start the scheduler */
	vTaskStartScheduler();

	/* Nunca debería arribar aquí */
    return 0;
}
