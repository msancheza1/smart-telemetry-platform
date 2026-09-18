#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "models.h"
#include "protocol.h"
#include "telemetry.h"



#define UDP_PORT 5000
#define BUFFER_SIZE 1024



int main()
{

    int server_socket;


    struct sockaddr_in server_address;
    struct sockaddr_in client_address;


    socklen_t client_length;


    char buffer[BUFFER_SIZE];



    /*
        Crear socket UDP
    */

    server_socket = socket(
        AF_INET,
        SOCK_DGRAM,
        0
    );


    if(server_socket < 0)
    {

        perror("Error creando socket");

        return 1;
    }


    printf("Socket UDP creado correctamente\n");



    /*
        Configuración del servidor
    */


    memset(
        &server_address,
        0,
        sizeof(server_address)
    );


    server_address.sin_family = AF_INET;


    server_address.sin_addr.s_addr =
        INADDR_ANY;


    server_address.sin_port =
        htons(UDP_PORT);



    /*
        Asociar socket al puerto
    */


    if(bind(
        server_socket,
        (struct sockaddr *)&server_address,
        sizeof(server_address)
    ) < 0)
    {

        perror("Error en bind");

        close(server_socket);

        return 1;
    }



    printf(
        "Servidor UDP escuchando en puerto %d\n",
        UDP_PORT
    );



    /*
        Recepción continua de mensajes
    */


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

            perror("Error recibiendo datos");

            continue;
        }



        buffer[bytes_received] = '\0';



        printf("\nMensaje recibido:\n");

        printf("%s\n", buffer);



        printf(
            "Cliente: %s\n",
            inet_ntoa(client_address.sin_addr)
        );



        /*
            Procesamiento del protocolo
        */


        NodeTelemetry node;



        if(parse_telemetry(buffer, &node))
        {

            update_node(node);



            printf("\nNodo actualizado correctamente:\n");


            printf(
                "ID: %s\n",
                node.id
            );


            printf(
                "Temperatura: %.2f\n",
                node.temperature
            );


            printf(
                "Humedad: %.2f\n",
                node.humidity
            );


            printf(
                "Energia: %.2f\n",
                node.energy
            );


            printf(
                "Vibracion: %.2f\n",
                node.vibration
            );

        }

        else
        {

            printf(
                "Mensaje invalido\n"
            );

        }


    }



    close(server_socket);


    return 0;

}