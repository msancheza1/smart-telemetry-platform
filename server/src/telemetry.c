#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "telemetry.h"

static NodeTelemetry nodes[MAX_NODES];
static NodeTelemetry latest;
static int node_count = 0;
static int has_latest = 0;
static pthread_mutex_t nodes_lock = PTHREAD_MUTEX_INITIALIZER;

void update_node(NodeTelemetry node)
{
    node.last_seen = time(NULL);
    node.active = 1;
    pthread_mutex_lock(&nodes_lock);
    for (int i = 0; i < node_count; ++i) {
        if (strcmp(nodes[i].id, node.id) == 0) {
            nodes[i] = node;
            latest = node;
            has_latest = 1;
            pthread_mutex_unlock(&nodes_lock);
            return;
        }
    }
    if (node_count < MAX_NODES) {
        nodes[node_count++] = node;
        latest = node;
        has_latest = 1;
    }
    pthread_mutex_unlock(&nodes_lock);
}

int get_node(const char *id, NodeTelemetry *result)
{
    int found = 0;
    pthread_mutex_lock(&nodes_lock);
    for (int i = 0; i < node_count; ++i) {
        if (strcmp(nodes[i].id, id) == 0) {
            *result = nodes[i];
            result->active = difftime(time(NULL), result->last_seen) <= NODE_TIMEOUT_SECONDS;
            found = 1;
            break;
        }
    }
    pthread_mutex_unlock(&nodes_lock);
    return found;
}

int get_all_nodes(NodeTelemetry *results, int capacity)
{
    pthread_mutex_lock(&nodes_lock);
    int count = node_count < capacity ? node_count : capacity;
    if (count > 0) memcpy(results, nodes, (size_t)count * sizeof(*results));
    time_t now = time(NULL);
    for (int i = 0; i < count; ++i)
        results[i].active = difftime(now, results[i].last_seen) <= NODE_TIMEOUT_SECONDS;
    pthread_mutex_unlock(&nodes_lock);
    return count;
}

int get_latest(NodeTelemetry *result)
{
    pthread_mutex_lock(&nodes_lock);
    int found = has_latest;
    if (found) {
        *result = latest;
        result->active = difftime(time(NULL), result->last_seen) <= NODE_TIMEOUT_SECONDS;
    }
    pthread_mutex_unlock(&nodes_lock);
    return found;
}

int get_node_count(void)
{
    pthread_mutex_lock(&nodes_lock);
    int count = node_count;
    pthread_mutex_unlock(&nodes_lock);
    return count;
}

void print_nodes(void)
{
    NodeTelemetry snapshot[MAX_NODES];
    int count = get_all_nodes(snapshot, MAX_NODES);
    printf("\n===== NODOS REGISTRADOS =====\n");
    for (int i = 0; i < count; ++i)
        printf("%s | TEMP %.2f | HUM %.2f | ENERGY %.2f | VIB %.2f\n",
               snapshot[i].id, snapshot[i].temperature, snapshot[i].humidity,
               snapshot[i].energy, snapshot[i].vibration);
}
