#ifndef TELEMETRY_H
#define TELEMETRY_H


#include "models.h"


void update_node(NodeTelemetry node);


NodeTelemetry* get_node(char *id);


void print_nodes();


int get_node_count();


#endif