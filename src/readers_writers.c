#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>

//#define _POSIX_C_SOURCE 200809L

#define CANT_LECTORES 4
#define CANT_ESCRITORES 2

static volatile sig_atomic_t g_running = 1;

static int readers;
static sem_t mutex;
static sem_t roomEmpty;

typedef struct {
    int id;
} arg_hilo_t;


static void on_sigint(int signum)
{
    (void)signum;
    g_running = 0;
}

static int install_signal_handlers(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_sigint;

    if (sigaction(SIGINT, &sa, NULL) < 0)
    {
        perror("sigaction");
        return -1;
    }

    return 0;
}

static void *lector(void *arg)
{
    arg_hilo_t *hilo = arg;

    while (g_running)
    {
        sem_wait(&mutex);
        readers += 1;
        
        if (readers == 1)
        {
            sem_wait(&roomEmpty);
        }
        sem_post(&mutex);

        //Seccion critica lectores
        printf("Lector %d leyendo \n", hilo->id);
        fflush(stdout);
        sleep(2);
        printf("Lector %d termina de leer \n", hilo->id);
        fflush(stdout);

        sem_wait(&mutex);
        readers -= 1;
        if (readers == 0)
        {
            sem_post(&roomEmpty);
        }
        sem_post(&mutex);
        sleep(1);
    }
    return NULL;
}

static void *escritor(void *arg)
{
    arg_hilo_t *hilo = arg;    

    while (g_running)
    {
        sem_wait(&roomEmpty);
        printf("Escritor %d escribiendo \n", hilo->id);
        fflush(stdout);
        sleep(2);
        printf("Escritor %d termina de escribir \n", hilo->id);
        fflush(stdout);
        sem_post(&roomEmpty);
        sleep(1);
    }

    return NULL;
}

int main(void)
{
    sem_init(&mutex,0,1);
    sem_init(&roomEmpty,0,1);

    pthread_t lectores[CANT_LECTORES];
    arg_hilo_t args_lectores[CANT_LECTORES];

    pthread_t escritores[CANT_ESCRITORES];
    arg_hilo_t args_escritores[CANT_ESCRITORES];   
    
    if (install_signal_handlers() < 0)
    {
        return EXIT_FAILURE;
    }

    printf("Readers-writers — Ctrl-C to stop\n");

    for (int i = 0; i < CANT_ESCRITORES; i++){
        args_escritores[i].id = i;
        int pthread_created = pthread_create(&escritores[i], NULL, escritor, &args_escritores[i]); 

        if (pthread_created != 0)
        {
            fprintf(stderr, "pthread_create failed %s\n", strerror(pthread_created));
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < CANT_LECTORES; i++){
        args_lectores[i].id = i;
        int pthread_created = pthread_create(&lectores[i], NULL, lector, &args_lectores[i]); 

        if (pthread_created != 0)
        {
            fprintf(stderr, "pthread_create failed %s\n", strerror(pthread_created));
            return EXIT_FAILURE;
        }
    }

    for (int i = 0; i < CANT_ESCRITORES; i++){
        int rc = pthread_join(escritores[i], NULL);
        if (rc != 0)
            fprintf(stderr, "pthread_join: %s\n", strerror(rc));
    }
    for (int i = 0; i < CANT_LECTORES; i++){
        int rc = pthread_join(lectores[i], NULL);
        if (rc != 0)
            fprintf(stderr, "pthread_join: %s\n", strerror(rc));
    }

    sem_destroy(&mutex);
    sem_destroy(&roomEmpty);

    return EXIT_SUCCESS;
}
