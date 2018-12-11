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
#include "ControlVehicular.h"
#include "GPS.h"
#include "UART.h"
#include "sdcard.h"


/*****************************************************************************
 * Types/enumerations/variables
 ****************************************************************************/


extern SemaphoreHandle_t Semaforo_RX2;
extern QueueHandle_t Cola_RX2;
extern QueueHandle_t Cola_TX2;
extern QueueHandle_t Cola_Datos_GPS;

extern RINGBUFF_T txring, rxring;								//Transmit and receive ring buffers
extern uint8_t rxbuff[UART_RRB_SIZE], txbuff[UART_SRB_SIZE];	//Transmit and receive buffers


volatile int HourGPS, MinuteGPS, DayGPS, MonthGPS, YearGPS;		//GPS: Variables que guardan informacion
volatile float LatGPS, LongGPS;									//GPS: Variables que guardan informacion
volatile float Lat1GPS, Lat2GPS, Long1GPS, Long2GPS;			//GPS: Variables auxiliares



///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* uC_StartUp */
void uC_StartUp (void)
{
	//DEBUGOUT("Configurando pines I/O..\n");	//Imprimo en la consola

	Chip_GPIO_SetDir (LPC_GPIO, LED_STICK, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, LED_STICK, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetDir (LPC_GPIO, BUZZER, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, BUZZER, IOCON_MODE_INACT, IOCON_FUNC0);
	Chip_GPIO_SetDir (LPC_GPIO, SW1, INPUT);
	Chip_IOCON_PinMux (LPC_IOCON, SW1, IOCON_MODE_PULLDOWN, IOCON_FUNC0);
	Chip_GPIO_SetDir (LPC_GPIO, GSM_RST, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, GSM_RST, IOCON_MODE_INACT, IOCON_FUNC0);


	//Set the chipSelectPin as digital output, do not select the slave yet
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, SDCS);
	Chip_GPIO_SetPinOutHigh(LPC_GPIO, SDCS);

	//Salidas apagadas
	Chip_GPIO_SetPinOutLow(LPC_GPIO, LED_STICK);
	Chip_GPIO_SetPinOutLow(LPC_GPIO, BUZZER);
	Chip_GPIO_SetPinOutLow(LPC_GPIO, GSM_RST);

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* funcion para analizar la trama GPS */
//Analiza la trama GPS y guarda los datos deseados en las variables globales de GPS
void AnalizarTramaGPS (uint8_t dato)
{
	static int i=0,j;
	static int EstadoTrama=0;
	static char Trama[100];
	static char HourMinute[4];
	static char Date[6];
	static char Lat1[4];
	static char Lat2[5];
	static char Long1[4];
	static char Long2[5];
	char aux[11];
	char aux2[11];
	char aux3[11];
	static struct Datos_Nube valor;

	//int Aux;
	//float Aux1;
	//float Aux2;
	bool flagFecha=OFF;

	if(dato=='$')		//Inicio de la trama
	{
		EstadoTrama=0;
	}
	switch(EstadoTrama)
	{
		case INICIO_TRAMA:		//INICIO_TRAMA
			EstadoTrama=1;
			memset(Trama,0,100);
			i=0;
		break;

		case CHEQUEO_FIN:		//CHEQUEO_FIN
			Trama[i]=dato;
			i++;
			if(dato=='*')
			{
				EstadoTrama=2;
			}
		break;

		case CHEQUEO_TRAMA:		//CHEQUEO_TRAMA
			if(Trama[16]=='A' && Trama[29]=='S' && Trama[43]=='W')
			{
				EstadoTrama=3;	//Trama correcta
			}
		break;

		case TRAMA_CORRECTA:		//TRAMA_CORRECTA
			//DEBUGOUT("%s",Trama);
			//DEBUGOUT("\n");
			EstadoTrama=4;
		break;

		case HORA_FECHA:		//HORA Y FECHA
			//Hora
			for(i=6;i<=9;i++)
			{
				HourMinute[i-6]=Trama[i];
			}
			HourMinute[4]='\0';
			HourGPS=atoi(HourMinute);
			MinuteGPS=HourGPS%100;
			HourGPS=HourGPS/100;
			if(HourGPS>=0 && HourGPS<=2)
			{
				HourGPS=HourGPS+21;
				flagFecha=ON;		//flag correccion de fecha
			}
			else
			{
				HourGPS=HourGPS-3;
				flagFecha=OFF;
			}
			itoa(HourGPS,aux,10);
			itoa(MinuteGPS,aux2,10);
			strcat(aux,":");
			strcat(aux,aux2);
			for(j=0;j<6;j++)
			{
				valor.hora[j]=aux[j];
			}
			//DEBUGOUT("%.2d:%.2d\t",HourGPS,MinuteGPS);

			//Fecha
			for(i=52;i<=57;i++)
			{
				Date[i-52]=Trama[i];
			}
			Date[6]='\0';
			if(flagFecha==ON)
			{
				flagFecha=OFF;
				DayGPS=atoi(Date-1);
			}
			else
			{
				DayGPS=atoi(Date);
			}
			YearGPS=DayGPS%100;
			MonthGPS=(DayGPS/100)%100;
			DayGPS=DayGPS/10000;
			itoa(DayGPS,aux,10);
			itoa(MonthGPS,aux2,10);
			itoa(YearGPS,aux3,10);
			strcat(aux,"/");
			strcat(aux,aux2);
			strcat(aux,"/");
			strcat(aux,aux3);
			for(j=0;j<9;j++)
			{
				valor.fecha[j]=aux[j];
			}
			//DEBUGOUT("%.2d/%.2d/%.2d\n",DayGPS,MonthGPS,YearGPS);
			EstadoTrama=5;
		break;

		case LAT_LONG: 	//LATITUD Y LONGITUD -> DD = d + (min/60) + (sec/3600)
			//Latitud
			for(i=18;i<=21;i++)
			{
				Lat1[i-18]=Trama[i];
			}
			Lat1[4]='\0';
			for(i=23;i<=27;i++)
			{
				Lat2[i-23]=Trama[i];
			}
			Lat2[5]='\0';
			aux[0]='-';
			aux[1]=Lat1[0];
			aux[2]=Lat1[1];
			aux[3]='.';
			aux[4]=Lat1[2];
			aux[5]=Lat1[3];
			aux[6]=Lat2[0];
			aux[7]=Lat2[1];
			aux[8]=Lat2[2];
			aux[9]=Lat2[3];
			aux[10]='\0';
			for(j=0;j<10;j++)
			{
				valor.latitud[j]=aux[j];
			}

			/*
			Aux=atoi(Lat1);
			Lat1GPS=Aux/10000000;
			Lat2GPS=(Aux/100000)%100;
			Aux1=(Aux%100000);
			Aux2=(Aux1/100000);
			Lat2GPS=Lat2GPS+Aux2;
			Lat1GPS=Lat1GPS+(Lat2GPS/60);
			LatGPS=-Lat1GPS;
			*/
			//DEBUGOUT("%f\t",LatGPS);  	//-> Tira HardFault si descomento esta linea


			//Longitud
			for(i=32;i<=35;i++)
			{
				Long1[i-32]=Trama[i];
			}
			Long1[4]='\0';
			for(i=37;i<=41;i++)
			{
				Long2[i-37]=Trama[i];
			}
			Long2[5]='\0';
			aux[0]='-';
			aux[1]=Long1[0];
			aux[2]=Long1[1];
			aux[3]='.';
			aux[4]=Long1[2];
			aux[5]=Long1[3];
			aux[6]=Long2[0];
			aux[7]=Long2[1];
			aux[8]=Long2[2];
			aux[9]=Long2[3];
			aux[10]='\0';

			for(j=0;j<10;j++)
			{
				valor.longitud[j]=aux[j];
			}
			/*
			Aux=atoi(Long1);
			Long1GPS=Aux/10000000;
			Long2GPS=(Aux/100000)%100;
			Aux1=(Aux%100000);
			Aux2=(Aux1/100000);
			Long2GPS=Long2GPS+Aux2;
			Long1GPS=Long1GPS+(Long2GPS/60);
			LongGPS=-Long1GPS;
			*/
			//DEBUGOUT("%f\n",LongGPS);	//-> Tira HardFault si descomento esta linea

			xQueueOverwrite(Cola_Datos_GPS,&valor);

			EstadoTrama=6;
		break;

		case 6: 	//
		break;

		default:
		break;
	}
}
