/*
 * Acelerometro.c
 *
 *  Created on: Jan 28, 2019
 *      Author: kevin
 */

#include "chip.h"
#include "Acelerometro.h"


/*
 * Configura la estructura XFER para realizar la comunicacion i2c.
 * * xfer	  : Puntero a la estructura del tipo I2C_XFER_T necesaria para utilizar la funcion Chip_I2C_MasterTransfer.
 * 				Chip_I2C_MasterTransfer : Funcion que resuelve la interaccion i2c en funcion de lo especificado en la estructura I2C_XFER_T
 *	 rbuf 	  : Puntero al buffer de lectura donde se volcaran los bytes leidos
 *	 rxSz 	  : Cantidad de bytes que se leeran y volcaran en rbuf
 *	 slaveAddr: Direccion estatica del slave con el que se desea comunicar
 *	 status   : Estado de la comunicacion, (estado inicial 0)
 *	 wbuf	  : Buffer de escritura donde se colocara tanto el registro que se desea escribir como el dato que desea ser escrito
 *	 			Ej de uso: wbuf[] = {reg_inicial, dato} solo escribe el byte dato en reg_inicial
 *	 					   wbuf[] = {reg_inicial, dato1, dato2} escribe el byte dato1 en reg_incial y dato2 en reg_inicial+1 (el registro siguiente)
 *	 txSz	  : La cantidad de bytes que se desean enviar, osea empezando a leer wbuf desde 0 inclusive, cuantos bytes manda de ese buffer
 *	 			Ej : wbuf[] = {reg_inicial, dato1, dato2}, entonces txSz deberia ser = 3
 *	 				 wbuf[] = {reg_inicial}, (caso tipico de solo lectura de ese registro), entonces txSz deberia ser = 1
 */
void I2C_XFER_config (I2C_XFER_T * xfer,uint8_t *rbuf, int rxSz, uint8_t slaveAddr, I2C_STATUS_T status, uint8_t * wbuf, int txSz)
{
	xfer->rxBuff = rbuf; //Buffer de lectura
	xfer->rxSz = rxSz;	//cantidad de bytes que se desean leer, arbitrariamente seteamos 10
	xfer->slaveAddr = slaveAddr; //Adress estatica del dispositivo i2c a leer (MPU6050)
	xfer->status = status;
	xfer->txBuff = wbuf; //Buffer de escritura
	xfer->txSz = txSz; //cantidad de bytes que se desean escribir, solo escribimos el registro desde
					//el que comenzamos a leer
	Chip_I2C_MasterTransfer(I2C1, xfer);
}


/* Llena el vector de muestras samples con la data de los registros de ACCEL, GYRO y TEMP del MPU
 * Al estar la informacion en 16 bits y ser levantada por registros de 8, en rbuf esta la parte low
 * 	y high de cada componente, por lo que se debe recomponer desplazando la parte high y concatenando la low]
 * 	para obtener el valor que se midio. Proceso que se realiza en esta funcion para pasar a samples.
 *
 * 	 rbuf : Tiene la data leida por el MPU con la data dividida en high y low
 * 	 samples : Tendra la data agrupada que representa al valor medido por los sensores en cada eje
 */
void Fill_Samples(signed short int * samples, uint8_t * rbuf)
{
	//De momento leer rbuf es lo mismo que leer xfer.rxBuff porque apuntan a la misma direccion
	//Desplazamos la parte alta a los 8 ultimos bits y le hacemos un OR para tener en los 8 primeros la parte baja


	samples[0]=(rbuf[0] << 8) | rbuf[1];
	samples[1]=(rbuf[2] << 8) | rbuf[3];
	samples[2]=(rbuf[4] << 8) | rbuf[5];
	samples[3]=(rbuf[6] << 8) | rbuf[7];
	samples[4]=(rbuf[8] << 8) | rbuf[9];
	samples[5]=(rbuf[10] << 8) | rbuf[11];
	samples[6]=(rbuf[12] << 8) | rbuf[13];
}


/*
 * Se encarga de inicializar los registros de PWR_MGMT necesarios para habilitar los
 *  sensores (acelerometro y giroscopo) en cada eje, para sus lecturas.
 *  La configuracion en la que quedan seteados es la por defecto:
 *  accelerometer (±2g) , gyroscope (±250°/sec).
 *
 *  * xfer : Puntero a la estructura del tipo I2C_XFER_T necesaria para la utilizacion de Chip_I2C_MasterTransfer.
 *  		 Chip_I2C_MasterTransfer : Funcion que resuelve la interaccion i2c en funcion de lo especificado en la estructura I2C_XFER_T
 */
void MPU6050_wakeup(I2C_XFER_T * xfer)
{

		//Setea PWR_MGMT_1 y 2 en 0, el byte de cada uno
		uint8_t wbuf[3] = {MPU6050_RA_PWR_MGMT_1, 0, 0};
		/*xfer->slaveAddr = MPU6050_DEVICE_ADDRESS;
		xfer->txBuff = wbuf;
		xfer->txSz = 3;
		xfer->rxSz = 0;*/
		I2C_XFER_config(xfer, xfer->rxBuff, 0, MPU6050_I2C_SLAVE_ADDRESS, 0, wbuf, 3);

		uint8_t wbuf2[2] = {0x1C, 0x10};//Sensibilidad de +-8G para el acelerometro
		/*xfer->slaveAddr = MPU6050_DEVICE_ADDRESS;
		xfer->txBuff = wbuf;
		xfer->txSz = 2;
		xfer->rxSz = 0;*/
		I2C_XFER_config(xfer, xfer->rxBuff, 0, MPU6050_I2C_SLAVE_ADDRESS, 0, wbuf2, 2);
}
