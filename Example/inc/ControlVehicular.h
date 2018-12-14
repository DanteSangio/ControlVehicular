/*
 * ControlVehicular.h
 *
 *  Created on: Oct 5, 2018
 *      Author: Fede
 */

#ifndef EXAMPLE_INC_CONTROLVEHICULAR_H_
#define EXAMPLE_INC_CONTROLVEHICULAR_H_

#include "stdint.h"

//Defines Generales
#define ON			((uint8_t) 1)
#define OFF			((uint8_t) 0)
#define PORT(x) 	((uint8_t) x)
#define PIN(x)		((uint8_t) x)
#define OUTPUT		((uint8_t) 1)
#define INPUT		((uint8_t) 0)
#define DEBUGOUT(...) //printf(__VA_ARGS__)

//Defines Pines
#define LED_STICK	PORT(0),PIN(22)
#define	LED_VERDE	PORT(2),PIN(8)
#define	LED_ROJO	PORT(2),PIN(7)

#define	BUZZER		PORT(2),PIN(6)

#define	SW1			PORT(2),PIN(10)
#define SW2			PORT(2),PIN(11)
#define	SW3			PORT(2),PIN(12)

#define GPS_TX		PORT(0),PIN(10)
#define GPS_RX		PORT(0),PIN(11)

#define GSM_TX		PORT(0),PIN(15)
#define GSM_RX		PORT(0),PIN(16)
#define GSM_RST		PORT(2),PIN(13)

#define TFT_MOSI	PORT(0),PIN(9)
#define TFT_MISO	PORT(0),PIN(8)
#define TFT_SCK		PORT(0),PIN(7)
#define TFT_CS		PORT(0),PIN(6)
#define TFT_RESET	PORT(0),PIN(21)
#define TFT_DC		PORT(0),PIN(22)

#define AC_SDA		PORT(0),PIN(0)
#define AC_SCL		PORT(0),PIN(1)

#define RFID_MOSI	TFT_MOSI
#define RFID_MISO	TFT_MISO
#define RFID_SCK	TFT_SCK
#define RFID_SS		PORT(0),PIN(18)
#define RFID_IRQ	PORT(0),PIN(2)
#define RFID_RST	PORT(0),PIN(3)

#define SD_CS		PORT(0),PIN(27)

#define F_CS		PORT(0),PIN(28)

struct Datos_Nube
{
	char  	latitud[11],longitud[11];
	char  	hora[6],fecha[9];
};

typedef struct
{
	char  	tarjeta[11];
	char	nombre [20];
}Tarjetas_RFID;

#endif /* EXAMPLE_INC_CONTROLVEHICULAR_H_ */

