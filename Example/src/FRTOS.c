/*
===============================================================================
 Name        : FRTOS.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/

/*****************************************************************************
 * Includes
 ****************************************************************************/

#include "stdlib.h"
#include "chip.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "string.h"
#include <cr_section_macros.h>

/*****************************************************************************
 * Defines
 ****************************************************************************/

#define ON			((uint8_t) 1)
#define OFF			((uint8_t) 0)
#define PORT(x) 	((uint8_t) x)
#define PIN(x)		((uint8_t) x)
#define OUTPUT		((uint8_t) 1)
#define INPUT		((uint8_t) 0)
#define DEBUGOUT(...) printf(__VA_ARGS__)

//UART
#define UART_SELECTION 	LPC_UART1
#define IRQ_SELECTION 	UART1_IRQn
#define HANDLER_NAME 	UART1_IRQHandler
#define TXD1	0,15	//RX UART1	(Pin 13 LPC1769) (EXP 18)
#define	RXD1	0,16	//TX UART1	(Pin 14 LPC1769) (EXP 19)
#define TXD2	0,10	//RX UART2	(Pin 40 LPC1769)
#define RXD2	0,11	//TX UART2	(Pin 41 LPC1769)

#define UART_SRB_SIZE 32	//S:Send - Transmit ring buffer size
#define UART_RRB_SIZE 1024	//R:Receive - Receive ring buffer size

//GSM
#define RST_GSM	1,18	//Expansion 15

//GPS
#define	INICIO_TRAMA	0
#define CHEQUEO_FIN		1
#define CHEQUEO_TRAMA	2
#define TRAMA_CORRECTA	3
#define HORA_FECHA		4
#define LAT_LONG		5

//Placa Infotronic
#define LED_STICK	PORT(0),PIN(22)
#define	BUZZER		PORT(0),PIN(28)
#define	SW1			PORT(2),PIN(10)
#define SW2			PORT(0),PIN(18)
#define	SW3			PORT(0),PIN(11)
#define SW4			PORT(2),PIN(13)
#define SW5			PORT(1),PIN(26)
#define	LED1		PORT(2),PIN(0)
#define	LED2		PORT(0),PIN(23)
#define	LED3		PORT(0),PIN(21)
#define	LED4		PORT(0),PIN(27)
#define	RGBB		PORT(2),PIN(1)
#define	RGBG		PORT(2),PIN(2)
#define	RGBR		PORT(2),PIN(3)
#define EXPANSION0	PORT(2),PIN(7)
#define EXPANSION1	PORT(1),PIN(29)
#define EXPANSION2	PORT(4),PIN(28)
#define EXPANSION3	PORT(1),PIN(23)
#define EXPANSION4	PORT(1),PIN(20)
#define EXPANSION5	PORT(0),PIN(19)
#define EXPANSION6	PORT(3),PIN(26)
#define EXPANSION7	PORT(1),PIN(25)
#define EXPANSION8	PORT(1),PIN(22)
#define EXPANSION9	PORT(1),PIN(19)
#define EXPANSION10	PORT(0),PIN(20)
#define EXPANSION11	PORT(3),PIN(25)
#define EXPANSION12	PORT(1),PIN(27)
#define EXPANSION13	PORT(1),PIN(24)
#define EXPANSION14	PORT(1),PIN(21)
#define EXPANSION15	PORT(1),PIN(18)
#define EXPANSION16	PORT(2),PIN(8)
#define EXPANSION17	PORT(2),PIN(12)
#define EXPANSION18	PORT(0),PIN(16)
#define EXPANSION19	PORT(0),PIN(15)
#define EXPANSION20	PORT(0),PIN(22) //RTS1
#define EXPANSION21	PORT(0),PIN(17) //CTS1
//#define EXPANSION22		PORT
//#define EXPANSION23		PORT
//#define EXPANSION24		PORT
#define ENTRADA_ANALOG0 PORT(1),PIN(31)
#define ENTRADA_DIG1	PORT(4),PIN(29)
#define ENTRADA_DIG2	PORT(1),PIN(26)
#define MOSI1			PORT(0),PIN(9)
#define MISO1 			PORT(0),PIN(8)
#define SCK1			PORT(0),PIN(7)
#define SSEL1			PORT(0),PIN(6)

/*****************************************************************************
 * Types/enumerations/variables
 ****************************************************************************/
SemaphoreHandle_t Semaforo_RX;
QueueHandle_t Cola_RX;
QueueHandle_t Cola_TX;

STATIC RINGBUFF_T txring, rxring;								//Transmit and receive ring buffers
static uint8_t rxbuff[UART_RRB_SIZE], txbuff[UART_SRB_SIZE];	//Transmit and receive buffers


/*****************************************************************************
 * Functions
 ****************************************************************************/
BaseType_t LeerCola(QueueHandle_t xQueue, uint8_t *Dato, uint8_t cantidad);
void EscribirCola(QueueHandle_t xQueue, uint8_t *Dato, uint8_t cantidad);


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* uC_StartUp */
void uC_StartUp (void)
{
	DEBUGOUT("Configurando pines I/O..\n");	//Imprimo en la consola
	Chip_GPIO_Init (LPC_GPIO);

	//Inicializacion de los pines del GSM
	Chip_GPIO_SetDir (LPC_GPIO, RXD1, INPUT);
	Chip_IOCON_PinMux (LPC_IOCON, RXD1, IOCON_MODE_INACT, IOCON_FUNC1);
	Chip_GPIO_SetDir (LPC_GPIO, TXD1, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, TXD1, IOCON_MODE_INACT, IOCON_FUNC1);
	Chip_GPIO_SetDir (LPC_GPIO, RST_GSM, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, RST_GSM, IOCON_MODE_INACT, IOCON_FUNC0);

	//Inicializacion de los pines de la Placa Infotronic
	Chip_GPIO_SetDir (LPC_GPIO, LED_STICK, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, LED_STICK, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetDir (LPC_GPIO, BUZZER, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, BUZZER, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetDir (LPC_GPIO, RGBB, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, RGBB, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetDir (LPC_GPIO, RGBG, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, RGBG, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetDir (LPC_GPIO, RGBR, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, RGBR, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetDir (LPC_GPIO, LED1, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, LED1, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetDir (LPC_GPIO, LED2, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, LED2, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetDir (LPC_GPIO, LED3, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, LED3, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetDir (LPC_GPIO, LED4, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, LED4, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetDir (LPC_GPIO, SW1, INPUT);
	Chip_IOCON_PinMux (LPC_IOCON, SW1, IOCON_MODE_PULLDOWN, IOCON_FUNC0);

	//Salidas apagadas
	Chip_GPIO_SetPinOutLow(LPC_GPIO, LED_STICK);
	Chip_GPIO_SetPinOutHigh(LPC_GPIO, BUZZER);
	Chip_GPIO_SetPinOutLow(LPC_GPIO, RGBR);
	Chip_GPIO_SetPinOutLow(LPC_GPIO, RGBG);
	Chip_GPIO_SetPinOutLow(LPC_GPIO, RGBB);
	Chip_GPIO_SetPinOutLow(LPC_GPIO, LED1);
	Chip_GPIO_SetPinOutLow(LPC_GPIO, LED2);
	Chip_GPIO_SetPinOutLow(LPC_GPIO, LED3);
	Chip_GPIO_SetPinOutLow(LPC_GPIO, LED4);
}


void UART1_IRQHandler(void)
{
	BaseType_t Testigo=pdFALSE;

	/* Handle transmit interrupt if enabled */
	if (LPC_UART1->IER & UART_IER_THREINT)
	{
		Chip_UART_TXIntHandlerRB(LPC_UART1, &txring);

		/* Disable transmit interrupt if the ring buffer is empty */
		if (RingBuffer_IsEmpty(&txring))
		{
			Chip_UART_IntDisable(LPC_UART1, UART_IER_THREINT);
		}
	}

	/* Handle receive interrupt */
	/* New data will be ignored if data not popped in time */
	if (LPC_UART1->IIR & UART_IIR_INTID_RDA )
	{
		while (Chip_UART_ReadLineStatus(LPC_UART1) & UART_LSR_RDR)
		{
			uint8_t ch = Chip_UART_ReadByte(LPC_UART1);
			RingBuffer_Insert(&rxring, &ch);
			xSemaphoreGiveFromISR(Semaforo_RX, &Testigo);	//Devuelve si una de las tareas bloqueadas tiene mayor prioridad que la actual
		}
		portYIELD_FROM_ISR(Testigo);					//Si testigo es TRUE -> ejecuta el scheduler
	}
}


static void xTaskUART1Config(void *pvParameters)
{
	DEBUGOUT("Configurando la UART1..\n");	//Imprimo en la consola

	/* Setup UART for 4800 */
	Chip_UART_Init(LPC_UART1);
	Chip_UART_SetBaud(LPC_UART1, 4800);
	Chip_UART_ConfigData(LPC_UART1, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(LPC_UART1, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(LPC_UART1);

	/* Before using the ring buffers, initialize them using the ring buffer init function */
	RingBuffer_Init(&rxring, rxbuff, 1, UART_RRB_SIZE);
	RingBuffer_Init(&txring, txbuff, 1, UART_SRB_SIZE);

	/* Reset and enable FIFOs, FIFO trigger level 3 (14 chars) */
	Chip_UART_SetupFIFOS(LPC_UART1, (UART_FCR_FIFO_EN | UART_FCR_RX_RS |
							UART_FCR_TX_RS | UART_FCR_TRG_LEV3));

	/* Enable receive data and line status interrupt */
	Chip_UART_IntEnable(LPC_UART1, (UART_IER_RBRINT | UART_IER_RLSINT));

	//Habilito interrupcion UART1
	NVIC_EnableIRQ(UART1_IRQn);

	vTaskDelete(NULL);	//Borra la tarea
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskGSMConfig */
static void vTaskGSMConfig(void *pvParameters)
{
	Chip_GPIO_SetPinOutHigh(LPC_GPIO, RST_GSM);
	vTaskDelay(10/portTICK_RATE_MS);	//Espero 10ms
	Chip_GPIO_SetPinOutLow(LPC_GPIO, RST_GSM);
	vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms
	Chip_GPIO_SetPinOutHigh(LPC_GPIO, RST_GSM);
	vTaskDelay(5000/portTICK_RATE_MS);	//Espero 5 segundos para que reinicie

	Chip_UART_SendRB(UART_SELECTION, &txring, "AT\r\n", sizeof("AT\r\n") - 1); //Enviamos "AT"
	vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms
	Chip_UART_SendRB(UART_SELECTION, &txring, "AT\r\n", sizeof("AT\r\n") - 1); //Enviamos "AT"
	vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms
	Chip_UART_SendRB(UART_SELECTION, &txring, "AT\r\n", sizeof("AT\r\n") - 1); //Enviamos "AT"
	vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms

	vTaskDelay(4000/portTICK_RATE_MS);	//Espero 4 segundos
	//Aca deberia devolver "OK"

	//Seguir ejemplo de https://www.teachmemicro.com/send-data-sim800-gprs-thingspeak/
	//Utilizando libreria Adafruit_FONA

	/*
	 * Pagina 208 Command Manual
		AT+CGATT	- Attach or detach from GPRS service
		AT+CGDCONT	- Define PDP context
		AT+CGQMIN	- Quality of service profile (minimum acceptable)
		AT+CGQREQ	- Quality of service profile (requested)
		AT+CGACT	- PDP context activate or deactivate
		AT+CGDATA	- Enter data state
		AT+CGPADDR	- Show PDP address
		AT+CGCLASS	- GPRS mobile station class
		AT+CGEREP	- Control unsolicited GPRS event reporting
		AT+CGREG	- Network registration status
		AT+CGSMS	- Select service for MO SMS messages
	 */
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
		Chip_UART_SendRB(LPC_UART1, &txring, &Receive , 1);
	}
	vTaskDelete(NULL);	//Borra la tarea si sale del while
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskLeerAnillo */
//Pasa la informacion del anillo a la cola de recepcion
static void vTaskLeerAnillo(void *pvParameters)
{
	uint8_t Receive=0;
	uint8_t Testigo=0, dato=0;

	while (1)
	{
		xSemaphoreTake(Semaforo_RX, portMAX_DELAY);
		Testigo = RingBuffer_Pop(&rxring, &Receive);
		if(Testigo)
		{
			xSemaphoreGive(Semaforo_RX);
			xQueueSendToBack(Cola_RX, &Receive, portMAX_DELAY);
		}

		//leo la cola de rercepcion y lo muestro en pantalla
		if(LeerCola(Cola_RX,&dato,1))
		{
			DEBUGOUT("%c", dato);	//Imprimo en la consola
		}

	}
	vTaskDelete(NULL);	//Borra la tarea si sale del while
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* funcion para escribir la cola */
//
void EscribirCola(QueueHandle_t xQueue, uint8_t *Dato, uint8_t cantidad)
{
	uint8_t i=0;

	for(i=0;i<cantidad;i++)
	{
		xQueueSendToBack(xQueue, &Dato[i], portMAX_DELAY);
	}

}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* funcion para leer la cola */
//Devuelve 0 si no estan disponibles la cantidad de items pedidos, caso contrario devuelve 1
BaseType_t LeerCola(QueueHandle_t xQueue, uint8_t *Dato, uint8_t cantidad)
{
	uint8_t i=0;
	UBaseType_t uxNumberOfItems;

	/* How many items are currently in the queue referenced by the xQueue handle? */
	uxNumberOfItems = uxQueueMessagesWaiting( xQueue );

	if(uxNumberOfItems < cantidad)
		return pdFALSE;
	else
	{
		for(i=0;i<cantidad;i++)
		{
			xQueueReceive(xQueue, &Dato[i], portMAX_DELAY);
		}
		return pdTRUE;
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* main
*/
int main(void)
{
	DEBUGOUT("Inicializando prueba GSM..\n");	//Imprimo en la consola

	uC_StartUp();

	SystemCoreClockUpdate();

	vSemaphoreCreateBinary(Semaforo_RX);			//Creamos el semaforo

	Cola_RX = xQueueCreate(UART_RRB_SIZE, sizeof(uint8_t));	//Creamos una cola

	Cola_TX = xQueueCreate(UART_SRB_SIZE, sizeof(uint8_t));	//Creamos una cola

	xTaskCreate(vTaskLeerAnillo, (char *) "vTaskLeerAnillo",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 2UL),
				(xTaskHandle *) NULL);

	xTaskCreate(vTaskCargarAnillo, (char *) "vTaskCargarAnillo",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	xTaskCreate(xTaskUART1Config, (char *) "xTaskUART1Config",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 3UL),
				(xTaskHandle *) NULL);

	xTaskCreate(vTaskGSMConfig, (char *) "vTaskGSMConfig",
					configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
					(xTaskHandle *) NULL);

	/* Start the scheduler */
	vTaskStartScheduler();

	/* Nunca debería arribar aquí */
    return 0;
}
