#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "alerts.h"

static Alert alerts[MAX_ALERTS];
static int alert_count = 0;
static int next_alert = 0;
static pthread_mutex_t alerts_lock = PTHREAD_MUTEX_INITIALIZER;

static void create_alert(const char *node_id, const char *type, float value)
{
    pthread_mutex_lock(&alerts_lock);
    Alert *alert = &alerts[next_alert];
    snprintf(alert->node_id, sizeof(alert->node_id), "%s", node_id);
    snprintf(alert->type, sizeof(alert->type), "%s", type);
    alert->value = value;
    next_alert = (next_alert + 1) % MAX_ALERTS;
    if (alert_count < MAX_ALERTS) ++alert_count;
    pthread_mutex_unlock(&alerts_lock);
    printf("ALERTA: %s | %s | %.2f\n", node_id, type, value);
}

void check_alerts(NodeTelemetry node)
{
    if (node.temperature > 40) create_alert(node.id, "TEMP_HIGH", node.temperature);
    if (node.humidity > 90) create_alert(node.id, "HUM_HIGH", node.humidity);
    if (node.energy > 800) create_alert(node.id, "ENERGY_HIGH", node.energy);
    if (node.vibration > 8) create_alert(node.id, "VIB_HIGH", node.vibration);
}

int get_alert_count(void)
{
    pthread_mutex_lock(&alerts_lock);
    int count = alert_count;
    pthread_mutex_unlock(&alerts_lock);
    return count;
}

int get_alerts(Alert *results, int capacity)
{
    pthread_mutex_lock(&alerts_lock);
    int count = alert_count < capacity ? alert_count : capacity;
    int oldest = (next_alert - alert_count + MAX_ALERTS) % MAX_ALERTS;
    for (int i = 0; i < count; ++i)
        results[i] = alerts[(oldest + i) % MAX_ALERTS];
    pthread_mutex_unlock(&alerts_lock);
    return count;
}

void print_alerts(void)
{
    Alert snapshot[MAX_ALERTS];
    int count = get_alerts(snapshot, MAX_ALERTS);
    printf("\n===== ALERTAS =====\n");
    for (int i = 0; i < count; ++i)
        printf("%s | %s | %.2f\n", snapshot[i].node_id,
               snapshot[i].type, snapshot[i].value);
}
