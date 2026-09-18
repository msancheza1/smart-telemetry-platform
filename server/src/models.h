#ifndef MODELS_H
#define MODELS_H


#define MAX_NODES 100
#define NODE_ID_SIZE 32


typedef struct
{
    char id[NODE_ID_SIZE];

    float temperature;
    float humidity;
    float energy;
    float vibration;

    int active;

} NodeTelemetry;


#endif