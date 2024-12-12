#include "../inc/server_functions.h"
#include "../inc/sensor_functions.h"
#include "../inc/filter.h"

extern int cant_th;
extern int close_socket;
extern int sigint_counter;

int max_clients         = MAX_CLIENTS;
int backlog             = SOMAXCONN;
int SMA_filter_order    = 3;
float EWMA_filter_alpha = 0.5;

extern outData * shmdata;
extern sem_t * semID;
extern pid_t pid;



void sigchld_handler(int signum)
{
    int rv = 0, pid = 0;
    signal(SIGCHLD, sigchld_handler); /*Reseteo la señal*/

    while ((pid = waitpid(-1, &rv, WNOHANG)) > 0)
    {
        printf("--- Saliendo del proceso hijo PID %d con status de salida %d ---\n\n", pid, WEXITSTATUS(rv));
    }
}
/* 
    -1 para q te tome cualquier status hijo
    status de salida (lo que devuelve el hijo)
    wnohang para que no sea bloqueante
    wexitstatus que valor de retorno tuvo la funcion
*/


// Funcion para conectar con cada cliente
void * connection_handler(void *thread_rcvd)
{
    //Get the socket descriptor
    thread_info * thread = (thread_info *) thread_rcvd;
    outData out_data;

    int sock_fd = thread->accept_sfd;
    char message_buff[BUFFER_SIZE];
    int active = 1;

    /* Inicializacion */
    
    thread->semID_th = semID;
    thread->shmDATA_th = shmdata;
    thread->active = 1;
    cant_th++;

    /* Conection with client */

    printf(CYAN"[LOG] HANDLER: Nueva conexión atendida en socket: %d\n"DEFAULT, sock_fd);


    /* Para comenzar, debo enviar un OK al cliente */

    if(write(sock_fd, "OK", sizeof(char) * 2) < 0)    
    {
        perror(RED"[ERROR] HANDLER: Error al envar OK" DEFAULT);
        active = 0;
    }    

    printf(CYAN"[LOG] HANDLER: Sent first OK\n"DEFAULT);
    
    /* Recibo AKN */
    

    if(read(sock_fd, message_buff, sizeof(char) * 3 ) < 0)
    {
        perror(RED"[ERROR] HANDLER: Al leer AKN" DEFAULT);
        active = 0;
    }    

    if(message_buff[0] != 'A' || message_buff[1] != 'K' || message_buff[2] !='N')
    {
        perror(RED"[ERROR] HANDLER: AKN invalido" DEFAULT);
        active = 0;
    }
    
    printf(CYAN"[LOG] HANDLER: Recieved AKN\n"DEFAULT);

    /* Una vez que lo recibi, mando "OK" denuevo (3 way handshake) */
    
    if(write(sock_fd, "OK", sizeof(char) * 2) < 0)    
    {
        perror(RED"[ERROR] HANDLER: Error al envar OK" DEFAULT);
        active = 0;
    }    


    printf(CYAN "[LOG] HANDLER: Sincronización satisfactoria. Comienza la transmision\n"DEFAULT);  //Succesful sync with client
    
    while(active) 
    {
        /* Esperando el mensaje del cliente*/

        if(read(sock_fd, message_buff, BUFFER_SIZE) < 0)
        {
            perror(RED"[ERROR] HANDLER: Al leer el mensaje del cliente" DEFAULT);
            active = 0;
        }
        //printf(CYAN "[LOG] HANDLER: read ok!\n"DEFAULT);    

        /* Si el cliente me manda un Keep Alive (KA), accedo a la Shared Mem para leer los datos del sensor */

        if(message_buff[0] == 'K' && message_buff[1] == 'A')
        {

            if(sem_wait(thread->semID_th) < 0)                                                
            {
                perror(RED"[ERROR] HANDLER: Al esperar el semaforo"DEFAULT);
                active = 0;
            }
            //printf(CYAN "[LOG] HANDLER: semwait ok!\n"DEFAULT);    

            memcpy(&out_data, thread->shmDATA_th, sizeof(outData));
            
            if(sem_post(thread->semID_th) < 0)                                                // Free the semaphore
            {
                perror(RED"[ERROR] HANDLER: Al liberar el semaforo"DEFAULT);
                active = 0;
            }

            if(write(sock_fd, &out_data, sizeof(out_data)) < 0)    
            {
                perror(RED"[ERROR] HANDLER: Error al envar los datos" DEFAULT);
                active = 0;
            }
            //printf(CYAN "[LOG] HANDLER: write ok!\n"DEFAULT);
        }   

        /* Si el cliente manda END, termina la conexion */
		
        else if(message_buff[0] == 'E' && message_buff[1] == 'N' && message_buff[2] == 'D')
        {
            printf(CYAN"[LOG] HANDLER: Cliente solicito desconexion\n"DEFAULT);
            active = 0;
        }
        else
            printf(CYAN"[LOG] HANDLER: El mensaje recibido es desconocido (%s)\n"DEFAULT, message_buff);
    }
    
    /* cerrar el socket del file descriptor por desconeccion una vez que salgo del while*/
    
    if (close(sock_fd) < 0)
    {
        perror(RED"[ERROR] HANDLER: Cerrando el socket del thread"DEFAULT);
    }
    
    thread->active = 0;
    cant_th--;
    printf(CYAN "[LOG] HANDLER: Conexion finalizada para el socket: %d \n"DEFAULT, sock_fd);
  
    return 0;
} 


void sigint_handler(int signal)
{
    if (sigint_counter == 0)
    {
        close_socket = 1;
        sigint_counter++;
    }
    else
    {
        kill(pid, SIGKILL);
        perror(RED"[] Cerrando forzadamente"DEFAULT);
        exit(1);
    }
    
}

void read_file()
{
    FILE *file = fopen("file.txt", "r");
    if (file == 0)
    {
        perror(RED"[ERROR] READ FILE: No se pudo abrir el archivo"DEFAULT);
        return;
    }
    
    fscanf(file, "%d\n%d\n%d\n%f", &max_clients, &backlog, &SMA_filter_order, &EWMA_filter_alpha);
    printf("[LOG] READ FILE: %f\n", EWMA_filter_alpha);

    if (max_clients <= 0) 
    {
        perror(RED"[ERROR] READ FILE: Se debe permitir al menos un cliente"DEFAULT);
        return;
    }
    
    if (SMA_filter_order <= 0) 
    {
        perror(RED"[ERROR] READ FILE: El orden del promediador debe ser mayor a 0"DEFAULT);
        return;
    }
    
    if (EWMA_filter_alpha <= 0 || EWMA_filter_alpha > 1) 
    {
        perror(RED"[ERROR] READ FILE: El valor de alpha debe estar en el rango (0, 1]"DEFAULT);
        return;
    }
    SMA_alloc();
}

void sigusr2_handler(int signal)
{
    FILE *file = fopen("file.txt", "r");
    if (file == 0)
    {
        perror(RED"[ERROR] READ FILE: No se pudo abrir el archivo"DEFAULT);
        return;
    }
    
    fscanf(file, "%d\n%d\n%d\n%f", &max_clients, &backlog, &SMA_filter_order, &EWMA_filter_alpha);
    printf("[LOG] READ FILE: %f\n", EWMA_filter_alpha);

    if (max_clients <= 0) 
    {
        perror(RED"[ERROR] READ FILE: Se debe permitir al menos un cliente"DEFAULT);
        return;
    }
    
    if (SMA_filter_order <= 0) 
    {
        perror(RED"[ERROR] READ FILE: El orden del promediador debe ser mayor a 0"DEFAULT);
        return;
    }
    
    if (EWMA_filter_alpha <= 0 || EWMA_filter_alpha > 1) 
    {
        perror(RED"[ERROR] READ FILE: El valor de alpha debe estar en el rango (0, 1]"DEFAULT);
        return;
    }
    SMA_alloc();
}


void saRestart (void)
{
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sa.sa_flags = 0;                                    //Do not use SA_RESTART
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
}
