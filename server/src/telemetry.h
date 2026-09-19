#ifndef TELEMETRY_H
#define TELEMETRY_H

#include "models.h"

/* Copies data while holding the internal lock. */
void update_node(NodeTelemetry node);
int get_node(const char *id, NodeTelemetry *result);
int get_all_nodes(NodeTelemetry *results, int capacity);
int get_latest(NodeTelemetry *result);
int get_node_count(void);
void print_nodes(void);

#endif
