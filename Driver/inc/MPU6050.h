#ifndef __MPU6050_LIB_H
#define __MPU6050_LIB_H

#define MPU6050_ADDRESS             0x068
#define USER_CTRL                   0x06A
#define PWR_MGMT_1                  0x06B       

#define SMPLRT_DIV                  0x019     
#define FIFO_EN                     0x023
#define FIFO_COUNTH                 0x072
#define FIFO_COUNTL                 0x073
#define FIFO_R_W                    0x074
#define WHO_AM_I                    0x075

#define INT_PIN_CFG                 0x037      
#define INT_ENABLE                  0x038  
#define INT_STATUS                  0x03A       


#define CONFIG                      0x01A       
#define GYRO_CONFIG                 0x01B      
#define ACCEL_CONFIG                0x01C

#define ACCEL_XOUT_H                0x03B
#define ACCEL_XOUT_L                0x03C
#define ACCEL_YOUT_H                0x03D
#define ACCEL_YOUT_L                0x03E
#define ACCEL_ZOUT_H                0x03F
#define ACCEL_ZOUT_L                0x040

#define TEMP_OUT_H                  0x041
#define TEMP_OUT_L                  0x042

#define GYRO_XOUT_H                 0x043
#define GYRO_XOUT_L                 0x044
#define GYRO_YOUT_H                 0x045
#define GYRO_YOUT_L                 0x046
#define GYRO_ZOUT_H                 0x047
#define GYRO_ZOUT_L                 0x048

#define DATAFRAME_LEN               14		// size of a single frame of sensor data

enum LIST {
	ACCELX_H = 0,
	ACCELX_L,
	ACCELY_H,
	ACCELY_L,
	ACCELZ_H,
	ACCELZ_L,
	TEMP_H,
	TEMP_L,
	GYROX_H ,
	GYROX_L,
	GYROY_H,
	GYROY_L,
	GYROZ_H,
	GYROZ_L
};


#endif