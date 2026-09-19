#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>

#include "tcp_server.h"
#include "telemetry.h"
#include "alerts.h"
#include "udp_server.h"

#define TCP_PORT 5001
#define RESPONSE_SIZE 8192
#define COMMAND_SIZE 128

static void append(char *response, size_t *used, const char *format,
                   const char *a, const char *b, double value)
{
    if (*used >= RESPONSE_SIZE) return;
    int written = snprintf(response + *used, RESPONSE_SIZE - *used,
                           format, a, b, value);
    if (written < 0) return;
    *used += (size_t)written;
    if (*used >= RESPONSE_SIZE) *used = RESPONSE_SIZE - 1;
}

static void format_node(char *response, size_t size, const char *prefix,
                        const NodeTelemetry *node)
{
    snprintf(response, size,
             "%s|%s|TEMP|%.2f|HUM|%.2f|ENERGY|%.2f|VIB|%.2f|ACTIVE|%d",
             prefix, node->id, node->temperature, node->humidity,
             node->energy, node->vibration, node->active);
}

static void process_command(const char *command, char *response)
{
    NodeTelemetry node;
    if (strcmp(command, "GET_NODES") == 0) {
        NodeTelemetry snapshot[MAX_NODES];
        int count = get_all_nodes(snapshot, MAX_NODES);
        int active = 0;
        for (int i = 0; i < count; ++i) active += snapshot[i].active;
        size_t used = (size_t)snprintf(response, RESPONSE_SIZE, "OK|NODES|%d", active);
        for (int i = 0; i < count; ++i)
            if (snapshot[i].active)
                append(response, &used, "|%s", snapshot[i].id, "", 0);
    } else if (strncmp(command, "GET_STATUS|", 11) == 0) {
        const char *id = command + 11;
        if (*id && strlen(id) < NODE_ID_SIZE && get_node(id, &node))
            format_node(response, RESPONSE_SIZE, "OK|NODE", &node);
        else
            snprintf(response, RESPONSE_SIZE, "ERROR|NODE_NOT_FOUND");
    } else if (strcmp(command, "GET_LATEST") == 0) {
        if (get_latest(&node))
            format_node(response, RESPONSE_SIZE, "OK|LATEST", &node);
        else
            snprintf(response, RESPONSE_SIZE, "ERROR|NO_TELEMETRY");
    } else if (strcmp(command, "GET_ALERTS") == 0) {
        Alert snapshot[MAX_ALERTS];
        int count = get_alerts(snapshot, MAX_ALERTS);
        size_t used = (size_t)snprintf(response, RESPONSE_SIZE, "OK|ALERTS|%d", count);
        for (int i = 0; i < count; ++i)
            append(response, &used, "|%s,%s,%.2f", snapshot[i].node_id,
                   snapshot[i].type, snapshot[i].value);
    } else if (strcmp(command, "GET_SYSTEM_STATUS") == 0) {
        snprintf(response, RESPONSE_SIZE, "OK|SYSTEM|NODES|%d|ALERTS|%d",
                 get_node_count(), get_alert_count());
    } else if (strcmp(command, "GET_STATS") == 0) {
        snprintf(response, RESPONSE_SIZE, "OK|STATS|RECEIVED|%lu|INVALID|%lu",
                 get_received_count(), get_invalid_count());
    } else {
        snprintf(response, RESPONSE_SIZE, "ERROR|UNKNOWN_COMMAND");
    }
}

static void *client_handler(void *arg)
{
    int client = *(int *)arg;
    free(arg);
    char command[COMMAND_SIZE];
    char response[RESPONSE_SIZE] = {0};
    size_t used = 0;
    char ch;
    ssize_t received;
    int too_long = 0;
    int complete = 0;
    struct timeval timeout = {.tv_sec = 3, .tv_usec = 0};
    setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    while ((received = recv(client, &ch, 1, 0)) == 1) {
        if (ch == '\n') { complete = 1; break; }
        if (ch != '\r') {
            if (used == sizeof(command) - 1) { too_long = 1; break; }
            command[used++] = ch;
        }
    }
    command[used] = '\0';
    if (too_long)
        snprintf(response, sizeof(response), "ERROR|COMMAND_TOO_LONG");
    else if (used == 0)
        snprintf(response, sizeof(response), "ERROR|EMPTY_COMMAND");
    else if (!complete)
        snprintf(response, sizeof(response), "ERROR|INCOMPLETE_COMMAND");
    else
        process_command(command, response);

    size_t length = strlen(response);
    for (size_t sent = 0; sent < length;) {
        ssize_t n = send(client, response + sent, length - sent, 0);
        if (n <= 0) break;
        sent += (size_t)n;
    }
    close(client);
    return NULL;
}

void start_tcp_server(void)
{
    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) { perror("socket TCP"); exit(EXIT_FAILURE); }
    int reuse = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(TCP_PORT);
    if (bind(server, (struct sockaddr *)&address, sizeof(address)) < 0 ||
        listen(server, 10) < 0) {
        perror("bind/listen TCP");
        close(server);
        exit(EXIT_FAILURE);
    }
    printf("Servidor TCP escuchando en puerto %d\n", TCP_PORT);
    for (;;) {
        int *client = malloc(sizeof(*client));
        if (!client) continue;
        *client = accept(server, NULL, NULL);
        if (*client < 0) { free(client); continue; }
        pthread_t thread;
        if (pthread_create(&thread, NULL, client_handler, client) != 0) {
            close(*client);
            free(client);
            continue;
        }
        pthread_detach(thread);
    }
}
