#include <stdio.h>
#include <pthread.h>
#include <signal.h>


#include "udp_server.h"
#include "tcp_server.h"



void *udp_thread(void *arg)
{

    start_udp_server();

    return NULL;

}



void *tcp_thread(void *arg)
{

    start_tcp_server();

    return NULL;

}




int main()
{

    signal(SIGPIPE, SIG_IGN);


    printf("\n");
    printf("===============================\n");
    printf(" SMART TELEMETRY PLATFORM\n");
    printf(" CENTRAL SERVER\n");
    printf("===============================\n\n");



    pthread_t udp_tid;
    pthread_t tcp_tid;



    /*
        Crear servidor UDP
    */

    if (pthread_create(
        &udp_tid,
        NULL,
        udp_thread,
        NULL
    ) != 0) {
        perror("pthread_create UDP");
        return 1;
    }



    /*
        Crear servidor TCP
    */

    if (pthread_create(
        &tcp_tid,
        NULL,
        tcp_thread,
        NULL
    ) != 0) {
        perror("pthread_create TCP");
        return 1;
    }




    /*
        Mantener procesos vivos
    */

    pthread_join(
        udp_tid,
        NULL
    );


    pthread_join(
        tcp_tid,
        NULL
    );



    return 0;

}
