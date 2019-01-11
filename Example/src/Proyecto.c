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
#include <cr_section_macros.h>
#include "MFRC522.h"
#include "rfid_utils.h"
#include "string.h"
#include "stdlib.h"
#include "ControlVehicular.h"

#include "GPS.h"
#include "UART.h"
#include <RFID.h>
#include "GSM.h"

#include "GUI.h"
#include "DIALOG.h"
#include "GRAPH.h"
#include "LCD_X_SPI.h"


#include "Pantalla.h"

//#include "RegsLPC1769.h"
/*
#define DEBUGOUT1(...) printf(__VA_ARGS__)
#define DEBUGOUT(...) printf(__VA_ARGS__)
#define DEBUGSTR(...) printf(__VA_ARGS__)
*/
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
QueueHandle_t xcolalcd;
QueueHandle_t xcolateclado;


extern const GUI_BITMAP bmutnlogo; /* declare external Bitmap */
extern const GUI_BITMAP bmcontrol; /* declare external Bitmap */




///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* xTaskPulsadores */
static void xTaskPulsadores(void *pvParameters)
{
	static uint8_t Send=OFF;
	static uint8_t SendAnt=OFF;
	while (1)
	{
		Send=0;

		if(Chip_GPIO_GetPinState(LPC_GPIO, SW1)==OFF)	//Si se presiona el SW1
		{
			Send++;	//Envio a la cola que se presiono el SW1
		}
		if(Chip_GPIO_GetPinState(LPC_GPIO, SW2)==OFF)	//Si se presiona el SW2
		{
			Send=Send+10;
		}
		if(SendAnt==Send)
		{
			xQueueSendToBack(Cola_Pulsadores, &Send, portMAX_DELAY);
			while(Chip_GPIO_GetPinState(LPC_GPIO, SW2)==OFF || Chip_GPIO_GetPinState(LPC_GPIO, SW1)==OFF);
		}
		SendAnt=Send;
		vTaskDelay(100/portTICK_RATE_MS);	//Espero 1s
	}
	vTaskDelete(NULL);	//Borra la tarea
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* vTaskPantalla */
void vTaskPantalla(void *pvParameters)
{
	static uint8_t EstadoPantalla=0;
	static uint8_t FlagEstado=ON;
	uint8_t Receive=OFF;

	/*
	uint8_t op,menu,gananterior; //menu=EST_MEDICION
	static uint8_t PC_config=0; // empieza en el menu de medicion
	char cadena[16];

	GUI_COLOR colorfondoboton;
	BUTTON_Handle botona,botonb,botonc,botond,botone;
	GRAPH_Handle grafico;
	GRAPH_SCALE_Handle escalagrafh,escalagrafv;
	GRAPH_DATA_Handle datagraf;
	*/

	short int array[250];
	int i=250;

	while(i!=0)
	{
		i--;
		array[i]=0;
	}

	GUI_Init();

	GUI_Clear();
	GUI_DrawBitmap(&bmutnlogo, 0, 0);

	vTaskDelay(1000/portTICK_PERIOD_MS);

	GUI_Clear();
	GUI_DrawBitmap(&bmcontrol, 0, 0);

	vTaskDelay(1000/portTICK_PERIOD_MS);

	GUI_SetBkColor(0x00000000); //gris claro 0x00D3D3D3
	GUI_Clear();

	/*
	BUTTON_SetDefaultFont(GUI_FONT_COMIC18B_ASCII);
	BUTTON_SetDefaultTextAlign(GUI_TA_HCENTER | GUI_TA_VCENTER);
	GUI_SetFont(GUI_FONT_COMIC18B_ASCII);
	botona=BUTTON_CreateEx(5,145,150,40,0,WM_CF_SHOW,0,GUI_ID_BUTTON0);
	//BUTTON_SetTextAling(botona,GUI_TA_HCENTER | GUI_TA_VCENTER);
	BUTTON_SetText(botona, "FAST/SLOW");
	botonb=BUTTON_CreateEx(165,145,150,40,0,WM_CF_SHOW,0,GUI_ID_BUTTON1);
	BUTTON_SetText(botonb, "A/C");
	//GUI_SetTextAling(botona,GUI_TA_HCENTER | GUI_TA_VCENTER);
	//GUI_SetFont(botona,GUI_FONT_COMIC18B_ASCII);
	botonc=BUTTON_CreateEx(5,195,150,40,0,WM_CF_SHOW,0,GUI_ID_BUTTON2);
	BUTTON_SetText(botonc, "GRAFICO");
	botond=BUTTON_CreateEx(165,195,150,40,0,WM_CF_SHOW,0,GUI_ID_BUTTON3);
	BUTTON_SetText(botond, "OPCIONES");
	BUTTON_SetBkColor(botona,BUTTON_CI_UNPRESSED,0x00FFFF80);*/
	/*
	GUI_Exec();

	colorfondoboton=BUTTON_GetDefaultBkColor(BUTTON_CI_UNPRESSED);

	//borro pantalla
	//GUI_SetBkColor(0x0080FF80);//Verde
	//GUI_SetBkColor(0x00D3D3D3); //gris claro
	//GUI_Clear();

	grafico=GRAPH_CreateEx(0,5,310,180,0,WM_CF_HIDE,0,GUI_ID_GRAPH0);

	*/

	while(1)
	{
		Receive = 0;
		xQueueReceive(Cola_Pulsadores, &Receive, 100);

		switch(EstadoPantalla)
		{
			case 0:		//PANTALLA PRINCIPAL CON USUARIO REGISTRADO
				if(FlagEstado==ON)
				{
					FlagEstado=OFF;
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("10/01/2019",160,10);

					GUI_SetFont(GUI_FONT_D80);
					GUI_DispDecAt(23,32,50,2);
					GUI_DispDecAt(48,165,50,2);
					GUI_SetFont(GUI_FONT_32B_ASCII);
					GUI_DispStringAt(".",155,75);
					GUI_DispStringAt(".",155,50);

					GUI_SetFont(GUI_FONT_32B_ASCII);
					GUI_DispStringHCenterAt("JUAN FERNANDEZ",160,150);

					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("MENU",80,210);
					GUI_DispStringHCenterAt("S.O.S.",240,210);

					GUI_DrawHLine(195,0,320);
					GUI_DrawHLine(200,0,320);
				}
				if(Receive==1)		//Paso al estado HORAS DE TRABAJO
				{
					EstadoPantalla=1;
					FlagEstado=ON;
					GUI_Clear();
				}
				else if(Receive==10)	//Paso al estado CONFIRMAR SMS
				{
					EstadoPantalla=10;
					FlagEstado=ON;
					GUI_Clear();
				}
			break;

			case 1:		//HORAS DE TRABAJO
				if(FlagEstado==ON)
				{
					FlagEstado=OFF;
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("Juan Fernandez",100,10);
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispDecAt(23,240,10,2);
					GUI_DispStringAt(":",264,10);
					GUI_DispDecAt(48,271,10,2);

					GUI_DrawHLine(40,0,320);
					GUI_DrawHLine(45,0,320);

					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("HORAS DE TRABAJO",160,75);
					GUI_SetFont(GUI_FONT_32B_ASCII);
					GUI_DispStringHCenterAt(":",160,120);
					GUI_SetFont(GUI_FONT_D48);
					GUI_DispDecAt(6,80,115,2);
					GUI_DispDecAt(15,165,115,2);

					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("SIGUIENTE",80,210);

					GUI_DrawHLine(195,0,320);
					GUI_DrawHLine(200,0,320);
				}
				if(Receive==1)		//Paso al estado SMS CONTACTO
				{
					EstadoPantalla=2;
					FlagEstado=ON;
					GUI_Clear();
				}
				//Contar 10 seg, si no se presiona tecla -> EstadoPantalla=0;
			break;

			case 2:		//SMS CONTACTO
				if(FlagEstado==ON)
				{
					FlagEstado=OFF;
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("Juan Fernandez",100,10);
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispDecAt(23,240,10,2);
					GUI_DispStringAt(":",264,10);
					GUI_DispDecAt(48,271,10,2);

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
				}
				if(Receive==1)		//Paso al estado SALIDA CONDUCTOR
				{
					EstadoPantalla=5;
					FlagEstado=ON;
					GUI_Clear();
				}
				else if(Receive==10)	//Paso al estado CONFIRMAR SMS
				{
					EstadoPantalla=10;
					FlagEstado=ON;
					GUI_Clear();
				}
				//Contar 10 seg, si no se presiona tecla -> EstadoPantalla=0;
			break;

			case 3:		//
			break;

			case 4:		//
			break;

			case 5:		//SALIDA CONDUCTOR
				if(FlagEstado==ON)
				{
					FlagEstado=OFF;
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("Juan Fernandez",100,10);
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispDecAt(23,240,10,2);
					GUI_DispStringAt(":",264,10);
					GUI_DispDecAt(48,271,10,2);

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
				}
				if(Receive==1)		//Paso al estado PANTALLA PRINCIPAL
				{
					EstadoPantalla=1;
					FlagEstado=ON;
					GUI_Clear();
				}
				else if(Receive==10)		//Paso al estado CONFIRMAR SALIDA
				{
					EstadoPantalla=9;
					FlagEstado=ON;
					GUI_Clear();
				}
				//Contar 10 seg, si no se presiona tecla -> EstadoPantalla=0;
			break;

			case 6:		//ENVIAR MENSAJE
				GUI_SetFont(GUI_FONT_24B_ASCII);
				GUI_DispStringHCenterAt("Juan Fernandez",100,10);
				GUI_SetFont(GUI_FONT_24B_ASCII);
				GUI_DispDecAt(23,240,10,2);
				GUI_DispStringAt(":",264,10);
				GUI_DispDecAt(48,271,10,2);

				GUI_DrawHLine(40,0,320);
				GUI_DrawHLine(45,0,320);

				GUI_SetFont(GUI_FONT_24B_ASCII);
				GUI_DispStringHCenterAt("MENSAJE ENVIADO",160,110);

				GUI_DrawHLine(195,0,320);
				GUI_DrawHLine(200,0,320);

				Chip_GPIO_SetPinOutHigh(LPC_GPIO, BUZZER);
				vTaskDelay(1000/portTICK_PERIOD_MS);
				Chip_GPIO_SetPinOutLow(LPC_GPIO, BUZZER);

				vTaskDelay(2000/portTICK_PERIOD_MS);
				EstadoPantalla=0;
				GUI_Clear();
			break;

			case 7:		//PANTALLA PRINCIPAL SIN USUARIO REGISTRADO
				if(FlagEstado==ON)
				{
					FlagEstado=OFF;
					GUI_SetFont(GUI_FONT_32B_ASCII);
					GUI_DispStringHCenterAt("10/01/2019",160,10);

					GUI_SetFont(GUI_FONT_D48);
					GUI_DispDecAt(23,32,150,2);
					GUI_DispDecAt(48,165,150,2);
					GUI_SetFont(GUI_FONT_32B_ASCII);
					GUI_DispStringAt(".",155,135);
					GUI_DispStringAt(".",155,145);

					GUI_SetFont(GUI_FONT_32B_ASCII);
					GUI_DispStringHCenterAt("POR FAVOR",160,50);
					GUI_DispStringHCenterAt("ACERQUE SU TARJETA",160,80);

					GUI_DrawHLine(195,0,320);
					GUI_DrawHLine(200,0,320);
				}

				//Si acerca la tarjeta correcta cambia al estado 0

				//Por ahora para probar..
				vTaskDelay(2000/portTICK_PERIOD_MS);
				EstadoPantalla=0;
				FlagEstado=ON;
				GUI_Clear();
			break;

			case 9:		//CONFIRMAR SALIDA
				if(FlagEstado==ON)
				{
					FlagEstado=OFF;
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("Juan Fernandez",100,10);
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispDecAt(23,240,10,2);
					GUI_DispStringAt(":",264,10);
					GUI_DispDecAt(48,271,10,2);

					GUI_DrawHLine(40,0,320);
					GUI_DrawHLine(45,0,320);

					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt(" PARA CONFIRMAR LA SALIDA",160,60);
					GUI_DispStringHCenterAt(" DE CONDUCTOR MANTENGA",160,90);
					GUI_DispStringHCenterAt(" PRESIONADOS LOS",160,120);
					GUI_DispStringHCenterAt(" DOS PULSADORES...",160,150);

					GUI_DrawHLine(195,0,320);
					GUI_DrawHLine(200,0,320);
				}
				if(Receive==11)		//Se presionaron ambos pulsadores
				{
					EstadoPantalla=11; //FALTA SACAR LA TARJETA
					FlagEstado=ON;
					GUI_Clear();
				}
				//Contar 10 seg, si no se presiona tecla -> EstadoPantalla=0;
			break;

			case 10:	//CONFIRMAR SMS
				if(FlagEstado==ON)
				{
					FlagEstado=OFF;
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt("Juan Fernandez",100,10);
					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispDecAt(23,240,10,2);
					GUI_DispStringAt(":",264,10);
					GUI_DispDecAt(48,271,10,2);

					GUI_DrawHLine(40,0,320);
					GUI_DrawHLine(45,0,320);

					GUI_SetFont(GUI_FONT_24B_ASCII);
					GUI_DispStringHCenterAt(" SE ENVIARA UN SMS A LA",160,60);
					GUI_DispStringHCenterAt(" CENTRAL. PARA CONFIRMAR",160,90);
					GUI_DispStringHCenterAt(" MANTENGA APRETADO LOS",160,120);
					GUI_DispStringHCenterAt(" DOS PULSADORES...",160,150);

					GUI_DrawHLine(195,0,320);
					GUI_DrawHLine(200,0,320);
				}
				if(Receive==11)		//Se presionaron ambos pulsadores
				{
					EstadoPantalla=6;
					FlagEstado=ON;
					GUI_Clear();
				}
				//Contar 10 seg, si no se presiona tecla -> EstadoPantalla=0;
			break;

			case 11:

				GUI_SetFont(GUI_FONT_24B_ASCII);
				GUI_DispStringHCenterAt("Juan Fernandez",100,10);
				GUI_SetFont(GUI_FONT_24B_ASCII);
				GUI_DispDecAt(23,240,10,2);
				GUI_DispStringAt(":",264,10);
				GUI_DispDecAt(48,271,10,2);

				GUI_DrawHLine(40,0,320);
				GUI_DrawHLine(45,0,320);

				GUI_SetFont(GUI_FONT_24B_ASCII);
				GUI_DispStringHCenterAt("SE DESLOGUEO",160,100);
				GUI_DispStringHCenterAt("CORRECTAMENTE",160,130);
				GUI_DrawHLine(195,0,320);
				GUI_DrawHLine(200,0,320);

				Chip_GPIO_SetPinOutHigh(LPC_GPIO, BUZZER);
				vTaskDelay(1000/portTICK_PERIOD_MS);
				Chip_GPIO_SetPinOutLow(LPC_GPIO, BUZZER);

				vTaskDelay(1000);

				EstadoPantalla=0;
				FlagEstado=ON;
				GUI_Clear();

				break;
		}
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
/* main */
int main (void)
{
	SystemCoreClockUpdate();

	/* Initializes GPIO */
	Chip_GPIO_Init(LPC_GPIO);

	//uC_StartUp();

	Chip_GPIO_SetDir (LPC_GPIO, SW1, INPUT);
	Chip_IOCON_PinMux (LPC_IOCON, SW1, IOCON_MODE_PULLDOWN, IOCON_FUNC0);
	Chip_GPIO_SetDir (LPC_GPIO, SW2, INPUT);
	Chip_IOCON_PinMux (LPC_IOCON, SW2, IOCON_MODE_PULLDOWN, IOCON_FUNC0);


	Cola_Pulsadores = xQueueCreate(1, sizeof(uint32_t));	//Creamos una cola

	xTaskCreate(vTaskPantalla, (char *) "vTaskPantalla",
					( ( unsigned short ) 700), ( void * )xcolateclado, (tskIDLE_PRIORITY + 1UL),
							(xTaskHandle *) NULL);

	xTaskCreate(xTaskPulsadores, (char *) "vTaskPulsadores",
				configMINIMAL_STACK_SIZE, NULL, (tskIDLE_PRIORITY + 1UL),
				(xTaskHandle *) NULL);

	/* Start the scheduler */
	vTaskStartScheduler();

	/* Nunca debería arribar aquí */
    return 0;
}
