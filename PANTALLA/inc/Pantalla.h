
#ifndef PANTALLA_H_
#define PANTALLA_H_

#include "chip.h"
#include "FreeRTOS.h"
#include "semphr.h"
//#include "genmacro.h"

//#include "tarea1.h"
#include "GUI.h"
#include "DIALOG.h"
#include "GRAPH.h"
#include "LCD_X_SPI.h"

extern QueueHandle_t xcolalcd;
extern float S1[300],S;
extern uint8_t ultimo_estado;
extern uint32_t S1i;
void vTaskPantalla(void *);


#define OPLCD0	0 //BOTON A
#define OPLCD1	1 //BOTON B
#define OPLCD2	2 //BOTON C
#define OPLCD3	3 //BOTON D
#define OPLCD4	4 //poner SLOW
#define OPLCD5	5//poner FAST
#define OPLCD6	6//poner A
#define OPLCD7	7//poner C
#define OPLCD8	8//poner GUARDADO EN MEMORIA
#define OPLCD9	9//poner GUARdADO EN PC
#define OPLCD10	10 // APAGAR DISPLAY
#define OPLCD11	11  // GRAFICAR MENU PC
#define OPLCD12	12  //GRaficar menu MEDICION
#define OPLCD13	13 //Graficar menu GrAFICO
#define OPLCD14	14 //poner memoria BoRRADA

#define OPLCD15 15 //Grafica menu hora
#define OPLCD16 16 //Hora Boton Izq
#define OPLCD17 17 //hora Boton der

#define OPLCD18 18 //Grafica menu de fecha
#define OPLCD19 19 //fecha boton izq
#define OPLCD20 20 //fecha boton med
#define OPLCD21 21 //fecha boton der

#define OPLCD22 22 // (reservada (? ( se la asigne a SELEC_E para no correr todas... con las macros se puede arreglar

#define OPLCD23 23 // Actualiza pantalla de Hora
#define OPLCD24 24 // Actualiza pantalla de Fecha

#define OPLCD25 25 // envia nueva medicion








#endif /* PANTALLA_H_ */
