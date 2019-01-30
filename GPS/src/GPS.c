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
#include "RFID.h"

/*****************************************************************************
 * Types/enumerations/variables
 ****************************************************************************/


extern SemaphoreHandle_t Semaforo_RX2;
extern QueueHandle_t Cola_RX2;
extern QueueHandle_t Cola_TX2;
extern QueueHandle_t Cola_Datos_GPS;

extern RINGBUFF_T txring, rxring;								//Transmit and receive ring buffers
extern uint8_t rxbuff[UART_RRB_SIZE], txbuff[UART_SRB_SIZE];	//Transmit and receive buffers


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
	Chip_GPIO_SetDir (LPC_GPIO, SW2, INPUT);
	Chip_IOCON_PinMux (LPC_IOCON, SW2, IOCON_MODE_PULLDOWN, IOCON_FUNC0);
	Chip_GPIO_SetDir (LPC_GPIO, GSM_RST, OUTPUT);
	Chip_IOCON_PinMux (LPC_IOCON, GSM_RST, IOCON_MODE_INACT, IOCON_FUNC0);

	Chip_IOCON_PinMux(LPC_IOCON, AC_SDA, IOCON_MODE_INACT, IOCON_FUNC3);
	Chip_IOCON_PinMux(LPC_IOCON, AC_SCL, IOCON_MODE_INACT, IOCON_FUNC3);
	Chip_IOCON_EnableOD(LPC_IOCON, AC_SDA);
	Chip_IOCON_EnableOD(LPC_IOCON, AC_SCL);

    Chip_I2C_SetClockRate(I2C1, 100000);
	Chip_I2C_SetMasterEventHandler(I2C1, Chip_I2C_EventHandlerPolling);

	//Set the chipSelectPin as digital output, do not select the slave yet
	Chip_GPIO_SetPinDIROutput(LPC_GPIO, SD_CS);
	Chip_GPIO_SetPinOutHigh(LPC_GPIO, SD_CS);

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
	__DATA(RAM2)	static int i=0,j;
	__DATA(RAM2)	static int EstadoTrama=0;
	__DATA(RAM2)	static char Trama[100];
	__DATA(RAM2)	static char HourMinute[4];
	__DATA(RAM2)	static char Date[6];
	__DATA(RAM2)	static char Lat1[14];
	__DATA(RAM2)	static char Lat2[6];
	__DATA(RAM2)	static char Long1[14];
	__DATA(RAM2)	static char Long2[6];
	__DATA(RAM2)	static struct Datos_Nube valor;
	__DATA(RAM2)	static 	int HourGPS, MinuteGPS, DayGPS, MonthGPS, YearGPS;		//GPS: Variables que guardan informacion
	__DATA(RAM2)	static	int LatGPS, LongGPS;									//GPS: Variables que guardan informacion
	__DATA(RAM2)	static	int LatGrados, LatMinutos, LatSeg, LongGrados, LongMinutos, LongSeg;			//GPS: Variables auxiliares
	char aux[11];
	char aux2[11];
	char aux3[11];


	char	velocidad[10];
	int		velocidadNum;

	int Auxnum;
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

		case VELOCIDAD:		//TRAMA_CORRECTA
			//Obtengo los nudos * 1000 (sin el punto)
			velocidad[0]=Trama[45];
			velocidad[1]=Trama[47];
			velocidad[2]=Trama[48];
			velocidad[3]=Trama[49];
			velocidad[4]='\0';
			velocidadNum = atoi(velocidad);
			velocidadNum = velocidadNum * 1.852 / 1000;//Lo paso a kilometros por hora / 1000
			if(velocidadNum>=5) // solo muestro si es  mayor a 5km/h
			{
				valor.velocidad = velocidadNum;
			}
			else
			{
				valor.velocidad = 0;
			}
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

			//Fecha
			for(i=52;i<=57;i++)
			{
				Date[i-52]=Trama[i];
			}
			Date[6]='\0';
			DayGPS=atoi(Date);
			if(flagFecha==ON)//Le resto 1 dia si esta el flag encendido
			{
				flagFecha=OFF;
				DayGPS=DayGPS - 10000;
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

			strcat(Lat1,Lat2);
			Auxnum=atoi(Lat1);
			LatGrados=Auxnum/10000000;
			LatGPS=LatGrados*10000000;
			LatMinutos=(Auxnum/100000)%100;
			LatMinutos=LatMinutos*10000000/60;
			LatGPS=LatGPS+LatMinutos;
			LatSeg=(Auxnum%100000)*60;
			LatSeg=LatSeg*100/3600;
			LatGPS=LatGPS+LatSeg;

			ConvIntaChar(LatGPS, valor.latitud);
			valor.latitud[0]='-';
			valor.latitud[10]=valor.latitud[9];
			valor.latitud[9]=valor.latitud[8];
			valor.latitud[8]=valor.latitud[7];
			valor.latitud[7]=valor.latitud[6];
			valor.latitud[6]=valor.latitud[5];
			valor.latitud[5]=valor.latitud[4];
			valor.latitud[4]=valor.latitud[3];
			valor.latitud[3]='.';

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


			strcat(Long1,Long2);
			Auxnum=atoi(Long1);
			LongGrados=Auxnum/10000000;
			LongGPS=LongGrados*10000000;
			LongMinutos=(Auxnum/100000)%100;
			LongMinutos=LongMinutos*10000000/60;
			LongGPS=LongGPS+LongMinutos;
			LongSeg=(Auxnum%100000)*60;
			LongSeg=LongSeg*100/3600;
			LongGPS=LongGPS+LongSeg;


			ConvIntaChar(LongGPS, valor.longitud);
			valor.longitud[0]='-';
			valor.longitud[10]=valor.longitud[9];
			valor.longitud[9]=valor.longitud[8];
			valor.longitud[8]=valor.longitud[7];
			valor.longitud[7]=valor.longitud[6];
			valor.longitud[6]=valor.longitud[5];
			valor.longitud[5]=valor.longitud[4];
			valor.longitud[4]=valor.longitud[3];
			valor.longitud[3]='.';

			if(YearGPS>18 && YearGPS<30)
			{
				xQueueOverwrite(Cola_Datos_GPS,&valor);
			}
			EstadoTrama=6;
		break;

		case 6: 	//
		break;

		default:
		break;
	}
}
