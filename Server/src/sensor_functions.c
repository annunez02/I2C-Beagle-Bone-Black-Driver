#include "../inc/sensor_functions.h"
#include "../inc/server_functions.h"
#include "../inc/filter.h"

extern int cant_th;
extern int close_socket;
extern int sigint_counter;
extern int max_clients;
extern int backlog;
extern outData * shmdata;
extern sem_t * semID;

rawData raw_data;
outData * out_data;

void cargarSharedMem(dataBuffer * shMem)
{

   int8_t rawDataBuff[DATAFRAME_LEN];
    
    int driver_fd = open("/dev/td3_ani", O_RDONLY);
    if (driver_fd == -1) 
    {
        perror(RED"[ERROR] LOADING SHMEM: Error al abrir /dev/td3_ani"DEFAULT);
        return;
    }

    ssize_t read_buffer = read(driver_fd, &rawDataBuff, 1);
    if (read_buffer == -1) 
    {
        perror(RED"[ERROR] LOADING SHMEM: Error al leer de /dev/td3_ani"DEFAULT);
        close(driver_fd);
        return;
    }

    /*
        Leo primero la parte alta (1 byte o 8 bits) y despues la baja
        Ver pag 7 de MPU REG MAP
    */
    

    raw_data.raw_ac_x   = (rawDataBuff[0] << 8)  | rawDataBuff[1];
    raw_data.raw_ac_y   = (rawDataBuff[2] << 8)  | rawDataBuff[3];
    raw_data.raw_ac_z   = (rawDataBuff[4] << 8)  | rawDataBuff[5];
    raw_data.raw_temp   = (rawDataBuff[6] << 8)  | rawDataBuff[7];
    raw_data.raw_gy_x   = (rawDataBuff[8] << 8)  | rawDataBuff[9];
    raw_data.raw_gy_y   = (rawDataBuff[10] << 8) | rawDataBuff[11];
    raw_data.raw_gy_z   = (rawDataBuff[12] << 8) | rawDataBuff[13];

    out_data->out_ac_x = raw_data.raw_ac_x / 16384.0;
    out_data->out_ac_y = raw_data.raw_ac_y / 16384.0;
    out_data->out_ac_z = raw_data.raw_ac_z / 16384.0;
    out_data->out_temp = raw_data.raw_temp / 340 + 36.5;
    out_data->out_gy_x = raw_data.raw_gy_x / 131.0;
    out_data->out_gy_y = raw_data.raw_gy_y / 131.0;
    out_data->out_gy_z = raw_data.raw_gy_z / 131.0;

    SMA_filter();
    EWMA_filter();

    //rawDataBuff %=100;
    //memcpy(&shMem->data[shMem->inIdx], out_data, sizeof(outData));
    memcpy(shMem->data, out_data, sizeof(outData));
/*
    printf(GREEN"____________________________________________________________\n"
                "Temp sin filtrar escrita en la SH MEM: %f\n"
                "Temp promediada escrita en la SH MEM: %f\n"
                "Temp con filtro EWMA escrita en la SH MEM: %f\n"DEFAULT, out_data->out_temp, out_data->SMA_out_temp, out_data->EWMA_out_temp);

    
    printf("____________________________________________________________\n"
           " Datos sin filtrar escritos en la memoria compartida: \n"
            "out_ac_x: %f\n"
            "out_ac_y: %f\n"
            "out_ac_z: %f\n"
            "out_temp: %f\n"
            "out_gy_x: %f\n"
            "out_gy_y: %f\n"
            "out_gy_z: %f\n", 
            out_data->out_ac_x, out_data->out_ac_y, out_data->out_ac_z, out_data->out_temp, out_data->out_gy_x, out_data->out_gy_y, out_data->out_gy_z);
    printf("____________________________________________________________\n"
           " Datos filtrados en el filtro SMA escritos en la memoria compartida: \n"
            "SMA_out_ac_x: %f\n"
            "SMA_out_ac_y: %f\n"
            "SMA_out_ac_z: %f\n"
            "SMA_out_temp: %f\n"
            "SMA_out_gy_x: %f\n"
            "SMA_out_gy_y: %f\n"
            "SMA_out_gy_z: %f\n", 
            out_data->SMA_out_ac_x, out_data->SMA_out_ac_y, out_data->SMA_out_ac_z, out_data->SMA_out_temp, out_data->SMA_out_gy_x, out_data->SMA_out_gy_y, out_data->SMA_out_gy_z);
    printf("____________________________________________________________\n"
           " Datos filtrados en el filtro EWMA escritos en la memoria compartida: \n"
            "EWMA_out_ac_x: %f\n"
            "EWMA_out_ac_y: %f\n"
            "EWMA_out_ac_z: %f\n"
            "EWMA_out_temp: %f\n"
            "EWMA_out_gy_x: %f\n"
            "EWMA_out_gy_y: %f\n"
            "EWMA_out_gy_z: %f\n", 
            out_data->EWMA_out_ac_x, out_data->EWMA_out_ac_y, out_data->EWMA_out_ac_z, out_data->EWMA_out_temp, out_data->EWMA_out_gy_x, out_data->EWMA_out_gy_y, out_data->EWMA_out_gy_z);
    
    */
    // shMem->inIdx++;
    // shMem->inIdx %= 100;
    close(driver_fd);

    // todo lo de la shmem tienen que sea floats
}

void sensor_main(int shmID)
{
    dataBuffer bufData;

    // proceso hijo
    printf("[LOG] SENSOR: Proceso hijo PID: %d\n", getpid()); // get pid
    
    printf("[LOG] SENSOR: Hola soy el programa sensor, cargando la SHMEM \n");
    printf("[LOG] SENSOR: Mi pid es: %d \n", getpid());
    

    if ((bufData.data = shmat(shmID, (void*) 0, 0)) == (void *) -1)
    {
        perror(RED"[ERROR] SENSOR: Al attach"DEFAULT);
        return;
    }
    
    semID = sem_open(SEM_KEY, O_CREAT, 0666, 1);        // Create semaphore to avoid collisions in shared memory
    if(semID == SEM_FAILED)
    {
        perror(RED"[ERROR] SENSOR: Al crear el semaforo"DEFAULT);
        return;
    }

    //bufData.inIdx = 0;

    out_data = (outData*)calloc(1, sizeof(outData));

    while (close_socket == 0)
    {
        if(sem_wait(semID) < 0)                                                // Takes the semaphore
        {
            perror(RED"[ERROR] SENSOR: Al esperar el semaforo"DEFAULT);
            return;
        }

        cargarSharedMem(&bufData);

        if(sem_post(semID) < 0)                                                // Free the semaphore
        {
            perror(RED"[ERROR] SENSOR: Al liberar el semaforo"DEFAULT);
            return;
        }
        usleep(500000);
    }

    printf("[LOG] SENSOR: Cerrando el productor\n");

    //printf("SHMEM: %s \n", shDat);
    
    //scanf("%d", &a);
    //execlp("gnome-terminal", "gnome-terminal", "--", "bash", "-c", SENSOR_PATH, NULL); //cambia la img del proceso q estoy corriendo por una totalmente aparte

}