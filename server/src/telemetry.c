#include <stdio.h>
#include <string.h>

#include "telemetry.h"


static NodeTelemetry nodes[MAX_NODES];

static int node_count = 0;



void update_node(NodeTelemetry node)
{

    for(int i = 0; i < node_count; i++)
    {

        if(strcmp(nodes[i].id, node.id) == 0)
        {

            nodes[i] = node;
            nodes[i].active = 1;

            return;
        }

    }


    if(node_count < MAX_NODES)
    {

        nodes[node_count] = node;

        nodes[node_count].active = 1;

        node_count++;

    }

}



NodeTelemetry* get_node(char *id)
{

    for(int i = 0; i < node_count; i++)
    {

        if(strcmp(nodes[i].id, id) == 0)
        {
            return &nodes[i];
        }

    }


    return NULL;

}



void print_nodes()
{

    printf("\n===== NODOS REGISTRADOS =====\n");


    for(int i = 0; i < node_count; i++)
    {

        printf(
            "%s | TEMP: %.2f | HUM: %.2f | ENERGY: %.2f | VIB: %.2f\n",
            nodes[i].id,
            nodes[i].temperature,
            nodes[i].humidity,
            nodes[i].energy,
            nodes[i].vibration
        );

    }

}



int get_node_count()
{
    return node_count;
}