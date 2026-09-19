#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "models.h"

/* Returns 1 for a complete, valid telemetry message, otherwise 0. */
int parse_telemetry(const char *message, NodeTelemetry *node);

#endif
