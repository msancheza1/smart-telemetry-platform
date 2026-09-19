#ifndef ALERTS_H
#define ALERTS_H

#include "models.h"

#define MAX_ALERTS 100

typedef struct {
    char node_id[NODE_ID_SIZE];
    char type[32];
    float value;
} Alert;

void check_alerts(NodeTelemetry node);
int get_alert_count(void);
int get_alerts(Alert *results, int capacity);
void print_alerts(void);

#endif
