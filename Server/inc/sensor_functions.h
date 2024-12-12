#ifndef __my_server_functions_LIB_H
#define __my_server_functions_LIB_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h> 
#include <signal.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <semaphore.h>



#define KEY_PATH        "/home/debian/TD3/Server/file.txt"
#define SEM_KEY         "semaphore_key0"

#define DATAFRAME_LEN   14
#define MAX_READING     72



typedef struct rawData
{
    int16_t raw_ac_x;
    int16_t raw_ac_y;
    int16_t raw_ac_z;
    int16_t raw_temp;
    int16_t raw_gy_x;
    int16_t raw_gy_y;
    int16_t raw_gy_z;
} rawData;

typedef struct outData
{
    float out_ac_x;
    float out_ac_y;
    float out_ac_z;
    float out_temp;
    float out_gy_x;
    float out_gy_y;
    float out_gy_z;

    float SMA_out_ac_x;
    float SMA_out_ac_y;
    float SMA_out_ac_z;
    float SMA_out_temp;
    float SMA_out_gy_x;
    float SMA_out_gy_y;
    float SMA_out_gy_z;

    float EWMA_out_ac_x;
    float EWMA_out_ac_y;
    float EWMA_out_ac_z;
    float EWMA_out_temp;
    float EWMA_out_gy_x;
    float EWMA_out_gy_y;
    float EWMA_out_gy_z;

} outData;

typedef struct dataBuffer
{
    outData * data;     // puntero a la shared memory
    //uint32_t inIdx;     // indice de entrada
} dataBuffer;

void cargarSharedMem(dataBuffer *);
void sensor_main(int);

//SMA EWMA

#endif 