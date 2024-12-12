#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define TEMP_ADD 0x48
#define CONST_CONV_TEMP 0.0625
#define CHAR_BIT 8	// Cantidad de bits por byte

struct ModuleData
{
	int16_t accel_outx;
	int16_t accel_outy;
	int16_t accel_outz;
	int16_t temp;
	int16_t gyro_outx;
	int16_t gyro_outy;
	int16_t gyro_outz;
};

struct ModuleData_Float
{
	float accel_outx;
	float accel_outy;
	float accel_outz;
	float temp;
	float gyro_outx;
	float gyro_outy;
	float gyro_outz;
};

int  main (void)
{
	int fd, i=0;
	uint8_t dato_a_leer[14];
	struct ModuleData module_data_raw;
	struct ModuleData_Float md_float;
	int cant=0;

	printf("[tester]-$ Open starting...\n");
	
	fd = open("/dev/td3_ani", O_RDWR);
	if (fd < 0)
	{
		printf("[tester]-$ No se pudo abrir td3_driver\n");
		return -1;
	}
	printf("[tester]-$ Open Operation Finished\n");

	sleep(1);
	printf("[tester]-$ Starting Read Operation\n");
	cant = read(fd, &dato_a_leer, 1);

	printf("[tester]-$ Cantidad de datos a leer: %d.\n", cant);
	if(cant < 0)
	{
		printf("[tester]-$ Error al leer.\n");
	 	close (fd);
	 	return 0;
	}

	for(i=0 ; i < 14; i++)
		printf("[tester]-$ dato_a_leer[%d] = 0x%X \n", i, dato_a_leer[i]);

	module_data_raw.accel_outx = (dato_a_leer[0] << 8) | dato_a_leer[1];
	module_data_raw.accel_outy = (dato_a_leer[2] << 8) | dato_a_leer[3];
	module_data_raw.accel_outz = (dato_a_leer[4] << 8) | dato_a_leer[5];
	module_data_raw.temp 	   = (dato_a_leer[6] << 8) | dato_a_leer[7];
	module_data_raw.gyro_outx  = (dato_a_leer[8] << 8) | dato_a_leer[9];
	module_data_raw.gyro_outy  = (dato_a_leer[10] << 8) | dato_a_leer[11];
	module_data_raw.gyro_outz  = (dato_a_leer[12] << 8) | dato_a_leer[13];

	printf("raw_accel_outx=%d\nraw_accel_outy=%d\nraw_accel_outz=%d\nraw_temp=%d\nraw_gyro_outx=%d\nraw_gyro_outy=%d\nraw_gyro_outz=%d\ncant bytes leidos = %d\n",
	module_data_raw.accel_outx,module_data_raw.accel_outy,module_data_raw.accel_outz,module_data_raw.temp,module_data_raw.gyro_outx,module_data_raw.gyro_outy,module_data_raw.gyro_outz, cant);

	md_float.accel_outx = module_data_raw.accel_outx / 16384.0;
	md_float.accel_outy = module_data_raw.accel_outy / 16384.0;
	md_float.accel_outz = module_data_raw.accel_outz / 16384.0;
	md_float.gyro_outx  = module_data_raw.gyro_outx / 131.0;
	md_float.gyro_outy  = module_data_raw.gyro_outy / 131.0;
	md_float.gyro_outz  = module_data_raw.gyro_outz / 131.0;
	md_float.temp 		= module_data_raw.temp/340 + 36.5;
	printf("accel_outx=%f\naccel_outy=%f\naccel_outz=%f\ntemp=%f\ngyro_outx=%f\ngyro_outy=%f\ngyro_outz=%f\ncant bytes leidos = %d\n",
	md_float.accel_outx,md_float.accel_outy,md_float.accel_outz,md_float.temp,md_float.gyro_outx,md_float.gyro_outy,md_float.gyro_outz, cant);

	close(fd);
	printf("[tester]-$ Close Operation Finished\n");

	return 0;
}
