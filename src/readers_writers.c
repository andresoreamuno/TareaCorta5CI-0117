#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define _POSIX_C_SOURCE 200809L

#define CANT_LECTORES 4
#define CANT_ESCRITORES 2

static volatile sig_atomic_t g_running = 1;

static int readers;
static sem_t mutex;
static sem_t roomEmpty;

typedef struct {
    int file_descriptor;
    unsigned long connection_id;
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
        printf("Lector %lu leyendo \n", hilo->connection_id);
        sleep(2000);

        sem_wait(&mutex);
        readers -= 1;
        if (readers == 0)
        {
            sem_post(&roomEmpty);
        }
        sem_post(&mutex);
    }
    return NULL;
}

static void *escritor(void *arg)
{
    hilo_t *hilo = arg;    

    while (g_running)
    {
        sem_wait(&roomEmpty);
        printf("Escritor %lu escribiendo \n", hilo->connection_id);
        sleep(2000);
        sem_post(&roomEmpty);
    }

    return NULL;
}

int main(int argc, char **argv)
{
    pthread_t lectores[CANT_LECTORES];
    arg_hilo_t args_lectores[CANT_LECTORES];

    pthread_t escritores[CANT_ESCRITORES];
    arg_hilo_t args_escritores[CANT_ESCRITORES];   
    
    if (install_signal_handlers() < 0)
    {
        return EXIT_FAILURE;
    }

    printf("Readers-writers %u — Ctrl-C to stop\n", port);

    for (long i = 0; i < CANT_ESCRITORES; i++){
        int pthread_created = pthread_create(&escritores[i], NULL, escritor, NULL); 

        if (pthread_created != 0)
        {
            fprintf(stderr, "pthread_create failed %s\n", strerror(pthread_created));
            return EXIT_FAILURE;
        }
    }

    for (long i = 0; i < CANT_LECTORES; i++){
        int pthread_created = pthread_create(&lectores[i], NULL, lector, NULL); 

        if (pthread_created != 0)
        {
            fprintf(stderr, "pthread_create failed %s\n", strerror(pthread_created));
            return EXIT_FAILURE;
        }
    }

    for (long i = 0; i < CANT_ESCRITORES; i++){
        int rc = pthread_join(escritores[i], NULL);
        if (rc != 0)
            fprintf(stderr, "pthread_join: %s\n", strerror(rc));
    }
    for (long i = 0; i < CANT_LECTORES; i++){
        int rc = pthread_join(lectores[i], NULL);
        if (rc != 0)
            fprintf(stderr, "pthread_join: %s\n", strerror(rc));
    }


    return EXIT_SUCCESS;
}
