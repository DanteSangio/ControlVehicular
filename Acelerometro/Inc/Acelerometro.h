/*
 * Acelerometro.h
 *
 *  Created on: Jan 28, 2019
 *      Author: kevin
 */

#ifndef ACELEROMETRO_INC_ACELEROMETRO_H_
#define ACELEROMETRO_INC_ACELEROMETRO_H_

#define MPU6050_DEVICE_ADDRESS   0x68
#define MPU6050_RA_ACCEL_XOUT_H  0x3B
#define MPU6050_RA_PWR_MGMT_1    0x6B
#define MPU6050_RA_PWR_MGMT_2    0x6C
#define MPU6050_PWR1_SLEEP_BIT   6
#define MPU6050_I2C_SLAVE_ADDRESS 0x68
#define STATIC_0x40_REFERENCE_REGISTER 0x6B
#define ACCEL_XOUT_H 0x3B
#define ACCEL_XOUT_L 0x3C

#define	CHOQUE		10000
#define VUELCO		1500
#define	XDefecto	4310
#define	YDefecto	-250
#define	ZDefecto	-510

void MPU6050_wakeup(I2C_XFER_T * xfer);
void Fill_Samples(signed short int * samples, uint8_t * rbuf);
void I2C_XFER_config (I2C_XFER_T * xfer,uint8_t *rbuf, int rxSz, uint8_t slaveAddr, I2C_STATUS_T status, uint8_t * wbuf, int txSz);

#endif /* ACELEROMETRO_INC_ACELEROMETRO_H_ */
