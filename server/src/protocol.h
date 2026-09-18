#ifndef PROTOCOL_H
#define PROTOCOL_H


#include "models.h"


int parse_telemetry(
    char *message,
    NodeTelemetry *node
);


#endif