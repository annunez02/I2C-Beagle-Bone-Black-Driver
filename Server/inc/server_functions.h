#ifndef __server_functions_LIB_H
#define __server_functions_LIB_H

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

#include "sensor_functions.h"

#define BLACK   "\x1B[0;30m"
#define RED     "\x1B[0;31m"
#define GREEN   "\x1B[0;32m"
#define YELLOW  "\x1B[0;33m"
#define BLUE    "\x1B[0;34m"
#define PURPLE  "\x1B[0;35m"
#define CYAN    "\x1B[0;36m"
#define WHITE   "\x1B[0;37m"
#define DEFAULT "\x1B[0m"


#define TAM 256
#define BUFFER_SIZE 2048

void sigchld_handler (int);
void sigint_handler(int);
void sigusr2_handler(int);

void * connection_handler(void *);
void saRestart (void);
void read_file(void);

#define KEY_PATH    "/home/debian/TD3/Server/file.txt"
#define MAX_CLIENTS 3
#define SENSOR_PATH "/home/debian/TD3/Sensor/sensor; exec bash " // ; exit para que se cierre sola la terminal cuando termine
#define SEM_KEY     "semaphore_key0"

typedef struct thread_info
{
    pthread_t   thread_id;
    int         accept_sfd;     // file descriptor
    int         active;
    sem_t       *semID_th;
    outData     *shmDATA_th;
} thread_info;

#endif 