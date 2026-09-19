#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "udp_server.h"

#include "models.h"
#include "protocol.h"
#include "telemetry.h"
#include "alerts.h"



#define UDP_PORT 5000
#define BUFFER_SIZE 1024

static atomic_ulong received_count = 0;
static atomic_ulong invalid_count = 0;

unsigned long get_received_count(void) { return atomic_load(&received_count); }
unsigned long get_invalid_count(void) { return atomic_load(&invalid_count); }




void start_udp_server()
{


    int server_socket;



    struct sockaddr_in server_address;

    struct sockaddr_in client_address;



    socklen_t client_length;



    char buffer[BUFFER_SIZE];





    server_socket = socket(
        AF_INET,
        SOCK_DGRAM,
        0
    );



    if(server_socket < 0)
    {

        perror(
            "Error creando socket UDP"
        );

        exit(EXIT_FAILURE);

    }



    printf(
        "Socket UDP creado correctamente\n"
    );





    memset(
        &server_address,
        0,
        sizeof(server_address)
    );



    server_address.sin_family =
        AF_INET;



    server_address.sin_addr.s_addr =
        INADDR_ANY;



    server_address.sin_port =
        htons(UDP_PORT);





    if(bind(
        server_socket,
        (struct sockaddr *)&server_address,
        sizeof(server_address)
    ) < 0)
    {

        perror(
            "Error en bind UDP"
        );

        close(server_socket);

        exit(EXIT_FAILURE);

    }





    printf(
        "Servidor UDP escuchando en puerto %d\n",
        UDP_PORT
    );





    while(1)
    {


        client_length =
            sizeof(client_address);




        int bytes_received =
            recvfrom(
                server_socket,
                buffer,
                BUFFER_SIZE - 1,
                0,
                (struct sockaddr *)&client_address,
                &client_length
            );



        if(bytes_received < 0)
        {

            perror(
                "Error recibiendo UDP"
            );

            continue;

        }



        if (bytes_received == BUFFER_SIZE - 1 ||
            memchr(buffer, '\0', (size_t)bytes_received) != NULL)
        {
            atomic_fetch_add(&invalid_count, 1);
            printf("Mensaje UDP demasiado largo o con bytes nulos\n");
            continue;
        }

        buffer[bytes_received] = '\0';





        printf(
            "\nMensaje UDP recibido:\n%s\n",
            buffer
        );



        printf(
            "Cliente: %s\n",
            inet_ntoa(
                client_address.sin_addr
            )
        );





        NodeTelemetry node;




        if(parse_telemetry(
            buffer,
            &node
        ))
        {

            atomic_fetch_add(&received_count, 1);


            update_node(node);



            check_alerts(node);




            printf(
                "Nodo actualizado: %s\n",
                node.id
            );


        }
        else
        {

            atomic_fetch_add(&invalid_count, 1);

            printf(
                "Mensaje de telemetria invalido\n"
            );

        }



    }



    close(server_socket);


}
