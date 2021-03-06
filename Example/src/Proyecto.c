/*
===============================================================================
 Name        : FRTOS.c
 Author      : $(author)
 Version     :
 Copyright   : $(copyright)
 Description : main definition
===============================================================================
*/


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Includes */
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
#include <cr_section_macros.h>
#include "sdcard.h"
#include "fat32.h"
#include "GUI.h"
#include "LCD_X_SPI.h"
#include "Pantalla.h"
//#include "DIALOG.h"
//#include "GRAPH.h"
#include "Acelerometro.h"


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Defines */
//#define DEBUGOUT1(...) //printf(__VA_ARGS__)
//#define DEBUGOUT(...)  printf(__VA_ARGS__)
//#define DEBUGSTR(...) //printf(__VA_ARGS__)


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Declaraciones globales */
//static volatile bool fIntervalReached;
//static volatile bool fAlarmTimeMatched;
//static volatile bool On0, On1;


//__DATA(RAM2)	int last_balance = 0;
//__DATA(RAM2)	unsigned int last_user_ID;

//__DATA(RAM2)	uint8_t	ReceivePulsadores;
//__DATA(RAM2)	uint8_t Flag10sPantalla=OFF;

__DATA(RAM2) 	RINGBUFF_T txring1,txring2, rxring1,rxring2;					//Transmit and receive ring buffers
__DATA(RAM2)	static uint8_t 	rxbuff1[UART_RRB_SIZE], txbuff1[UART_SRB_SIZE],
								rxbuff2[UART_RRB_SIZE], txbuff2[UART_SRB_SIZE];

extern const GUI_BITMAP bmutnlogo; //declare external Bitmap
extern const GUI_BITMAP bmcontrol; //declare external Bitmap


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Semaforos */
__DATA(RAM2)	SemaphoreHandle_t Semaforo_RFID;
__DATA(RAM2)	SemaphoreHandle_t Semaforo_RTCgsm;
__DATA(RAM2)	SemaphoreHandle_t Semaforo_RX1;
__DATA(RAM2)	SemaphoreHandle_t Semaforo_RX2;
__DATA(RAM2)	SemaphoreHandle_t Semaforo_GSM_Closed;
__DATA(RAM2)	SemaphoreHandle_t Semaforo_GSM_Recibido;
__DATA(RAM2)	SemaphoreHandle_t Semaforo_SSP;
__DATA(RAM2)	SemaphoreHandle_t Semaforo_RTCsd;
__DATA(RAM2)	SemaphoreHandle_t Semaforo_YaHayTarj;
__DATA(RAM2)	SemaphoreHandle_t Semaforo_GSM_Enviar;
__DATA(RAM2)	SemaphoreHandle_t Semaforo_Tarjeta_Incorrecta;
__DATA(RAM2)	SemaphoreHandle_t Semaforo_SD;
__DATA(RAM2)	SemaphoreHandle_t Semaforo_Sist_Inic;
__DATA(RAM2)	SemaphoreHandle_t Semaforo_Reset10Seg;
__DATA(RAM2)	SemaphoreHandle_t Semaforo_Flag10Seg;
__DATA(RAM2)	SemaphoreHandle_t Semaforo_Habilitacion10Seg;
__DATA(RAM2)	SemaphoreHandle_t Semaforo_Muestras_Acelerometro;
__DATA(RAM2)	SemaphoreHandle_t Semaforo_Analisis_Acelerometro;
__DATA(RAM2)	SemaphoreHandle_t Semaforo_Vuelco;
__DATA(RAM2)	SemaphoreHandle_t Semaforo_Choque;
__DATA(RAM2)	SemaphoreHandle_t Semaforo_Reset30Seg;


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Colas */
__DATA(RAM2)	QueueHandle_t Cola_RX1,Cola_RX2;
__DATA(RAM2)	QueueHandle_t Cola_TX1,Cola_TX2;
__DATA(RAM2)	QueueHandle_t Cola_Pulsadores;
__DATA(RAM2)	QueueHandle_t Cola_Leds;
__DATA(RAM2)	QueueHandle_t Cola_SD;
__DATA(RAM2)	QueueHandle_t Cola_Datos_GPS;
__DATA(RAM2)	QueueHandle_t Cola_Datos_RFID;
__DATA(RAM2)	QueueHandle_t Cola_Inicio_Tarjetas;
__DATA(RAM2)	QueueHandle_t HoraEntrada;
__DATA(RAM2)	QueueHandle_t Cola_PromX;
__DATA(RAM2)	QueueHandle_t Cola_PromY;
__DATA(RAM2)	QueueHandle_t Cola_PromZ;
__DATA(RAM2)	QueueHandle_t Cola_Inicio_Lector;


eLeds Leds;

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* RTC_IRQHandler */
void RTC_IRQHandler(void)
{
	BaseType_t Testigo=pdFALSE,Testigo2=pdFALSE, Testigo3=pdFALSE, Testigo4=pdFALSE;
	static uint8_t i=0, j=0;;

	/* Interrupcion cada 1 seg */
	if (Chip_RTC_GetIntPending(LPC_RTC, RTC_INT_COUNTER_INCREASE))
	{
		/* Clear pending interrupt */
		Chip_RTC_ClearIntPending(LPC_RTC, RTC_INT_COUNTER_INCREASE);



		if(xSemaphoreTakeFromISR(Semaforo_Reset10Seg, &Testigo3) == pdTRUE )
		{
			j=0;
		}


		if(xSemaphoreTakeFromISR(Semaforo_Habilitacion10Seg, &Testigo4) == pdTRUE)// si puedo tomar el semaforo significa q esta en on
		{
			xSemaphoreGiveFromISR(Semaforo_Habilitacion10Seg,&Testigo4);
			j++;
			if(j>=10)
			{
				xSemaphoreGiveFromISR(Semaforo_Flag10Seg,&Testigo4);//significa que hay que cambiar pantalla
				j=0;
			}
		}

		i++;

		if(xSemaphoreTakeFromISR(Semaforo_Reset30Seg, &Testigo3) == pdTRUE )
		{
			i=0;
		}

		if(i>=30)
		{
			i=0;
			xSemaphoreGiveFromISR(Semaforo_RTCgsm, &Testigo);	//Devuelve si una de las tareas bloqueadas tiene mayor prioridad que la actual
			xSemaphoreGiveFromISR(Semaforo_RTCsd, &Testigo2);//grabo cada medio seg

			portYIELD_FROM_ISR(Testigo);					//Si testigo es TRUE -> ejecuta el scheduler
			portYIELD_FROM_ISR(Testigo2);					//Si testigo es TRUE -> ejecuta el scheduler

		}
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* UART1_IRQHandler */
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


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* xTaskRFIDConfig
 * Inicializacion del RFID
 */
static void xTaskRFIDConfig(void *pvParameters)
{

	__DATA(RAM2)	static	MFRC522Ptr_t mfrcInstance;	// RFID structs

	//vTaskDelay(5000/portTICK_RATE_MS);

	xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);

	//DEBUGOUT1("PRUEBA RFID..\n");	//Imprimo en la consola

	setupRFID(&mfrcInstance);
	xQueueOverwrite(Cola_Inicio_Lector,&mfrcInstance);

	Chip_GPIO_SetPinOutHigh(LPC_GPIO, RFID_SS);

	xSemaphoreGive(Semaforo_SSP);//Doy el semaforo para que se inicie la lectura

	vTaskDelete(NULL);	//Borra la tarea
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskRTConfig
 * Inicializacion del RTC utilizando la hora provista por el GPS
 * */
static void xTaskRTConfig(void *pvParameters)
{
	RTC_TIME_T FullTime;
	struct Datos_Nube informacion;
	uint16_t	aux;
	char pepe[4];

	//DEBUGOUT("PRUEBA RTC..\n");	//Imprimo en la consola

	Chip_RTC_Init(LPC_RTC);

	/* Seteo el tiempo con la hora brindada por el GPS */

	do
		{
		vTaskDelay(100/portTICK_RATE_MS);
		xQueuePeek(Cola_Datos_GPS, &informacion, portMAX_DELAY);

		FullTime.time[RTC_TIMETYPE_SECOND]  = 0;
		pepe[0] = informacion.hora[3]; pepe[1] = informacion.hora[4]; pepe[2] =0;
		aux = atoi (pepe);
		FullTime.time[RTC_TIMETYPE_MINUTE]  = aux;
		pepe[0] = informacion.hora[0]; pepe[1] = informacion.hora[1]; pepe[2] =0;
		aux = atoi (pepe);
		FullTime.time[RTC_TIMETYPE_HOUR]    = aux;
		pepe[0] = informacion.fecha[0]; pepe[1] = informacion.fecha[1]; pepe[2] =0;
		aux = atoi (pepe);
		FullTime.time[RTC_TIMETYPE_DAYOFMONTH]  = aux;
		FullTime.time[RTC_TIMETYPE_DAYOFWEEK]   = 4;
		FullTime.time[RTC_TIMETYPE_DAYOFYEAR]   = 207;
		pepe[0] = informacion.fecha[3]; pepe[1] = 0; pepe[2] =0;
		aux = atoi (pepe);
		FullTime.time[RTC_TIMETYPE_MONTH]   = aux;
		pepe[0] = informacion.fecha[5]; pepe[1] = informacion.fecha[6]; pepe[2] =0;
		aux = atoi (pepe);
		aux = aux + 2000;
		FullTime.time[RTC_TIMETYPE_YEAR]= aux;
		}while(aux < 2019 && aux > 2030);
	Chip_RTC_SetFullTime(LPC_RTC, &FullTime);
	//

	/* Set the RTC to generate an interrupt on each second */
	Chip_RTC_CntIncrIntConfig(LPC_RTC, RTC_AMR_CIIR_IMSEC, ENABLE);

	/* Clear interrupt pending */
	Chip_RTC_ClearIntPending(LPC_RTC, RTC_INT_COUNTER_INCREASE);

	/* Enable RTC interrupt in NVIC */
	NVIC_EnableIRQ((IRQn_Type) RTC_IRQn);

	/* Enable RTC (starts increase the tick counter and second counter register) */
	Chip_RTC_Enable(LPC_RTC, ENABLE);

	vTaskDelete(NULL);	//Borra la tarea
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* xTaskUART1Config */
static void xTaskUART1Config(void *pvParameters)
{
	//DEBUGOUT("Configurando la UART1..\n");	//Imprimo en la consola

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

	vTaskDelay(5000/portTICK_RATE_MS);

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
	//DEBUGOUT("Configurando la UART2..\n");	//Imprimo en la consola

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
/* vTaskGSMConfig
 * Inicializacion del GSM
 * */
static void vTaskGSMConfig(void *pvParameters)
{
	//Inicializacion del reset del GSM
	//Chip_GPIO_SetDir (LPC_GPIO, RST_GSM, INPUT);
	//Chip_IOCON_PinMux (LPC_IOCON, RST_GSM, IOCON_MODE_INACT, IOCON_FUNC0);

	Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT\r\n", sizeof("AT\r\n") - 1); //Enviamos "AT"
	vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms
	Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT\r\n", sizeof("AT\r\n") - 1); //Enviamos "AT"
	vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms
	Chip_UART_SendRB(UART_SELECTION_GSM, &TX_RING_GSM, "AT\r\n", sizeof("AT\r\n") - 1); //Enviamos "AT"
	vTaskDelay(100/portTICK_RATE_MS);	//Espero 100ms

	vTaskDelete(NULL);	//Borra la tarea
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskCargarAnillo */
//Encargada de cargar el anillo a partir de la cola
static void vTaskCargarAnillo1(void *pvParameters)
{
	uint8_t Receive=0;
	while (1)
	{
		xQueueReceive(Cola_TX1, &Receive, portMAX_DELAY);
		Chip_UART_SendRB(LPC_UART1, &txring1, &Receive , 1);
	}
	vTaskDelete(NULL);	//Borra la tarea si sale del while
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskLeerAnillo */
//Pasa la informacion del anillo a la cola de recepcion
static void vTaskLeerAnillo1(void *pvParameters)
{
	uint8_t Receive=0;
	uint8_t Testigo=0;

	while (1)
	{
		xSemaphoreTake(Semaforo_RX1, portMAX_DELAY);
		Testigo = RingBuffer_Pop(&rxring1, &Receive);
		if(Testigo)
		{
			xSemaphoreGive(Semaforo_RX1);
			xQueueSendToBack(Cola_RX1, &Receive, portMAX_DELAY);
		}
	}
	vTaskDelete(NULL);	//Borra la tarea si sale del while
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskCargarAnillo */
//Encargada de cargar el anillo a partir de la cola
static void vTaskCargarAnillo2(void *pvParameters)
{
	uint8_t Receive=0;
	while (1)
	{
		xQueueReceive(Cola_TX2, &Receive, portMAX_DELAY);
		Chip_UART_SendRB(LPC_UART2, &txring2, &Receive, 1);
	}
	vTaskDelete(NULL);	//Borra la tarea si sale del while
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskLeerAnillo */
//Pasa la informacion del anillo a la cola de recepcion
static void vTaskLeerAnillo2(void *pvParameters)
{
	uint8_t Receive=0;
	uint8_t Testigo=0;

	while (1)
	{
		xSemaphoreTake(Semaforo_RX2, portMAX_DELAY);
		Testigo = RingBuffer_Pop(&rxring2, &Receive);
		if(Testigo)
		{
			xSemaphoreGive(Semaforo_RX2);
			xQueueSendToBack(Cola_RX2, &Receive, portMAX_DELAY);
		}
	}
	vTaskDelete(NULL);	//Borra la tarea si sale del while
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskRFID
 * Se leen las tarjetas RFID, se las compara con las descargadas y se almacena la actual en la cola Cola_Datos_RFID
 * */
static void vTaskRFID(void *pvParameters)
{

	__DATA(RAM2)	static	MFRC522Ptr_t mfrcInstance;	// RFID structs

	xSemaphoreTake(Semaforo_GSM_Recibido,portMAX_DELAY);//me aseguro que ya se hayan cargado las tarjetas
	xSemaphoreGive(Semaforo_GSM_Recibido);
	xQueueReceive(Cola_Inicio_Lector,&mfrcInstance,portMAX_DELAY);

	while (1)
	{
		xSemaphoreTake(Semaforo_YaHayTarj, portMAX_DELAY);
		xSemaphoreGive(Semaforo_YaHayTarj);
		xSemaphoreTake(Semaforo_SSP, portMAX_DELAY); //semaforo de uso de ssp
    	Chip_GPIO_SetPinOutHigh(LPC_GPIO, TFT_DC);//Para que no se resetie la pantalla

		// Look for new cards in RFID
		if (PICC_IsNewCardPresent(mfrcInstance))
		{
			// Select one of the cards
			if (PICC_ReadCardSerial(mfrcInstance))
			{
				//int status = writeCardBalance(mfrcInstance, 100000); // used to recharge the card
				userTapIn(mfrcInstance);
				Chip_GPIO_SetPinOutHigh(LPC_GPIO, RFID_SS);

			}
		}

    	Chip_GPIO_SetPinOutLow(LPC_GPIO, TFT_DC);//Para que no se resetie la pantalla
		Chip_GPIO_SetPinOutHigh(LPC_GPIO, RFID_SS);
		xSemaphoreGive(Semaforo_SSP);
		vTaskDelay( 500 / portTICK_PERIOD_MS );//Muestreo cada medio seg
	}

	vTaskDelete(NULL);	//Borra la tarea si sale del while
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskAnalizarGPS
 * Leo lo que me manda el GPS CADA 5 seg, los datos intermedios los desecho. Los datos se cargan en Cola_Datos_GPS
 * */
static void vTaskAnalizarGPS(void *pvParameters)
{
	uint8_t dato=0;
	uint8_t i;

	while (1)
	{
		for(i=0;i<5;i++)
		{
			xQueueReset(RX_COLA_GPS);
			vTaskDelay(1000/portTICK_RATE_MS); //1seg
		}

		while(LeerCola(RX_COLA_GPS,&dato,1))
		{
			AnalizarTramaGPS(dato); // pasa la struct en una cola dentro de la funcion
			//DEBUGOUT("%c", dato);	//Imprimo en la consola
		}
	}
	vTaskDelete(NULL);	//Borra la tarea
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskEnviarGSM
 * Envia periodicamente (semaforo dado por la interrupcion del RTC) mensajes a thingspeak con lat long vel y usuario
 * */
static void vTaskEnviarGSM(void *pvParameters)
{
	struct Datos_Nube informacion;
	Tarjetas_RFID informacionRFID;

	xSemaphoreTake(Semaforo_GSM_Recibido,portMAX_DELAY);//me aseguro que ya se hayan cargado las tarjetas
	xSemaphoreGive(Semaforo_GSM_Recibido);
	while(1)
	{
		//Para enviar datos por GPRS a ThingSpeak
		xSemaphoreTake(Semaforo_RTCgsm,portMAX_DELAY);//hago que envie cada medio min la informacion
		xQueuePeek(Cola_Datos_GPS, &informacion, portMAX_DELAY);
		xQueuePeek(Cola_Datos_RFID, &informacionRFID, portMAX_DELAY);
		xSemaphoreTake(Semaforo_GSM_Enviar,portMAX_DELAY);//Me aseguro de estar enviando solo esta tarea
		EnviarTramaGSM(informacion.latitud,informacion.longitud, informacionRFID.tarjeta, informacion.velocidad);
		xSemaphoreGive(Semaforo_GSM_Enviar);

		if(informacion.velocidad > VELOCIDADMAXIMA)
		{
			Chip_GPIO_SetPinOutHigh(LPC_GPIO, BUZZER);
		}
		else
		{
			Chip_GPIO_SetPinOutLow(LPC_GPIO, BUZZER);
		}
	}
	vTaskDelete(NULL);	//Borra la tarea
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskTarjetasGSM*/
//Lee las tarjetas de Thingspeak y las almacena en un vector estatico
static void vTaskTarjetasGSM(void *pvParameters)
{
	Tarjetas_RFID* InicioTarjetas=NULL;//Puntero al inicio del vector de tarjetas
	uint8_t dato=0;

	//tarea que llamo al iniciar el programa y luego la borro

    while (InicioTarjetas == NULL)
	{
    	//xQueueReset(RX_COLA_GSM);
    	RecibirTramaGSM();
    	vTaskDelay(3000/portTICK_RATE_MS);//espero 5 seg para asegurarme que llego tod o
		while(LeerCola(RX_COLA_GSM,&dato,1))
		{
			InicioTarjetas = AnalizarTramaGSMrecibido(dato);
			//DEBUGOUT("%c", dato);	//Imprimo en la consola
		}
	}

	xQueueOverwrite(Cola_Inicio_Tarjetas,&InicioTarjetas);// envio a una cola el comienzo del vector de tarjetas
	xSemaphoreGive(Semaforo_GSM_Recibido);
	Chip_GPIO_SetPinOutHigh(LPC_GPIO, BUZZER);
	vTaskDelay(1000/portTICK_RATE_MS);	//Espero 1s
	Chip_GPIO_SetPinOutLow(LPC_GPIO, BUZZER);


	vTaskDelete(NULL);	//Borra la tarea si sale del while 1
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* xTaskPulsadores
 * Lectura de los pulsadores mediante una cola
 * */
static void xTaskPulsadores(void *pvParameters)
{
	static uint8_t ReceivePulsadores=0;

	xSemaphoreTake(Semaforo_Sist_Inic, portMAX_DELAY);// me fijo que este inicializada el resto del sistema
	xSemaphoreGive(Semaforo_Sist_Inic);

	while (1)
	{

		if(Chip_GPIO_GetPinState(LPC_GPIO, SW1)==OFF)	//Si se presiona el SW1
		{
			ReceivePulsadores++;	//Envio a la cola que se presiono el SW1
		}
		if(Chip_GPIO_GetPinState(LPC_GPIO, SW2)==OFF)	//Si se presiona el SW2
		{
			ReceivePulsadores=ReceivePulsadores+10;
		}

		if(ReceivePulsadores != 0)
		{
			xQueueSendToBack(Cola_Pulsadores,&ReceivePulsadores,portMAX_DELAY);// envio a una cola el valor de los pulsadores
			ReceivePulsadores=0;
		}

		while(Chip_GPIO_GetPinState(LPC_GPIO, SW2)==OFF || Chip_GPIO_GetPinState(LPC_GPIO, SW1)==OFF)
		{
			vTaskDelay(50/portTICK_RATE_MS); // Espero 50 mseg y vuelvo a comprobar si solto
		}

		vTaskDelay(300/portTICK_RATE_MS);	//Espero medio seg
	}
	vTaskDelete(NULL);	//Borra la tarea
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* xTaskLeds
 * Accion sobre los leds
 * */
static void xTaskLeds(void *pvParameters)
{
	Leds = ROJO_TITILANDO;
	xQueueSendToBack(Cola_Leds, &Leds, portMAX_DELAY);

	while (1)
	{
		xQueueReceive(Cola_Leds, &Leds, 0);

		switch ( Leds )
		{
			case TODO_APAGADO:
				Chip_GPIO_SetPinOutLow(LPC_GPIO, LED_ROJO);
				Chip_GPIO_SetPinOutLow(LPC_GPIO, LED_VERDE);
				break;

			case TODO_PRENDIDO:
				Chip_GPIO_SetPinOutHigh(LPC_GPIO, LED_ROJO);
				Chip_GPIO_SetPinOutHigh(LPC_GPIO, LED_VERDE);
				break;

			case ROJO_PRENDIDO:
				Chip_GPIO_SetPinOutHigh(LPC_GPIO, LED_ROJO);
				Chip_GPIO_SetPinOutLow(LPC_GPIO, LED_VERDE);
				break;

			case VERDE_PRENDIDO:
				Chip_GPIO_SetPinOutHigh(LPC_GPIO, LED_VERDE);
				Chip_GPIO_SetPinOutLow(LPC_GPIO, LED_ROJO);
				break;

			case ROJO_TITILANDO:
				Chip_GPIO_SetPinOutLow(LPC_GPIO, LED_VERDE);
				Chip_GPIO_SetPinToggle(LPC_GPIO, LED_ROJO);
				break;

			case VERDE_TITILANDO:
				Chip_GPIO_SetPinOutLow(LPC_GPIO, LED_ROJO);
				Chip_GPIO_SetPinToggle(LPC_GPIO, LED_VERDE);
				break;

			case TODO_TITILANDO:
				Chip_GPIO_SetPinToggle(LPC_GPIO, LED_ROJO);
				Chip_GPIO_SetPinToggle(LPC_GPIO, LED_VERDE);
				break;
		}

		vTaskDelay(500/portTICK_RATE_MS);

	}
	vTaskDelete(NULL);	//Borra la tarea
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskInicSD
 * Inicializacion de la tarjeta SD
 * */
static void vTaskInicSD(void *pvParameters)
{
    uint8_t returnStatus,sdcardType;

	/* PARA COMPROBAR SI LA SD ESTA CONECTADA */
	xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);
	Chip_GPIO_SetPinOutHigh(LPC_GPIO, TFT_DC);

	do //if(returnStatus)
    {
        returnStatus = SD_Init(&sdcardType);
        if(returnStatus == SDCARD_NOT_DETECTED)
        {
        	//DEBUGOUT("\n\r SD card not detected..");
        }
        else if(returnStatus == SDCARD_INIT_FAILED)
        {
        	//DEBUGOUT("\n\r Card Initialization failed..");
        }
        else if(returnStatus == SDCARD_FAT_INVALID)
        {
        	//DEBUGOUT("\n\r Invalid Fat filesystem");
        }
    }while(returnStatus!=0);

    //DEBUGOUT("\n\rSD Card Detected!");

	Chip_GPIO_SetPinOutHigh(LPC_GPIO, TFT_DC);
	xSemaphoreGive(Semaforo_SSP);

	xSemaphoreGive(Semaforo_SD);// me fijo que este inicializada la SD

    vTaskDelete(NULL);	//Borra la tarea
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* xTaskWriteSD
 * Escritura periodica de la tarjeta SD (semaforo dado por la interrupcion del RTC)
 * */
static void xTaskWriteSD(void *pvParameters)
{
    uint8_t returnStatus,i=0;
    static	fileConfig_st *srcFilePtr = NULL;
    static	char Receive[100];

	xSemaphoreTake(Semaforo_SD, portMAX_DELAY);// me fijo que este inicializada la SD
	/* PARA ESCRIBIR ARCHIVO */
    do
    {
    	srcFilePtr = FILE_Open("datalog.txt",WRITE,&returnStatus);
    }while(srcFilePtr == 0);

    while (1)
	{

        xSemaphoreTake(Semaforo_RTCsd, portMAX_DELAY);//grabo cada medio seg
    	xSemaphoreTake(Semaforo_Sist_Inic, portMAX_DELAY);// me fijo que este inicializada el resto del sistema
    	xSemaphoreGive(Semaforo_Sist_Inic);

        xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);// me fijo que este disponible el canal ssp
    	Chip_GPIO_SetPinOutHigh(LPC_GPIO, TFT_DC);//Para que no se resetie la pantalla

		taskENTER_CRITICAL();//Hago que no pueda cambiar de tarea mientras este aca

        InfoSd(Receive);//En la funcion se reciben los datos a guardar y se los coloca en una trama

        i=0;


        while(Receive[i] != 0)
		{
		   FILE_PutCh(srcFilePtr,Receive[i++]);
		}
        FILE_PutCh(srcFilePtr,EOF);
        FILE_Close(srcFilePtr);
        FILE_Open("datalog.txt",APPEND,&returnStatus);

    	Chip_GPIO_SetPinOutHigh(LPC_GPIO, SD_CS);
    	Chip_GPIO_SetPinOutLow(LPC_GPIO, TFT_DC);//Para que no se resetie la pantalla
    	xSemaphoreGive(Semaforo_SSP);

    	taskEXIT_CRITICAL();

        for(i=0;Receive[i] != 0 || i<100;i++)//limpio el vector
        {
        	Receive[i]=0;
        }


	}
	vTaskDelete(NULL);	//Borra la tarea si sale del while 1
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskTFT
 *  Encargada del manejo de lo que se muestra en pantalla como asi las decisiones que se toman en la misma
 * */
void vTaskTFT(void *pvParameters)
{
	__DATA(RAM2)	static uint8_t EstadoPantalla=3;
	__DATA(RAM2)	static uint8_t FlagEstado=ON;
	uint8_t ReceivePulsadores;
	uint8_t primeraEntrada=ON;
	//struct Datos_Nube	ReceiveGPS;
	uint32_t dia, mes, ano, hora, velocidad;
	char nombre [20];
	__DATA(RAM2)	static uint8_t minConductor=0, horaConductor=0;
	__DATA(RAM2)	static uint8_t mensaje=0;
	__DATA(RAM2)	static uint32_t	minAnt, minutos;
	RTC_TIME_T pFullTime;
	struct Datos_Nube informacion;
	Tarjetas_RFID informacionRFID;

	xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);
	Chip_GPIO_SetPinOutLow(LPC_GPIO, TFT_CS);
	GUI_Init();

	/*
	GUI_Clear();
	GUI_DrawBitmap(&bmutnlogo, 0, 0);
	vTaskDelay(1000/portTICK_PERIOD_MS);
	GUI_Clear();
	GUI_DrawBitmap(&bmcontrol, 0, 0);
	 */

	GUI_SetBkColor(0x00000000); //gris claro 0x00D3D3D3
	Chip_GPIO_SetPinOutHigh(LPC_GPIO, TFT_CS);
	xSemaphoreGive(Semaforo_SSP);

	Chip_RTC_GetFullTime(LPC_RTC, &pFullTime);
	minutos=pFullTime.time[1];
	hora=pFullTime.time[2];
	dia=pFullTime.time[3];
	mes=pFullTime.time[6];
	ano=pFullTime.time[7];

	minAnt=minutos;

	while(1)
	{

		vTaskDelay(50/portTICK_RATE_MS);
		if(xQueueReceive(Cola_Pulsadores, &ReceivePulsadores, 10/portTICK_RATE_MS) == pdFALSE )
		{
			ReceivePulsadores=0;// si no hay nada en la cola pongo en 0 la variable
		}

		if(xSemaphoreTake(Semaforo_Vuelco,10 / portTICK_PERIOD_MS) == pdTRUE)
		{
			EstadoPantalla=6;
			FlagEstado=ON;
			mensaje=3; // Mensaje por vuelco
		}

		if(xSemaphoreTake(Semaforo_Choque,10 / portTICK_PERIOD_MS) == pdTRUE)
		{
			EstadoPantalla=6;
			FlagEstado=ON;
			mensaje=4; // Mensaje por choque
		}

		Chip_RTC_GetFullTime(LPC_RTC, &pFullTime);
		minutos=pFullTime.time[1];
		hora=pFullTime.time[2];
		dia=pFullTime.time[3];
		mes=pFullTime.time[6];
		ano=pFullTime.time[7];


		switch(EstadoPantalla)
		{
			case 0:		//PANTALLA PRINCIPAL CON USUARIO REGISTRADO (HORA)
				if(FlagEstado==ON)
				{
					if(primeraEntrada==ON)
					{
						primeraEntrada=OFF;
						minAnt=minutos;
					}

					Leds = VERDE_PRENDIDO;
					xQueueSendToBack(Cola_Leds, &Leds, portMAX_DELAY);

					xQueuePeek(Cola_Datos_RFID, &informacionRFID, portMAX_DELAY);
					strcpy(nombre,informacionRFID.nombre);

					xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);
					GUI_Clear();
					minAnt=minutos;
					FlagEstado=OFF;
					xSemaphoreGive(Semaforo_Reset10Seg);
					xSemaphoreGive(Semaforo_Habilitacion10Seg);
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringAt("/     /",127,10);
					GUI_DispDecAt(dia,101,10,2);
					GUI_DispDecAt(mes,137,10,2);
					GUI_DispDecAt(ano,172,10,4);

					GUI_SetFont(GUI_FONT_D80);
					GUI_DispDecAt(hora,32,50,2);
					GUI_DispDecAt(minutos,165,50,2);
					GUI_SetFont(GUI_FONT_32B_ASCII);
					GUI_DispStringAt(".",155,80);
					GUI_DispStringAt(".",155,55);

					GUI_SetFont(GUI_FONT_32B_ASCII);
					GUI_DispStringHCenterAt(nombre,160,150);

					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("MENU",80,210);
					GUI_DispStringHCenterAt("S.O.S.",240,210);

					GUI_DrawHLine(195,0,320);
					GUI_DrawHLine(200,0,320);
					Chip_GPIO_SetPinOutHigh(LPC_GPIO, TFT_CS);
					xSemaphoreGive(Semaforo_SSP);
				}
				if(ReceivePulsadores==1)		//Paso al estado HORAS DE TRABAJO
				{
					EstadoPantalla=1;
					FlagEstado=ON;
				}
				else if(ReceivePulsadores==10)	//Paso al estado CONFIRMAR SMS
				{
					EstadoPantalla=10;
					FlagEstado=ON;
					mensaje = 1; //Debe enviarse el mensaje de emergencia
				}

				else if(xSemaphoreTake(Semaforo_Flag10Seg,10 / portTICK_PERIOD_MS) == pdTRUE)
				{
					EstadoPantalla=8;
					FlagEstado=ON;
					xSemaphoreTake(Semaforo_Habilitacion10Seg, 10 / portTICK_PERIOD_MS);//Deshabilito el conteo
				}


			break;

			case 1:		//HORAS DE TRABAJO
				if(FlagEstado==ON)
				{

					xQueuePeek(Cola_Datos_RFID, &informacionRFID, portMAX_DELAY);
					strcpy(nombre,informacionRFID.nombre);

					xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);
					GUI_Clear();
					xSemaphoreGive(Semaforo_Reset10Seg);//reseteo cuenta
					xSemaphoreGive(Semaforo_Habilitacion10Seg); //habilito conteo
					FlagEstado=OFF;
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt(nombre,100,10);
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispDecAt(hora,240,10,2);
					GUI_DispStringAt(":",264,10);
					GUI_DispDecAt(minutos,271,10,2);

					GUI_DrawHLine(40,0,320);
					GUI_DrawHLine(45,0,320);

					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("HORAS DE TRABAJO",160,75);
					GUI_SetFont(GUI_FONT_32B_ASCII);
					GUI_DispStringHCenterAt(":",160,120);
					GUI_SetFont(GUI_FONT_D48);
					GUI_DispDecAt(horaConductor,80,115,2);
					GUI_DispDecAt(minConductor,165,115,2);

					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("SIGUIENTE",80,210);

					GUI_DrawHLine(195,0,320);
					GUI_DrawHLine(200,0,320);
					Chip_GPIO_SetPinOutHigh(LPC_GPIO, TFT_CS);
					xSemaphoreGive(Semaforo_SSP);
				}
				if(ReceivePulsadores==1)		//Paso al estado SMS CONTACTO
				{
					EstadoPantalla=2;
					FlagEstado=ON;

				}


				else if(xSemaphoreTake(Semaforo_Flag10Seg,10 / portTICK_PERIOD_MS) == pdTRUE)
				{
					EstadoPantalla=0;
					FlagEstado=ON;
					xSemaphoreTake(Semaforo_Habilitacion10Seg, 10 / portTICK_PERIOD_MS);//Deshabilito el conteo
				}

			break;

			case 2:		//SMS CONTACTO
				if(FlagEstado==ON)
				{

					xQueuePeek(Cola_Datos_RFID, &informacionRFID, portMAX_DELAY);
					strcpy(nombre,informacionRFID.nombre);

					xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);
					GUI_Clear();
					FlagEstado=OFF;
					xSemaphoreGive(Semaforo_Reset10Seg);
					xSemaphoreGive(Semaforo_Habilitacion10Seg);
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt(nombre,100,10);
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispDecAt(hora,240,10,2);
					GUI_DispStringAt(":",264,10);
					GUI_DispDecAt(minutos,271,10,2);

					GUI_DrawHLine(40,0,320);
					GUI_DrawHLine(45,0,320);

					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("PARA ENVIAR UN SMS A",160,75);
					GUI_DispStringHCenterAt("LA CENTRAL, PRESIONE",160,110);
					GUI_DispStringHCenterAt("CONFIRMAR...",160,145);

					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("SIGUIENTE",80,210);
					GUI_DispStringHCenterAt("CONFIRMAR",240,210);

					GUI_DrawHLine(195,0,320);
					GUI_DrawHLine(200,0,320);
					Chip_GPIO_SetPinOutHigh(LPC_GPIO, TFT_CS);
					xSemaphoreGive(Semaforo_SSP);
				}
				if(ReceivePulsadores==1)		//Paso al estado SALIDA CONDUCTOR
				{
					EstadoPantalla=5;
					FlagEstado=ON;

				}
				else if(ReceivePulsadores==10)	//Paso al estado CONFIRMAR SMS
				{
					EstadoPantalla=10;
					FlagEstado=ON;

					mensaje=2; // Mensaje para comunicarse con el conductor
				}

				else if(xSemaphoreTake(Semaforo_Flag10Seg,10 / portTICK_PERIOD_MS) == pdTRUE)
				{
					EstadoPantalla=0;
					FlagEstado=ON;
					xSemaphoreTake(Semaforo_Habilitacion10Seg, 10 / portTICK_PERIOD_MS);//Deshabilito el conteo
				}

			break;

			case 3:		//INICIALIZANDO SISTEMA
				xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);

				GUI_Clear();

				GUI_SetFont(GUI_FONT_32B_ASCII);
				GUI_DispStringHCenterAt("INICIALIZANDO",160,90);
				GUI_DispStringHCenterAt("SISTEMA...",160,120);
				Chip_GPIO_SetPinOutHigh(LPC_GPIO, TFT_CS);
				xSemaphoreGive(Semaforo_SSP);

				xQueuePeek(Cola_Datos_GPS, &informacion, portMAX_DELAY);
				xSemaphoreTake(Semaforo_GSM_Recibido,portMAX_DELAY);//me aseguro que ya se hayan cargado las tarjetas
				xSemaphoreGive(Semaforo_GSM_Recibido);

				Leds = VERDE_TITILANDO;
				xQueueSendToBack(Cola_Leds, &Leds, portMAX_DELAY);

				FlagEstado=ON;
				EstadoPantalla=7;
			break;

			case 4:		//TARJETA INCORRECTA
				if(FlagEstado==ON)
				{
					xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);

					GUI_Clear();
					FlagEstado=OFF;
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispDecAt(hora,240,10,2);
					GUI_DispStringAt(":",264,10);
					GUI_DispDecAt(minutos,271,10,2);

					GUI_DrawHLine(40,0,320);
					GUI_DrawHLine(45,0,320);

					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("TARJETA INCORRECTA",160,110);

					GUI_DrawHLine(195,0,320);
					GUI_DrawHLine(200,0,320);
					Chip_GPIO_SetPinOutHigh(LPC_GPIO, TFT_CS);
					xSemaphoreGive(Semaforo_SSP);
				}
				vTaskDelay(2000/portTICK_PERIOD_MS);
				EstadoPantalla=7;
				FlagEstado=ON;
			break;

			case 5:		//SALIDA CONDUCTOR
				if(FlagEstado==ON)
				{

					xQueuePeek(Cola_Datos_RFID, &informacionRFID, portMAX_DELAY);
					strcpy(nombre,informacionRFID.nombre);

					xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);
					GUI_Clear();
					FlagEstado=OFF;
					xSemaphoreGive(Semaforo_Reset10Seg);
					xSemaphoreGive(Semaforo_Habilitacion10Seg);
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt(nombre,100,10);
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispDecAt(hora,240,10,2);
					GUI_DispStringAt(":",264,10);
					GUI_DispDecAt(minutos,271,10,2);

					GUI_DrawHLine(40,0,320);
					GUI_DrawHLine(45,0,320);

					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt(" PARA CONFIRMAR SALIDA",160,75);
					GUI_DispStringHCenterAt(" DE CONDUCTOR, PRESIONE",160,110);
					GUI_DispStringHCenterAt(" CONFIRMAR... ",160,145);

					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("SIGUIENTE",80,210);
					GUI_DispStringHCenterAt("CONFIRMAR",240,210);

					GUI_DrawHLine(195,0,320);
					GUI_DrawHLine(200,0,320);
					Chip_GPIO_SetPinOutHigh(LPC_GPIO, TFT_CS);
					xSemaphoreGive(Semaforo_SSP);
				}
				if(ReceivePulsadores==1)		//Paso al estado PANTALLA PRINCIPAL
				{
					EstadoPantalla=1;
					FlagEstado=ON;

				}
				else if(ReceivePulsadores==10)		//Paso al estado CONFIRMAR SALIDA
				{
					EstadoPantalla=9;
					FlagEstado=ON;

				}

				else if(xSemaphoreTake(Semaforo_Flag10Seg,10 / portTICK_PERIOD_MS) == pdTRUE)
				{
					EstadoPantalla=0;
					FlagEstado=ON;
					xSemaphoreTake(Semaforo_Habilitacion10Seg, 10 / portTICK_PERIOD_MS);//Deshabilito el conteo
				}
			break;

			case 6:		//MENSAJE ENVIADO

				xQueuePeek(Cola_Datos_RFID, &informacionRFID, portMAX_DELAY);
				strcpy(nombre,informacionRFID.nombre);

				xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);
				GUI_Clear();
				GUI_SetFont(GUI_FONT_24B_ASCII);
				GUI_DispStringHCenterAt(nombre,100,10);
				GUI_SetFont(GUI_FONT_24B_ASCII);
				GUI_DispDecAt(hora,240,10,2);
				GUI_DispStringAt(":",264,10);
				GUI_DispDecAt(minutos,271,10,2);

				GUI_DrawHLine(40,0,320);
				GUI_DrawHLine(45,0,320);

				if(mensaje == 3)
				{
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("ENVIANDO MENSAJE",160,90);
					GUI_DispStringHCenterAt("VUELCO DETECTADO",160,130);
				}

				else if(mensaje == 4)
				{
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("ENVIANDO MENSAJE",160,90);
					GUI_DispStringHCenterAt("CHOQUE DETECTADO",160,130);
				}
				else
				{
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("ENVIANDO MENSAJE",160,110);
				}
				GUI_DrawHLine(195,0,320);
				GUI_DrawHLine(200,0,320);
				Chip_GPIO_SetPinOutHigh(LPC_GPIO, TFT_CS);
				xSemaphoreGive(Semaforo_SSP);


				//Para enviar SMS
				xSemaphoreTake(Semaforo_GSM_Enviar,portMAX_DELAY);//Me aseguro de estar enviando solo esta tarea
				EnviarMensajeGSM(mensaje); //Se pasa como parametro el mensaje PREDEFINIDO a enviar
				xSemaphoreGive(Semaforo_GSM_Enviar);

				Chip_GPIO_SetPinOutHigh(LPC_GPIO, BUZZER);
				vTaskDelay(1000/portTICK_PERIOD_MS);
				Chip_GPIO_SetPinOutLow(LPC_GPIO, BUZZER);

				xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);
				GUI_Clear();
				GUI_SetFont(GUI_FONT_24B_ASCII);
				GUI_DispStringHCenterAt(nombre,100,10);
				GUI_SetFont(GUI_FONT_24B_ASCII);
				GUI_DispDecAt(hora,240,10,2);
				GUI_DispStringAt(":",264,10);
				GUI_DispDecAt(minutos,271,10,2);

				GUI_DrawHLine(40,0,320);
				GUI_DrawHLine(45,0,320);

				if(mensaje == 3)
				{
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("MENSAJE ENVIADO",160,90);
					GUI_DispStringHCenterAt("VUELCO DETECTADO",160,130);
				}
				else if(mensaje == 4)
				{
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("ENVIANDO MENSAJE",160,90);
					GUI_DispStringHCenterAt("CHOQUE DETECTADO",160,130);
				}
				else
				{
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("MENSAJE ENVIADO",160,110);
				}

				GUI_DrawHLine(195,0,320);
				GUI_DrawHLine(200,0,320);
				Chip_GPIO_SetPinOutHigh(LPC_GPIO, TFT_CS);
				xSemaphoreGive(Semaforo_SSP);

				mensaje=0;
				vTaskDelay(2000/portTICK_PERIOD_MS);
				EstadoPantalla=0;
			break;

			case 7:		//PANTALLA PRINCIPAL SIN USUARIO REGISTRADO
				if(FlagEstado==ON)
				{
					xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);

					GUI_Clear();
					FlagEstado=OFF;
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringAt("/     /",127,10);
					GUI_DispDecAt(dia,101,10,2);
					GUI_DispDecAt(mes,137,10,2);
					GUI_DispDecAt(ano,172,10,4);

					GUI_SetFont(GUI_FONT_32B_ASCII);
					GUI_DispStringHCenterAt("POR FAVOR",160,60);
					GUI_DispStringHCenterAt("ACERQUE SU TARJETA",160,100);

					GUI_SetFont(GUI_FONT_D48);
					GUI_DispDecAt(hora,82,155,2);
					GUI_DispDecAt(minutos,170,155,2);
					GUI_SetFont(GUI_FONT_32B_ASCII);
					GUI_DispStringAt(".",158,145);
					GUI_DispStringAt(".",158,170);
					Chip_GPIO_SetPinOutHigh(LPC_GPIO, TFT_CS);
					xSemaphoreGive(Semaforo_SSP);

					Leds = VERDE_TITILANDO;
					xQueueSendToBack(Cola_Leds, &Leds, portMAX_DELAY);
				}
				if(uxQueueMessagesWaiting(Cola_Datos_RFID)!=0)
				{
					FlagEstado=ON;
					EstadoPantalla=0;
					xSemaphoreGive(Semaforo_Reset10Seg);
					xSemaphoreGive(Semaforo_Reset30Seg);
					xSemaphoreGive(Semaforo_Habilitacion10Seg);
					xSemaphoreTake(Semaforo_YaHayTarj, portMAX_DELAY);
					xSemaphoreGive(Semaforo_Sist_Inic);
					ReceivePulsadores=0;
					Chip_GPIO_SetPinOutHigh(LPC_GPIO, BUZZER);
					vTaskDelay(250/portTICK_PERIOD_MS);
					Chip_GPIO_SetPinOutLow(LPC_GPIO, BUZZER);
					minConductor=0;//Reseteo las horas de trabajo
					horaConductor=0;
				}
				if(xSemaphoreTake(Semaforo_Tarjeta_Incorrecta,10/portTICK_PERIOD_MS)==ON)
				{
					EstadoPantalla=4;
					FlagEstado=ON;
				}
				if(minAnt!=minutos)
				{
					FlagEstado=ON;
				}
			break;

			case 8:		//PANTALLA PRINCIPAL CON USUARIO REGISTRADO (VELOCIDAD)

				xQueuePeek(Cola_Datos_GPS, &informacion, portMAX_DELAY);
				velocidad=informacion.velocidad;

				xQueuePeek(Cola_Datos_RFID, &informacionRFID, portMAX_DELAY);
				strcpy(nombre,informacionRFID.nombre);

				if(FlagEstado==ON)
				{
					xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);

					GUI_Clear();
					FlagEstado=OFF;
					xSemaphoreGive(Semaforo_Reset10Seg);
					xSemaphoreGive(Semaforo_Habilitacion10Seg);
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringAt("/     /",127,10);
					GUI_DispDecAt(dia,101,10,2);
					GUI_DispDecAt(mes,137,10,2);
					GUI_DispDecAt(ano,172,10,4);

					if(velocidad<=99)
					{
						GUI_SetFont(GUI_FONT_D80);
						GUI_DispDecAt(velocidad,100,50,2);
						GUI_SetFont(GUI_FONT_32B_ASCII);
						GUI_DispStringHCenterAt("Km/h",272,80);
					}
					else
					{
						GUI_SetFont(GUI_FONT_D80);
						GUI_DispDecAt(velocidad,100,50,3);
						GUI_SetFont(GUI_FONT_32B_ASCII);
						GUI_DispStringHCenterAt("Km/h",300,80);
					}

					GUI_SetFont(GUI_FONT_32B_ASCII);
					GUI_DispStringHCenterAt(nombre,160,150);

					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("MENU",80,210);
					GUI_DispStringHCenterAt("S.O.S.",240,210);

					GUI_DrawHLine(195,0,320);
					GUI_DrawHLine(200,0,320);
				}
				if(velocidad<=99)
				{
					GUI_SetFont(GUI_FONT_D80);
					GUI_DispDecAt(velocidad,100,50,2);
					GUI_SetFont(GUI_FONT_32B_ASCII);
					GUI_DispStringHCenterAt("Km/h",272,80);
				}
				else
				{
					GUI_SetFont(GUI_FONT_D80);
					GUI_DispDecAt(velocidad,100,50,3);
					GUI_SetFont(GUI_FONT_32B_ASCII);
					GUI_DispStringHCenterAt("Km/h",300,80);
				}
				Chip_GPIO_SetPinOutHigh(LPC_GPIO, TFT_CS);
				xSemaphoreGive(Semaforo_SSP);


				if(ReceivePulsadores==1)		//Paso al estado HORAS DE TRABAJO
				{
					EstadoPantalla=1;
					FlagEstado=ON;

				}
				else if(ReceivePulsadores==10)	//Paso al estado CONFIRMAR SMS
				{
					EstadoPantalla=10;
					FlagEstado=ON;

					mensaje = 1; //Debe enviarse el mensaje de emergencia
				}

				else if(xSemaphoreTake(Semaforo_Flag10Seg,10 / portTICK_PERIOD_MS) == pdTRUE)
				{
					EstadoPantalla=0;
					FlagEstado=ON;
					xSemaphoreTake(Semaforo_Habilitacion10Seg, 10 / portTICK_PERIOD_MS);//Deshabilito el conteo
				}
			break;

			case 9:		//CONFIRMAR SALIDA
				if(FlagEstado==ON)
				{
					xQueuePeek(Cola_Datos_RFID, &informacionRFID, portMAX_DELAY);
					strcpy(nombre,informacionRFID.nombre);

					xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);
					GUI_Clear();
					FlagEstado=OFF;
					xSemaphoreGive(Semaforo_Reset10Seg);
					xSemaphoreGive(Semaforo_Habilitacion10Seg);
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt(nombre,100,10);
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispDecAt(hora,240,10,2);
					GUI_DispStringAt(":",264,10);
					GUI_DispDecAt(minutos,271,10,2);

					GUI_DrawHLine(40,0,320);
					GUI_DrawHLine(45,0,320);

					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt(" PARA CONFIRMAR SALIDA",160,60);
					GUI_DispStringHCenterAt(" DE CONDUCTOR MANTENGA",160,90);
					GUI_DispStringHCenterAt(" PRESIONADOS LOS",160,120);
					GUI_DispStringHCenterAt(" DOS PULSADORES...",160,150);

					GUI_DrawHLine(195,0,320);
					GUI_DrawHLine(200,0,320);
					Chip_GPIO_SetPinOutHigh(LPC_GPIO, TFT_CS);
					xSemaphoreGive(Semaforo_SSP);
				}
				if(ReceivePulsadores==11)		//Se presionaron ambos pulsadores
				{
					Leds = VERDE_TITILANDO;
					xQueueSendToBack(Cola_Leds, &Leds, portMAX_DELAY);

					EstadoPantalla=11;
					FlagEstado=ON;
					xSemaphoreTake(Semaforo_Sist_Inic, portMAX_DELAY);//Indico para que la SD no guarde datos mientras no hay RFID
				}

				else if(xSemaphoreTake(Semaforo_Flag10Seg,10 / portTICK_PERIOD_MS) == pdTRUE)
				{
					EstadoPantalla=0;
					FlagEstado=ON;
					xSemaphoreTake(Semaforo_Habilitacion10Seg, 10 / portTICK_PERIOD_MS);//Deshabilito el conteo
				}

			break;

			case 10:	//CONFIRMAR SMS
				if(FlagEstado==ON)
				{
					xQueuePeek(Cola_Datos_RFID, &informacionRFID, portMAX_DELAY);
					strcpy(nombre,informacionRFID.nombre);

					xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);
					GUI_Clear();
					FlagEstado=OFF;
					xSemaphoreGive(Semaforo_Reset10Seg);
					xSemaphoreGive(Semaforo_Habilitacion10Seg);
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt(nombre,100,10);
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispDecAt(hora,240,10,2);
					GUI_DispStringAt(":",264,10);
					GUI_DispDecAt(minutos,271,10,2);

					GUI_DrawHLine(40,0,320);
					GUI_DrawHLine(45,0,320);

					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt(" SE ENVIARA UN SMS A LA",160,60);
					GUI_DispStringHCenterAt(" CENTRAL. PARA CONFIRMAR",160,90);
					GUI_DispStringHCenterAt(" MANTENGA APRETADO LOS",160,120);
					GUI_DispStringHCenterAt(" DOS PULSADORES...",160,150);

					GUI_DrawHLine(195,0,320);
					GUI_DrawHLine(200,0,320);
					Chip_GPIO_SetPinOutHigh(LPC_GPIO, TFT_CS);
					xSemaphoreGive(Semaforo_SSP);
				}
				if(ReceivePulsadores==11)		//Se presionaron ambos pulsadores
				{

					EstadoPantalla=6;
					FlagEstado=ON;

				}
				else if(xSemaphoreTake(Semaforo_Flag10Seg,10 / portTICK_PERIOD_MS) == pdTRUE)
				{
					EstadoPantalla=0;
					FlagEstado=ON;
					xSemaphoreTake(Semaforo_Habilitacion10Seg, 10 / portTICK_PERIOD_MS);//Deshabilito el conteo
					mensaje=0;
				}

			break;

			case 11:	//DESLOGUEO CONFIRMADO

				xSemaphoreTake(Semaforo_SSP, portMAX_DELAY);
				GUI_Clear();
				GUI_SetFont(GUI_FONT_24B_ASCII);
				GUI_DispDecAt(hora,240,10,2);
				GUI_DispStringAt(":",264,10);
				GUI_DispDecAt(minutos,271,10,2);

				GUI_DrawHLine(40,0,320);
				GUI_DrawHLine(45,0,320);

				GUI_SetFont(GUI_FONT_24B_ASCII);
				GUI_DispStringHCenterAt("SE DESLOGUEO",160,100);
				GUI_DispStringHCenterAt("CORRECTAMENTE",160,130);
				GUI_DrawHLine(195,0,320);
				GUI_DrawHLine(200,0,320);
				Chip_GPIO_SetPinOutHigh(LPC_GPIO, TFT_CS);
				xSemaphoreGive(Semaforo_SSP);

				xQueueReset(Cola_Datos_RFID);
				xSemaphoreGive(Semaforo_YaHayTarj);

				Chip_GPIO_SetPinOutHigh(LPC_GPIO, BUZZER);
				vTaskDelay(1000/portTICK_PERIOD_MS);
				Chip_GPIO_SetPinOutLow(LPC_GPIO, BUZZER);

				vTaskDelay(1000/portTICK_RATE_MS);

				EstadoPantalla=7;
				minConductor=0;
				horaConductor=0;
				FlagEstado=ON;
			break;
		}

		if(minAnt!=minutos)
		{
			minAnt=minutos;
			minConductor++;
			if(minConductor==60)
			{
				minConductor=0;
				horaConductor++;
			}
		}
	}
	vTaskDelete(NULL);	//Borra la tarea si sale del while
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* xTaskMuestras
 * Tarea encargada de recolectar los datos del acelerometro
 */
static void xTaskMuestras(void *pvParameters)
{

	signed short int samples[7] = {0,0,0,0,0,0,0}; //cada posicion es de 16 bits, necesario para guardar
										   //la parte low y high de las muestras de accel
	static signed int promX = 0,promY = 0,promZ = 0;
	static signed short int  valxPrevio=0,valx;
	static uint8_t k;
	uint8_t rbuf[20] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	static uint8_t wbuf[2] = {0,0};
	static	I2C_XFER_T xfer;

	MPU6050_wakeup(&xfer);

	//Lectura de PWR_MGMMT_1 2 (para verificar si se lo saco del sleep y de standby a los ejes)
	wbuf[0] = MPU6050_RA_PWR_MGMT_1;
	I2C_XFER_config(&xfer, rbuf, 2, MPU6050_I2C_SLAVE_ADDRESS, 0, wbuf, 1);

	wbuf[0] = MPU6050_RA_ACCEL_XOUT_H;	//Configuracion de la 1era direccion desde la que se leeran los valores de los registros de los sensores

	while(1)
	{
		//xSemaphoreTake(Semaforo_Muestras_Acelerometro, portMAX_DELAY );

    	xSemaphoreTake(Semaforo_Sist_Inic, portMAX_DELAY);// me fijo que este inicializada el resto del sistema
    	xSemaphoreGive(Semaforo_Sist_Inic);

		for ( k = 0 ; k < 100 ; k ++ )
		{
			I2C_XFER_config(&xfer, rbuf, 14, MPU6050_I2C_SLAVE_ADDRESS, 0, wbuf, 1);

			Fill_Samples(samples, rbuf);

			valx = samples[0];

			if(valx != valxPrevio)
			{
				promX += samples[0];
				promY += samples[1];
				promZ += samples[2];
				valxPrevio = valx;
			}
			else
			{
				k--;//evito que sume al contador
			}
			vTaskDelay( 5 / 100 / configTICK_RATE_HZ );// delay de 500 useg
		}

		promX /= 100; promY /= 100; promZ /= 100;

		xQueueOverwrite(Cola_PromX,&promX);
		xQueueOverwrite(Cola_PromY,&promY);
		xQueueOverwrite(Cola_PromZ,&promZ);

		promX = promY = promZ = 0;

		xSemaphoreGive(Semaforo_Analisis_Acelerometro );
		vTaskDelay(5/portTICK_RATE_MS);//delay de 5ms
	}
	vTaskDelete(NULL);	//Borra la tarea
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* xTaskAcelerometro
 * Tarea encargada de procesar los datos recolectados del acelerometro
 */
static void xTaskAcelerometro(void *pvParameters)
{
	static	signed int 	deltaX,deltaY,deltaZ;
	static	signed int 	promX,promY,promZ;
	static	uint16_t	difX,difY,difZ;
	static signed int promXant = XDefecto, promYant = YDefecto, promZant = ZDefecto;

	while(1)
	{
		//xSemaphoreTake(Semaforo_Analisis_Acelerometro , portMAX_DELAY );
		xQueueReceive(Cola_PromX,&promX,portMAX_DELAY);
		xQueueReceive(Cola_PromY,&promY,portMAX_DELAY);
		xQueueReceive(Cola_PromZ,&promZ,portMAX_DELAY);

		//Calculo para ver si hay choque
		if(promX > promXant)
		{
			deltaX = promX - promXant;
		}
		else
		{
			deltaX = promXant - promX;
		}

		if(promY > promYant)
		{
			deltaY = promY - promYant;
		}
		else
		{
			deltaY = promYant - promY;
		}

		if(promZ > promZant)
		{
			deltaZ = promZ - promZant;
		}
		else
		{
			deltaZ = promZant - promZ;
		}

		//Calculo para ver si hay vuelco
		if(promX > XDefecto)
		{
			difX = promX - XDefecto;
		}
		else
		{
			difX = XDefecto - promX;
		}
		if(promY > YDefecto)
		{
			difY = promY - YDefecto;
		}
		else
		{
			difY = YDefecto - promY;
		}
		if(promZ > ZDefecto)
		{
			difZ = promZ - ZDefecto;
		}
		else
		{
			difZ = ZDefecto - promZ;
		}

		if ( ( deltaX > CHOQUE ) || ( deltaY > CHOQUE ) || ( deltaZ > CHOQUE ) ) //para un choque
		{
			xSemaphoreGive(Semaforo_Choque);
			vTaskDelay(10000/portTICK_RATE_MS);//delay de 10seg
		}
		if ( ( difX > VUELCO ) || ( difY > VUELCO ) || ( difZ > VUELCO ) ) //para un vuelco
		{
			xSemaphoreGive(Semaforo_Vuelco);
			vTaskDelay(10000/portTICK_RATE_MS);//delay de 10seg
		}

		promXant = promX; promYant = promY; promZant = promZ;

		xSemaphoreGive(Semaforo_Muestras_Acelerometro);

		vTaskDelay(50/portTICK_RATE_MS);//delay de 10ms

	}
	vTaskDelete(NULL);	//Borra la tarea
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* main */
int main (void)
{
	SystemCoreClockUpdate();

	/* Initializes GPIO */
	Chip_GPIO_Init(LPC_GPIO);

	uC_StartUp();

	vSemaphoreCreateBinary(Semaforo_RX1);				//Semaforo para indicar que hay algo para leer
	vSemaphoreCreateBinary(Semaforo_RX2);				//Semaforo para indicar que hay algo para leer
	vSemaphoreCreateBinary(Semaforo_RFID);				//Semaforo de inicializacion de rfid
	xSemaphoreTake(Semaforo_RFID, portMAX_DELAY);
	vSemaphoreCreateBinary(Semaforo_RTCgsm);			//Semaforo que permite guardar cada 30 seg
	xSemaphoreTake(Semaforo_RTCgsm, portMAX_DELAY);
	vSemaphoreCreateBinary(Semaforo_RTCsd);				//Semaforo que permite guardar cada 30 seg
	xSemaphoreTake(Semaforo_RTCsd, portMAX_DELAY);
	vSemaphoreCreateBinary(Semaforo_GSM_Closed);		//Semaforo para ver si recibi closed
	xSemaphoreTake(Semaforo_GSM_Closed, portMAX_DELAY);
	vSemaphoreCreateBinary(Semaforo_GSM_Recibido);		//Semaforo para asegurarse que se cargaron las tarjetas
	xSemaphoreTake(Semaforo_GSM_Recibido, portMAX_DELAY);
	vSemaphoreCreateBinary(Semaforo_SSP);				//Semaforo para la utilizacion del ssp
	vSemaphoreCreateBinary(Semaforo_YaHayTarj);			//Semaforo para indicar si ya hay colocada una tarjeta
	vSemaphoreCreateBinary(Semaforo_GSM_Enviar);		//Semaforo para indicar si se esta mandando algo por gsm
	vSemaphoreCreateBinary(Semaforo_Tarjeta_Incorrecta);//Semaforo para indicar que la tarjeta detectada es incorrecta
	xSemaphoreTake(Semaforo_Tarjeta_Incorrecta, portMAX_DELAY);
	vSemaphoreCreateBinary(Semaforo_SD);				//Semaforo de inicializacion de SD
	xSemaphoreTake(Semaforo_SD, portMAX_DELAY);
	vSemaphoreCreateBinary(Semaforo_Sist_Inic);			//Semaforo de inicializacion de sistema
	xSemaphoreTake(Semaforo_Sist_Inic, portMAX_DELAY);
	vSemaphoreCreateBinary(Semaforo_Flag10Seg);			//Semaforo para indicar 10 seg de la pantalla
	vSemaphoreCreateBinary(Semaforo_Habilitacion10Seg);			//Semaforo para habilitar los 10 seg de la pantalla
	vSemaphoreCreateBinary(Semaforo_Reset10Seg);			//Semaforo para resetear los 10 seg de la pantalla
	xSemaphoreTake(Semaforo_Flag10Seg, portMAX_DELAY);
	xSemaphoreTake(Semaforo_Habilitacion10Seg, portMAX_DELAY);
	xSemaphoreTake(Semaforo_Reset10Seg, portMAX_DELAY);
	vSemaphoreCreateBinary(Semaforo_Reset30Seg);			//Semaforo para resetear los 30 seg de la SD y GSM


	vSemaphoreCreateBinary(Semaforo_Muestras_Acelerometro);	//Semaforo para indicar que se analizaron las muestras
	vSemaphoreCreateBinary(Semaforo_Analisis_Acelerometro);	//Semaforo para indicar que se recogieron las muestras
	xSemaphoreTake(Semaforo_Analisis_Acelerometro , portMAX_DELAY );
	vSemaphoreCreateBinary(Semaforo_Vuelco);	//Semaforo para indicar que volco
	xSemaphoreTake(Semaforo_Vuelco , portMAX_DELAY );
	vSemaphoreCreateBinary(Semaforo_Choque);	//Semaforo para indicar que volco
	xSemaphoreTake(Semaforo_Choque , portMAX_DELAY );

	Cola_RX1 = xQueueCreate(UART_RRB_SIZE, sizeof(uint8_t));	//Creamos una cola
	Cola_TX1 = xQueueCreate(UART_SRB_SIZE, sizeof(uint8_t));	//Creamos una cola
	Cola_RX2 = xQueueCreate(UART_RRB_SIZE, sizeof(uint8_t));	//Creamos una cola
	Cola_TX2 = xQueueCreate(UART_SRB_SIZE, sizeof(uint8_t));	//Creamos una cola
	Cola_Pulsadores = xQueueCreate(4, sizeof(uint8_t));		//Creamos una cola
	Cola_Leds = xQueueCreate(1, sizeof(eLeds));		//Creamos una cola
	Cola_SD = xQueueCreate(4, sizeof(char) * 100);	//Creamos una cola para mandar una trama completa
	Cola_Datos_GPS = xQueueCreate(1, sizeof(struct Datos_Nube));
	Cola_Datos_RFID = xQueueCreate(1, sizeof(Tarjetas_RFID));
	HoraEntrada = xQueueCreate(1, sizeof(Entrada_RFID));
	Cola_Inicio_Tarjetas = xQueueCreate(1, sizeof(Tarjetas_RFID*));
	Cola_PromX = xQueueCreate(1, sizeof(signed int));
	Cola_PromY = xQueueCreate(1, sizeof(signed int));
	Cola_PromZ = xQueueCreate(1, sizeof(signed int));
	Cola_Inicio_Lector = xQueueCreate(1, sizeof(MFRC522Ptr_t));



	xTaskCreate(vTaskTFT, (char *) "vTaskTFT",
					( ( unsigned short ) 350), NULL, (tskIDLE_PRIORITY + 2UL),
							(xTaskHandle *) NULL);

	xTaskCreate(xTaskPulsadores, (char *) "xTaskPulsadores",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	xTaskCreate(xTaskLeds, (char *) "xTaskLeds",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
					(xTaskHandle *) NULL);

	xTaskCreate(xTaskWriteSD, (char *) "xTaskWriteSD",
					configMINIMAL_STACK_SIZE * 2, NULL, (tskIDLE_PRIORITY + 2UL),
					(xTaskHandle *) NULL);

	xTaskCreate(vTaskInicSD, (char *) "vTaskInicSD",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 3UL),
				(xTaskHandle *) NULL);


	xTaskCreate(vTaskRFID, (char *) "vTaskRFID",
			configMINIMAL_STACK_SIZE*2, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	xTaskCreate(xTaskRFIDConfig, (char *) "xTaskRFIDConfig",
			configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 4UL),
				(xTaskHandle *) NULL);

	xTaskCreate(xTaskRTConfig, (char *) "xTaskRTConfig",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 3UL),
				(xTaskHandle *) NULL);

	xTaskCreate(vTaskLeerAnillo1, (char *) "vTaskLeerAnillo1",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	xTaskCreate(vTaskLeerAnillo2, (char *) "vTaskLeerAnillo2",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	xTaskCreate(vTaskCargarAnillo1, (char *) "vTaskCargarAnillo1",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);
	/*
	//NO SE DEBERIA USAR
	xTaskCreate(vTaskCargarAnillo2, (char *) "vTaskCargarAnillo2",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);
	*/
	xTaskCreate(xTaskUART1Config, (char *) "xTaskUART1Config",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 3UL),
				(xTaskHandle *) NULL);

	xTaskCreate(xTaskUART2Config, (char *) "xTaskUART2Config",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 3UL),
				(xTaskHandle *) NULL);

	xTaskCreate(vTaskGSMConfig, (char *) "vTaskGSMConfig",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 3UL),
				(xTaskHandle *) NULL);

	xTaskCreate(vTaskEnviarGSM, (char *) "vTaskEnviarGSM",
				configMINIMAL_STACK_SIZE*2, NULL, (tskIDLE_PRIORITY + 2UL),
				(xTaskHandle *) NULL);

	xTaskCreate(vTaskAnalizarGPS, (char *) "vTaskAnalizarGPS",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	xTaskCreate(vTaskTarjetasGSM, (char *) "vTaskTarjetasGSM",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 2UL),
				(xTaskHandle *) NULL);


	xTaskCreate(xTaskAcelerometro, (char *) "xTaskAcelerometro",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 2UL),
				(xTaskHandle *) NULL);

	xTaskCreate(xTaskMuestras, (char *) "xTaskMuestras",
					configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
					(xTaskHandle *) NULL);

	/* Start the scheduler */
	vTaskStartScheduler();

	/* Nunca debería arribar aquí */
    return 0;
}
