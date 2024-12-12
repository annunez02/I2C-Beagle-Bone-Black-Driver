/*
signal handler -> una vez que recibe la señal el padre entra a la funcion que tiene el waitpid
exit hace un llamado al sistema operativo para avisar que termino el proceso 
*/

#include "../inc/server_functions.h"
#include "../inc/sensor_functions.h"
#include "../inc/filter.h"

//thread_info threads[MAX_CLIENTS];

thread_info* threads = NULL;
int cant_th         = 0;
int close_socket    = 0;
int sigint_counter  = 0;

extern int max_clients;
extern int backlog;
extern outData * out_data;

sem_t * semID       = 0;
outData * shmdata   = 0;
pid_t pid           = 0;            // Variable para guardar PID's    


int main(int argc, char **argv)
{
    int s       = 0;
    int puerto  = 0;

    struct sockaddr_in local;       // Struct para colocar datos en bind()
    struct sockaddr_in cliente;     // Para el accept
    socklen_t l_cliente;            // Variable para guardar el tamaño de la estructura cliente usada en aceppt()
    
    int n_sock  = 0;                // File descriptor del canal de datos usada por aceppt

    key_t shmkey    = 0;            // msg key for shared memory
    int shmID       = 0;               

    int i = 0, found = 0;

            
    
    signal(SIGINT, sigint_handler);
    signal(SIGUSR2, sigusr2_handler);


    if (argc != 2)
    {
        printf("Ingresar %s <port_donde_servir>\n", argv[0]);
        exit(1);
    }
    else
    {
        shmkey = ftok(KEY_PATH, 'A');

        shmID = shmget(shmkey, 1024, 0644 | IPC_CREAT); 
        if (shmID < 0)
        {
            perror("[ERROR] MAIN: Al realizar shmget");
            close_socket = 1;
        }

        semID = sem_open(SEM_KEY, O_CREAT, 0666, 1);        // Create semaphore to avoid collisions in shared memory
        if(semID == SEM_FAILED)
        {
            perror("[ERROR] MAIN: Al crear el semaforo");
            close_socket = 1;
        }
    
        read_file();


        switch (pid = fork())
        {
            case -1:

                perror("[ERROR] MAIN: En la creacion del fork");
                close_socket = 1;
            
                break;
            
            case 0:         // proceso hijo

                sensor_main(shmID);
                
                break; 

            default:        // proceso padre
                
                shmdata = shmat(shmID, (void *)0, SHM_RDONLY); // attach a la shared memory
                if (shmdata == (void *) -1)
                {
                    perror("[ERROR] MAIN: Al attach");
                    close_socket = 1;
                }

                printf("[LOG] MAIN: Proceso padre PID: %d\n", getpid());    // get pid
                printf("[LOG] MAIN: Proceso hijo PID: %d\n", pid); //             

                /* Seteamos el handler para recibir la señal del proceso hijo pero ignorarla */
                signal(SIGCHLD, sigchld_handler);   //sigignore para que cuando recibe la señal la ignore, es consciente que esta recibiendo la señal


                threads = (thread_info*)malloc(max_clients * sizeof(thread_info));

                if (threads == NULL)
                {
                    perror("[ERROR] MAIN: Al hacer malloc");
                    close_socket = 1;
                }
                // inicializo el bit te atividad en 0 por las dudas

                for (int j = 0; j < max_clients; j++)
                {
                    threads[j].active = 0;
                    threads[j].accept_sfd = 0;
                    threads[j].thread_id = 0;
                }
                

                /* PASO 1: socket() */
                s = socket(AF_INET, SOCK_STREAM, 0);    // Creo el socket, guardo en s el descriptor del socket
                if (s < 0)
                {
                    perror("[ERROR] MAIN: No se pudo crear el socket");
                    close_socket = 1;
                }

                puerto = atoi(argv[1]);                     // aton --> ascii to network
                local.sin_family = AF_INET;                 // Es la direccion y puerto local que le queremos asignar al socket
                local.sin_port = htons(puerto);             // host to network short (short de máquina a short de la red)
                local.sin_addr.s_addr = htonl(INADDR_ANY);  // IP de nuestra maquina. Se le agina INADDR_ANY para que sea portable, host to network long 
                memset((void *)&(local.sin_zero), '\0', 8); // Lleno la estructura con ceros

                /* Para solucionar el error del puerto no reutilizable */
                int opt = 1;
                if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) 
                {
                    perror("setsockopt(SO_REUSEADDR) failed");
                    close(s);
                    exit(1);
                }


                /* PASO 2 :bind() */
                if (bind(s, (struct sockaddr *)&local, sizeof(local)) < 0)
                {
                    perror("[ERROR] MAIN: Asignando direccion");
                    close_socket = 1;
                }

                

                /* PASO 3: listen() y accept () */      // Ponemos al socket en modo pasivo a la espera de conexiones
                listen(s, backlog);                   // backlog es el tamaño maximo la cola de pedidos de conexion

                saRestart();                                            // Setup signal handler without SA_RESTART

                printf("[LOG] MAIN: Esperando conexiones...\n");


                while (close_socket == 0)
                {
                    l_cliente = sizeof(cliente);
                    n_sock = accept(s, (struct sockaddr *)&cliente, &l_cliente);    // Completa los datos del cliente en el socket de datos n_sock
                    
                    // acepto la conexion y si veo que ya tengo todos los que puedo tener, entonces cierro el n_sock del que acabo de aceptar
                    // si termina uno entonces vuelvo a atender el accept

                    if (n_sock < 0)
                    {
                        perror("[ERROR] MAIN: Al aceptar el cliente");
                    }
                    else if (cant_th >= max_clients)
                    {
                        perror("[ERROR] MAIN: Maxima cantidad de clientes alcanzada");
                            if (close(n_sock) < 0)
                            {
                                perror("[ERROR] MAIN: Cerrando el socket del thread");
                                close_socket = 1;
                            }
                    }
                    else
                    {
                        printf("[LOG] MAIN: Conexión establecidacon %s: %d\n", inet_ntoa(cliente.sin_addr), ntohs(cliente.sin_port));  

                        while (i < max_clients && found != 1)
                        {
                            if (threads[i].active == 0)
                            {
                                found = 1;
                            }
                            else i++;
                        }
                        

                        // como found la uso unicamente para salir del while anterior, la vuelvo a poner en cero una vez que salgo
                        found = 0;
                        
                        threads[i].accept_sfd = n_sock;
                        /*PASO 4: read() write() --> comunicacion*/

                        if(pthread_create(&threads[i].thread_id, NULL, connection_handler, (void*) &(threads[i])) < 0)
                        {
                            perror("[ERROR] MAIN: No se pudo crear el thread");
                            close_socket = 1;
                        }
                        
                        puts("[LOG] MAIN: Handler asignado");

                    }

                }

                for (i = 0; i < max_clients; i++)
                {
                    if(threads[i].active == 1)
                    {
                        if (pthread_cancel(threads[i].thread_id) < 0)
                        {
                            perror("[ERROR] MAIN: Cerrando los threads");
                        }
                        if (close(threads[i].accept_sfd) < 0)
                        {
                            perror("[ERROR] MAIN: Cerrando el socket");
                        }
                    }
                }
               
                free(threads);
                free(out_data);
                SMA_free();

                if (close(s) < 0)
                {
                    perror("[ERROR] MAIN: Cerrando el socket principal");
                }

                if (sem_close(semID) < 0) // Close semaphore
                {
                    perror("[ERROR] MAIN: Cerrando el semaforo");
                }

                if (sem_unlink(SEM_KEY) == -1) 
                {
                    perror("[ERROR] MAIN: sem_unlink");
                }
                
                if (shmctl(shmID, IPC_RMID, NULL) == -1) 
                {
                    perror("[ERROR] MAIN: shmctl");
                }
                    
                break;
        }
    }
    return 0;
}


// to run: ./thread

// para clientes TCP, y para datos UDP porque no nos importa la integridad de los datos pero si la velocidad