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
#include "semphr.h"
#include "MFRC522.h"
#include "rfid_utils.h"
#include "string.h"
#include "stdlib.h"
#include "ControlVehicular.h"
#include "GPS.h"
#include "UART.h"
#include "RFID.h"
#include "GSM.h"


//#include "RegsLPC1769.h"



#define DEBUGOUT1(...) printf(__VA_ARGS__)
#define DEBUGOUT(...) printf(__VA_ARGS__)
#define DEBUGSTR(...) printf(__VA_ARGS__)

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

static volatile bool fIntervalReached;
static volatile bool fAlarmTimeMatched;
static volatile bool On0, On1;

// RFID structs
MFRC522Ptr_t mfrcInstance;

int last_balance = 0;
unsigned int last_user_ID;


typedef struct
{
	SemaphoreHandle_t 	Rx;
	RINGBUFF_T 	anillo;
	SemaphoreHandle_t 	cola;
	LPC_USART_T			*uart;
}ANILLO;

ANILLO anillo_struct;

//ThingSpeak
char http_cmd[80];
char url_string[] = "api.thingspeak.com/update?";	//URL
char apiKey[] = "api_key=4IVCTNA39FY9U35C&";		//Write API key from ThingSpeak: 4IVCTNA39FY9U35C
char data[] = "field1=30";	//
int status;
int datalen;

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

SemaphoreHandle_t Semaforo_RFID;
SemaphoreHandle_t Semaforo_RTC;
SemaphoreHandle_t Semaforo_RX1;
SemaphoreHandle_t Semaforo_RX2;


QueueHandle_t Cola_RX1,Cola_RX2;
QueueHandle_t Cola_TX1,Cola_TX2;
QueueHandle_t Cola_Pulsadores;



STATIC RINGBUFF_T txring1,txring2, rxring1,rxring2;								//Transmit and receive ring buffers
static uint8_t rxbuff1[UART_RRB_SIZE], txbuff1[UART_SRB_SIZE],rxbuff2[UART_RRB_SIZE], txbuff2[UART_SRB_SIZE];	//Transmit and receive buffers

//volatile int HourGPS, MinuteGPS, DayGPS, MonthGPS, YearGPS;		//GPS: Variables que guardan informacion
//volatile float LatGPS, LongGPS;									//GPS: Variables que guardan informacion
//volatile float Lat1GPS, Lat2GPS, Long1GPS, Long2GPS;			//GPS: Variables auxiliares



/*****************************************************************************
 * Private functions
 ****************************************************************************/
/* Gets and shows the current time and date */
static void showTime(RTC_TIME_T *pTime)
{
	DEBUGOUT("Time: %.2d:%.2d %.2d/%.2d/%.4d\r\n",
			 pTime->time[RTC_TIMETYPE_HOUR],
			 pTime->time[RTC_TIMETYPE_MINUTE],
			 pTime->time[RTC_TIMETYPE_DAYOFMONTH],
			 pTime->time[RTC_TIMETYPE_MONTH],
			 pTime->time[RTC_TIMETYPE_YEAR]);
}


/*******************************************************************************
 *  System routine functions
 ******************************************************************************/



void RTC_IRQHandler(void)
{

	BaseType_t Testigo=pdFALSE;

	/* Interrupcion cada 1 minuto */
	if (Chip_RTC_GetIntPending(LPC_RTC, RTC_INT_COUNTER_INCREASE)) {
		/* Clear pending interrupt */
		Chip_RTC_ClearIntPending(LPC_RTC, RTC_INT_COUNTER_INCREASE);
		xSemaphoreGiveFromISR(Semaforo_RTC, &Testigo);	//Devuelve si una de las tareas bloqueadas tiene mayor prioridad que la actual
		portYIELD_FROM_ISR(Testigo);					//Si testigo es TRUE -> ejecuta el scheduler

	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////

void UART1_IRQHandler(void)
{
	BaseType_t Testigo=pdFALSE;

	/* Handle transmit interrupt if enabled */
	if (LPC_UART1->IER & UART_IER_THREINT)
	{
		Chip_UART_TXIntHandlerRB(LPC_UART1, &txring1);

		/* Disable transmit interrupt if the ring buffer is empty */
		if (RingBuffer_IsEmpty(&txring1))
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
			RingBuffer_Insert(&rxring1, &ch);
			xSemaphoreGiveFromISR(Semaforo_RX1, &Testigo);	//Devuelve si una de las tareas bloqueadas tiene mayor prioridad que la actual
		}
		portYIELD_FROM_ISR(Testigo);					//Si testigo es TRUE -> ejecuta el scheduler
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* UART2_IRQHandler */
void UART2_IRQHandler(void)
{
	BaseType_t Testigo=pdFALSE;

	/* Handle transmit interrupt if enabled */
	if (LPC_UART2->IER & UART_IER_THREINT)
	{
		Chip_UART_TXIntHandlerRB(LPC_UART2, &txring2);

		/* Disable transmit interrupt if the ring buffer is empty */
		if (RingBuffer_IsEmpty(&txring2))
		{
			Chip_UART_IntDisable(LPC_UART2, UART_IER_THREINT);
		}
	}

	/* Handle receive interrupt */
	/* New data will be ignored if data not popped in time */
	if (LPC_UART2->IIR & UART_IIR_INTID_RDA )
	{
		while (Chip_UART_ReadLineStatus(LPC_UART2) & UART_LSR_RDR)
		{
			uint8_t ch = Chip_UART_ReadByte(LPC_UART2);
			RingBuffer_Insert(&rxring2, &ch);
			xSemaphoreGiveFromISR(Semaforo_RX2, &Testigo);	//Devuelve si una de las tareas bloqueadas tiene mayor prioridad que la actual
		}
		portYIELD_FROM_ISR(Testigo);					//Si testigo es TRUE -> ejecuta el scheduler
	}
}


/*****************************************************************************
 * Public functions
 ****************************************************************************/


static void xTaskRFIDConfig(void *pvParameters)
{
	SystemCoreClockUpdate();

	DEBUGOUT1("PRUEBA RFID..\n");	//Imprimo en la consola

	setupRFID(&mfrcInstance);

	xSemaphoreGive(Semaforo_RFID);//Doy el semaforo para que se inicie la lectura

	vTaskDelete(NULL);	//Borra la tarea
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskRTConfig */
static void xTaskRTConfig(void *pvParameters)
{
	RTC_TIME_T FullTime;

	SystemCoreClockUpdate();

	DEBUGOUT("PRUEBA RTC..\n");	//Imprimo en la consola

	Chip_RTC_Init(LPC_RTC);

	/* Set current time for RTC 2:00:00PM, 2012-10-05 */


	FullTime.time[RTC_TIMETYPE_SECOND]  = 0;
	FullTime.time[RTC_TIMETYPE_MINUTE]  = 25;
	FullTime.time[RTC_TIMETYPE_HOUR]    = 21;
	FullTime.time[RTC_TIMETYPE_DAYOFMONTH]  = 05;
	FullTime.time[RTC_TIMETYPE_DAYOFWEEK]   = 4;
	FullTime.time[RTC_TIMETYPE_DAYOFYEAR]   = 207;
	FullTime.time[RTC_TIMETYPE_MONTH]   = 10;
	FullTime.time[RTC_TIMETYPE_YEAR]    = 2018;

	Chip_RTC_SetFullTime(LPC_RTC, &FullTime);
	//

	/* Set the RTC to generate an interrupt on each minute */
	Chip_RTC_CntIncrIntConfig(LPC_RTC, RTC_AMR_CIIR_IMMIN, ENABLE);

	/* Clear interrupt pending */
	Chip_RTC_ClearIntPending(LPC_RTC, RTC_INT_COUNTER_INCREASE);

	/* Enable RTC interrupt in NVIC */
	NVIC_EnableIRQ((IRQn_Type) RTC_IRQn);

	/* Enable RTC (starts increase the tick counter and second counter register) */
	Chip_RTC_Enable(LPC_RTC, ENABLE);

//
	vTaskDelete(NULL);	//Borra la tarea
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskInicTimer */
static void vTaskRFID(void *pvParameters)
{
	xSemaphoreTake(Semaforo_RFID, portMAX_DELAY); //semaforo de inicializacion de rfid
	while (1)
	{

		// Look for new cards in RFID2
		if (PICC_IsNewCardPresent(mfrcInstance))
		{
			// Select one of the cards
			if (PICC_ReadCardSerial(mfrcInstance))
			{
//				int status = writeCardBalance(mfrcInstance, 100000); // used to recharge the card
				 userTapIn();
			}
		}
		vTaskDelay( 500 / portTICK_PERIOD_MS );//Muestreo cada 1 seg
	}

	vTaskDelete(NULL);	//Borra la tarea si sale del while
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskRTC */
static void vTaskRTC(void *pvParameters)
{
	RTC_TIME_T FullTime;
	while (1)
	{
		xSemaphoreTake(Semaforo_RTC, portMAX_DELAY);
		Chip_RTC_GetFullTime(LPC_RTC, &FullTime);
		showTime(&FullTime);
	}
	vTaskDelete(NULL);	//Borra la tarea si sale del while
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////////////

static void xTaskUART1Config(void *pvParameters)
{
	DEBUGOUT("Configurando la UART1..\n");	//Imprimo en la consola

	//Inicializacion de los pines de la UART1
	Chip_GPIO_SetDir (LPC_GPIO, RXD1, INPUT);
	Chip_IOCON_PinMux (LPC_IOCON, RXD1, IOCON_MODE_INACT, IOCON_FUNC1);
	Chip_GPIO_SetDir (LPC_GPIO, TXD1, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, TXD1, IOCON_MODE_INACT, IOCON_FUNC1);

	/* Setup UART for 9600 */
	Chip_UART_Init(LPC_UART1);
	Chip_UART_SetBaud(LPC_UART1, 9600);
	Chip_UART_ConfigData(LPC_UART1, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(LPC_UART1, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(LPC_UART1);

	/* Before using the ring buffers, initialize them using the ring buffer init function */
	RingBuffer_Init(&rxring1, rxbuff1, 1, UART_RRB_SIZE);
	RingBuffer_Init(&txring1, txbuff1, 1, UART_SRB_SIZE);

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
/* xTaskUART2Config */
static void xTaskUART2Config(void *pvParameters)
{
	DEBUGOUT("Configurando la UART2..\n");	//Imprimo en la consola

	//Inicializacion de los pines del GSM
	Chip_GPIO_SetDir (LPC_GPIO, RXD2, INPUT);
	Chip_IOCON_PinMux (LPC_IOCON, RXD2, IOCON_MODE_INACT, IOCON_FUNC1);
	Chip_GPIO_SetDir (LPC_GPIO, TXD2, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, TXD2, IOCON_MODE_INACT, IOCON_FUNC1);


	/* Setup UART for 9600 */
	Chip_UART_Init(LPC_UART2);
	Chip_UART_SetBaud(LPC_UART2, 9600);
	Chip_UART_ConfigData(LPC_UART2, (UART_LCR_WLEN8 | UART_LCR_SBS_1BIT));
	Chip_UART_SetupFIFOS(LPC_UART2, (UART_FCR_FIFO_EN | UART_FCR_TRG_LEV2));
	Chip_UART_TXEnable(LPC_UART2);

	/* Before using the ring buffers, initialize them using the ring buffer init function */
	RingBuffer_Init(&rxring2, rxbuff2, 1, UART_RRB_SIZE);
	RingBuffer_Init(&txring2, txbuff2, 1, UART_SRB_SIZE);

	/* Reset and enable FIFOs, FIFO trigger level 3 (14 chars) */
	Chip_UART_SetupFIFOS(LPC_UART2, (UART_FCR_FIFO_EN | UART_FCR_RX_RS |
							UART_FCR_TX_RS | UART_FCR_TRG_LEV3));

	/* Enable receive data and line status interrupt */
	Chip_UART_IntEnable(LPC_UART2, (UART_IER_RBRINT | UART_IER_RLSINT));

	//Habilito interrupcion UART1
	NVIC_EnableIRQ(UART2_IRQn);

	vTaskDelete(NULL);	//Borra la tarea
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskGSMConfig */
static void vTaskGSMConfig(void *pvParameters)
{
	//Inicializacion del reset del GSM
	Chip_GPIO_SetDir (LPC_GPIO, RST_GSM, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, RST_GSM, IOCON_MODE_INACT, IOCON_FUNC0);

	Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT\r\n", sizeof("AT\r\n") - 1); //Enviamos "AT"
	vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms
	Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT\r\n", sizeof("AT\r\n") - 1); //Enviamos "AT"
	vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms
	Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT\r\n", sizeof("AT\r\n") - 1); //Enviamos "AT"
	vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms

	vTaskDelete(NULL);	//Borra la tarea
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskCargarAnillo
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
}*/

/* vTaskCargarAnillo */
//Encargada de cargar el anillo a partir de la cola
static void vTaskCargarAnillo(void *pvParameters)
{
	ANILLO * Buffer;
	Buffer = (ANILLO*) pvParameters;

	uint8_t Receive=0;
	while (1)
	{
		xQueueReceive(Buffer->cola, &Receive, portMAX_DELAY);
		Chip_UART_SendRB(Buffer->uart,&Buffer->anillo, &Receive , 1);
	}
	vTaskDelete(NULL);	//Borra la tarea si sale del while
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskLeerAnillo
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
			AnalizarTramaGPS(dato);
			//DEBUGOUT("%c", dato);	//Imprimo en la consola
		}

	}
	vTaskDelete(NULL);	//Borra la tarea si sale del while
}*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskLeerAnillo */
//Pasa la informacion del anillo a la cola de recepcion
static void vTaskLeerAnillo(void *pvParameters)
{

	ANILLO * Buffer;
	Buffer = (ANILLO*) pvParameters;

	uint8_t Receive=0;
	uint8_t Testigo=0, dato=0;

	while (1)
	{
		xSemaphoreTake(Buffer->Rx, portMAX_DELAY);
		Testigo = RingBuffer_Pop(&Buffer->anillo, &Receive);
		if(Testigo)
		{
			xSemaphoreGive(Buffer->Rx);
			xQueueSendToBack(Buffer->cola, &Receive, portMAX_DELAY);
		}

/*		//leo la cola de rercepcion y lo muestro en pantalla
		if(LeerCola(Buffer->cola,&dato,1))
		{
			DEBUGOUT("%c", dato);	//Imprimo en la consola
		}
*/
	}
	vTaskDelete(NULL);	//Borra la tarea si sale del while
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskEnviarGSM */
static void vTaskAnalizarGPS(void *pvParameters)
{
	uint8_t dato=0;

	while (1)
	{
		if(LeerCola(TX_COLA_GPS,&dato,1))
		{
			AnalizarTramaGPS(dato); // que devuelva una struct que posea distintos campos: lat,long,hora,fecha y vel
			//DEBUGOUT("%c", dato);	//Imprimo en la consola
		}
	}
	vTaskDelete(NULL);	//Borra la tarea
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskEnviarGSM */
static void vTaskEnviarGSM(void *pvParameters)
{
	uint8_t Receive=OFF;

	while(1)
	{
		xQueueReceive(Cola_Pulsadores, &Receive, portMAX_DELAY);

		if(Receive==1)		//1: ENVIAR SMS
		{
			Receive=OFF;	//Reestablezco variable

			///////////////////
			//Para enviar SMS

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT\r\n", sizeof("AT\r\n") - 1); //Enviamos "AT"
			vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms
			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT\r\n", sizeof("AT\r\n") - 1); //Enviamos "AT"
			vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms
			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT\r\n", sizeof("AT\r\n") - 1); //Enviamos "AT"
			vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CNMI=2,2,0,0\r", sizeof("AT+CNMI=2,2,0,0\r") - 1); //No guardo los mensajes en memoria, los envio directamente por UART cuando llegan
			vTaskDelay(5000/portTICK_RATE_MS);	//Espero 5s

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CMGF=1\r", sizeof("AT+CMGF=1\r") - 1); //Activo modo texto. ALT+CMGF = 1 (texto). ALT + CMGF = 0 (PDU)
			vTaskDelay(5000/portTICK_RATE_MS);	//Espero 5s

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CSCS=\"GSM\"\r", sizeof("AT+CSCS=\"GSM\"\r") - 1);
			vTaskDelay(3000/portTICK_RATE_MS);	//Espero 3s

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CMGS=\"+5491137863836\"\r", sizeof("AT+CMGS=\"+5491137863836\"\r") - 1);
			vTaskDelay(3000/portTICK_RATE_MS);	//Espero 3s
			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "EMERGENCIA\032", sizeof("EMERGENCIA\032") - 1);
		}

		else if(Receive==2)	//2: ENVIAR DATOS POR GPRS A THINGSPEAK
		{
			Receive=OFF;	//Reestablezco variable

			/////////////////// SEGUN 	https://www.youtube.com/watch?v=f-VhitIURlY
			//Para enviar datos por GPRS a ThingSpeak

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT\r\n", sizeof("AT\r\n") - 1); //Enviamos "AT"
			vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms
			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT\r\n", sizeof("AT\r\n") - 1); //Enviamos "AT"
			vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms
			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT\r\n", sizeof("AT\r\n") - 1); //Enviamos "AT"
			vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CIPSHUT\r", sizeof("AT+CIPSHUT\r") - 1); //
			vTaskDelay(1000/portTICK_RATE_MS);	//Espero 1s

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CIPMUX=0\r", sizeof("AT+CIPMUX=0\r") - 1); //
			vTaskDelay(1000/portTICK_RATE_MS);	//Espero 1s

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CGATT=1\r", sizeof("AT+CGATT=1\r") - 1); //
			vTaskDelay(1000/portTICK_RATE_MS);	//Espero 1s

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CSTT=\"FONAnet\"\r", sizeof("AT+CSTT=\"FONAnet\"\r") - 1); //
			vTaskDelay(1000/portTICK_RATE_MS);	//Espero 1s

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CIICR\r", sizeof("AT+CIICR\r") - 1); //
			vTaskDelay(1000/portTICK_RATE_MS);	//Espero 1s

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CIFSR\r", sizeof("AT+CIFSR\r") - 1); //
			vTaskDelay(1000/portTICK_RATE_MS);	//Espero 1s

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CIPSTART=\"TCP\",\"", sizeof("AT+CIPSTART=\"TCP\",\"") - 1); //
			vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms
			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "184.106.153.149\",\"80\"\r", sizeof("184.106.153.149\",\"80\"\r") - 1); //
			vTaskDelay(3000/portTICK_RATE_MS);	//Espero 3s

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT+CIPSEND=48\r", sizeof("AT+CIPSEND=48\r") - 1); //44
			vTaskDelay(1000/portTICK_RATE_MS);	//Espero 1s

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "GET ", sizeof("GET ") - 1); //	GET
			vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms
			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "/update?", sizeof("/update?") - 1); //
			vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms
			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, apiKey, sizeof(apiKey) - 1); //
			vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms
			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, data, sizeof(data) - 1); //
			vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms

			Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "\r\n\r\n", sizeof("\r\n\r\n") - 1); //
			vTaskDelay(1000/portTICK_RATE_MS);	//Espero 1s
		}
	}
	vTaskDelete(NULL);	//Borra la tarea
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* xTaskPulsadores */
static void xTaskPulsadores(void *pvParameters)
{
	uint8_t Send=OFF;

	while (1)
	{
		if(Chip_GPIO_GetPinState(LPC_GPIO, SW1)==OFF)	//Si se presiona el SW1
		{
			Send=1;	//Para enviar SMS
			xQueueSendToBack(Cola_Pulsadores, &Send, portMAX_DELAY);
			vTaskDelay(1000/portTICK_RATE_MS);	//Espero 1s
		}
		if(Chip_GPIO_GetPinState(LPC_GPIO, SW2)==OFF)	//Si se presiona el SW2
		{
			Send=2;	//Para enviar datos por GPRS a ThingSpeak
			xQueueSendToBack(Cola_Pulsadores, &Send, portMAX_DELAY);
			vTaskDelay(1000/portTICK_RATE_MS);	//Espero 1s
		}
		if(Chip_GPIO_GetPinState(LPC_GPIO, SW3)==OFF)	//Si se presiona el SW3
		{
			Send=3;	//Para enviar datos por GPRS a ThingSpeak
			xQueueSendToBack(Cola_Pulsadores, &Send, portMAX_DELAY);
			vTaskDelay(1000/portTICK_RATE_MS);	//Espero 1s
		}
	}
	vTaskDelete(NULL);	//Borra la tarea
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* main
*/
int main (void)
{
	SystemCoreClockUpdate();

	/* Initializes GPIO */
	Chip_GPIO_Init(LPC_GPIO);

	uC_StartUp();

	vSemaphoreCreateBinary(Semaforo_RX1);			//Creamos el semaforo
	vSemaphoreCreateBinary(Semaforo_RX2);			//Creamos el semaforo
	vSemaphoreCreateBinary(Semaforo_RFID);
	xSemaphoreTake(Semaforo_RFID, portMAX_DELAY); //semaforo de inicializacion de rfid
	vSemaphoreCreateBinary(Semaforo_RTC);			//Creamos el semaforo

	Cola_RX1 = xQueueCreate(UART_RRB_SIZE, sizeof(uint8_t));	//Creamos una cola
	Cola_TX1 = xQueueCreate(UART_SRB_SIZE, sizeof(uint8_t));	//Creamos una cola
	Cola_RX2 = xQueueCreate(UART_RRB_SIZE, sizeof(uint8_t));	//Creamos una cola
	Cola_TX2 = xQueueCreate(UART_SRB_SIZE, sizeof(uint8_t));	//Creamos una cola
	Cola_Pulsadores = xQueueCreate(1, sizeof(uint32_t));	//Creamos una cola


	xTaskCreate(vTaskRFID, (char *) "vTaskRFID",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	xTaskCreate(xTaskRFIDConfig, (char *) "xTaskRFIDConfig",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 3UL),
				(xTaskHandle *) NULL);

	xTaskCreate(vTaskRTC, (char *) "vTaskRTC",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	xTaskCreate(xTaskRTConfig, (char *) "xTaskRTConfig",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 3UL),
				(xTaskHandle *) NULL);

	anillo_struct.Rx = Semaforo_RX1;
	anillo_struct.anillo = rxring1;
	anillo_struct.cola = Cola_RX1;

	xTaskCreate(vTaskLeerAnillo, (char *) "vTaskLeerAnillo1",
				configMINIMAL_STACK_SIZE, &anillo_struct, (tskIDLE_PRIORITY + 2UL),
				(xTaskHandle *) NULL);

	anillo_struct.Rx = Semaforo_RX2;
	anillo_struct.anillo = rxring2;
	anillo_struct.cola = Cola_RX2;

	xTaskCreate(vTaskLeerAnillo, (char *) "vTaskLeerAnillo2",
				configMINIMAL_STACK_SIZE, &anillo_struct, (tskIDLE_PRIORITY + 2UL),
				(xTaskHandle *) NULL);

	anillo_struct.Rx = Semaforo_RX1;
	anillo_struct.anillo = rxring1;
	anillo_struct.cola = Cola_RX1;
	anillo_struct.uart = LPC_UART1;

	xTaskCreate(vTaskCargarAnillo, (char *) "vTaskCargarAnillo1",
				configMINIMAL_STACK_SIZE, &anillo_struct, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	anillo_struct.Rx = Semaforo_RX2;
	anillo_struct.anillo = rxring2;
	anillo_struct.cola = Cola_RX2;
	anillo_struct.uart = LPC_UART2;

	xTaskCreate(vTaskCargarAnillo, (char *) "vTaskCargarAnillo2",
				configMINIMAL_STACK_SIZE, &anillo_struct, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	xTaskCreate(xTaskUART1Config, (char *) "xTaskUART1Config",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 3UL),
				(xTaskHandle *) NULL);

	xTaskCreate(xTaskUART2Config, (char *) "xTaskUART2Config",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 3UL),
				(xTaskHandle *) NULL);

	xTaskCreate(vTaskGSMConfig, (char *) "vTaskGSMConfig",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	xTaskCreate(vTaskEnviarGSM, (char *) "vTaskEnviarGSM",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	xTaskCreate(xTaskPulsadores, (char *) "vTaskPulsadores",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	/* Start the scheduler */
	vTaskStartScheduler();

	/* Nunca debería arribar aquí */
    return 0;
}
