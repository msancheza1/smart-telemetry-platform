#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "protocol.h"

static int parse_value(const char *text, float *value)
{
    char *end;
    errno = 0;
    *value = strtof(text, &end);
    return text[0] != '\0' && *end == '\0' && errno != ERANGE &&
           isfinite(*value) && *value >= 0;
}

int parse_telemetry(const char *message, NodeTelemetry *node)
{
    char buffer[1024];
    char *fields[10];
    char *save = NULL;
    size_t length;
    NodeTelemetry parsed = {0};

    if (!message || !node) return 0;
    length = strnlen(message, sizeof(buffer));
    if (length == 0 || length >= sizeof(buffer) || message[0] == '|' ||
        message[length - 1] == '|' || strstr(message, "||") != NULL) return 0;
    memcpy(buffer, message, length + 1);

    for (int i = 0; i < 10; ++i) {
        fields[i] = strtok_r(i == 0 ? buffer : NULL, "|", &save);
        if (!fields[i]) return 0;
    }
    if (strtok_r(NULL, "|", &save) != NULL) return 0;
    if (strcmp(fields[0], "TELEMETRY") != 0 ||
        strcmp(fields[2], "TEMP") != 0 ||
        strcmp(fields[4], "HUM") != 0 ||
        strcmp(fields[6], "ENERGY") != 0 ||
        strcmp(fields[8], "VIB") != 0) return 0;

    length = strlen(fields[1]);
    if (length == 0 || length >= sizeof(parsed.id)) return 0;
    for (size_t i = 0; i < length; ++i) {
        unsigned char c = (unsigned char)fields[1][i];
        if (!isalnum(c) && c != '_' && c != '-') return 0;
    }
    memcpy(parsed.id, fields[1], length + 1);
    if (!parse_value(fields[3], &parsed.temperature) ||
        !parse_value(fields[5], &parsed.humidity) ||
        !parse_value(fields[7], &parsed.energy) ||
        !parse_value(fields[9], &parsed.vibration) ||
        parsed.humidity > 100) return 0;

    parsed.active = 1;
    *node = parsed;
    return 1;
}
