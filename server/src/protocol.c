#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "protocol.h"


int parse_telemetry(
    char *message,
    NodeTelemetry *node
)
{

    char buffer[1024];

    strcpy(buffer, message);


    char *token;


    token = strtok(buffer, "|");


    if(token == NULL)
    {
        return 0;
    }


    if(strcmp(token, "TELEMETRY") != 0)
    {
        return 0;
    }



    // NODE ID

    token = strtok(NULL, "|");

    if(token == NULL)
    {
        return 0;
    }


    strcpy(node->id, token);



    while((token = strtok(NULL, "|")) != NULL)
    {

        if(strcmp(token, "TEMP") == 0)
        {

            token = strtok(NULL, "|");

            node->temperature =
                atof(token);

        }


        else if(strcmp(token, "HUM") == 0)
        {

            token = strtok(NULL, "|");

            node->humidity =
                atof(token);

        }


        else if(strcmp(token, "ENERGY") == 0)
        {

            token = strtok(NULL, "|");

            node->energy =
                atof(token);

        }


        else if(strcmp(token, "VIB") == 0)
        {

            token = strtok(NULL, "|");

            node->vibration =
                atof(token);

        }

    }


    return 1;

}