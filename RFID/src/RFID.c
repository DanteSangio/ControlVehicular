/**
 * Executed every time the card reader detects a user in
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

//extern int last_balance;
extern unsigned int last_user_ID;
extern MFRC522Ptr_t mfrcInstance;
extern QueueHandle_t Cola_Datos_RFID, Cola_Inicio_Tarjetas;
extern SemaphoreHandle_t Semaforo_Tarjeta_Incorrecta;

void userTapIn()
{
	//Tarjetas_RFID Tarj_actual;
	//uint32_t	ultTarjeta;


	// Convert the uid bytes to an integer, byte[0] is the MSB
	last_user_ID =
		(int)mfrcInstance->uid.uidByte[3] |
		(int)mfrcInstance->uid.uidByte[2] << 8 |
		(int)mfrcInstance->uid.uidByte[1] << 16 |
		(int)mfrcInstance->uid.uidByte[0] << 24;

	//DEBUGOUT("\nCard Read user ID: %u \n\r",last_user_ID);

	Comparar(last_user_ID);
	//ConvIntaChar(last_user_ID, Tarj_actual.tarjeta);

	//xQueueOverwrite(Cola_Datos_RFID, &Tarj_actual); // cargo en la cola la estructura actual


	// Read the user balance NO BORRAR SINO NO DETECTA CUANDO HAY TARJETA NUEVA
	//last_balance = readCardBalance(mfrcInstance);

}

void ConvIntaChar(uint32_t informacionRFID,char* auxRfid)
{
	uint32_t aux,dato;
	uint8_t	 i=0,j=0;
	dato = informacionRFID;
	char tarj[11];

	do
	{
		aux = dato % 10;
		itoa(aux,&tarj[i],10);
		dato = dato / 10;
		i++;
	}while(dato != 0);

	for(i=0,j=9;i<10;i++,j--)
	{
		auxRfid[j] = tarj[i];
	}
}

void Comparar(unsigned int tarjeta) //devuelvo la tarjeta que se esta utilizando
{
	//unsigned int base = 4266702969;
	Tarjetas_RFID* InicioTarjetas=NULL;//Puntero al inicio del vector de tarjetas
	uint8_t todocero, iguales;
	uint8_t j=0,k=0;
	char		tarjAcomparar[10];


	xQueuePeek(Cola_Inicio_Tarjetas,&InicioTarjetas,portMAX_DELAY);//leo el principio de las tarjetas
	ConvIntaChar(tarjeta,tarjAcomparar);

	for(k=0,todocero = FALSE,iguales = FALSE; todocero == FALSE && iguales == FALSE; k++)
	{
		for(j=0;j<10;j++)
		{
			if(InicioTarjetas[k].tarjeta[j] != tarjAcomparar[j])
				break;
		}
		if(j==10)
		{
			iguales = TRUE;
			xQueueOverwrite(Cola_Datos_RFID, &InicioTarjetas[k]); // cargo en la cola la estructura actual
		}

		for(j=0;j<10;j++)
		{
			if(InicioTarjetas[k].tarjeta[j] != 0)
				break;
		}

		if(j==10)
		{
			todocero = TRUE;
		}

	}


	if(iguales == TRUE)
	{
		DEBUGOUT("Tarjeta Registrada\n\r");
		Chip_GPIO_SetPinOutHigh(LPC_GPIO, BUZZER);
		vTaskDelay(500/portTICK_RATE_MS);	//Espero 1s
		Chip_GPIO_SetPinOutLow(LPC_GPIO, BUZZER);
	}
	else
	{
		//xQueueReset(Cola_Datos_RFID);//si se  ingreso una tarjeta no registrada no coloco ninguna
		DEBUGOUT("Tarjeta NO Registrada\n\r");
		Chip_GPIO_SetPinOutHigh(LPC_GPIO, BUZZER);
		vTaskDelay(100/portTICK_RATE_MS);	//Espero 1s
		Chip_GPIO_SetPinOutLow(LPC_GPIO, BUZZER);
		vTaskDelay(300/portTICK_RATE_MS);	//Espero 1s
		Chip_GPIO_SetPinOutHigh(LPC_GPIO, BUZZER);
		vTaskDelay(100/portTICK_RATE_MS);	//Espero 1s
		Chip_GPIO_SetPinOutLow(LPC_GPIO, BUZZER);
		vTaskDelay(300/portTICK_RATE_MS);	//Espero 1s
		Chip_GPIO_SetPinOutHigh(LPC_GPIO, BUZZER);
		vTaskDelay(100/portTICK_RATE_MS);	//Espero 1s
		Chip_GPIO_SetPinOutLow(LPC_GPIO, BUZZER);
		xSemaphoreGive(Semaforo_Tarjeta_Incorrecta);
	}

	return ;
}
