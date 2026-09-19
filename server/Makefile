CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic -D_POSIX_C_SOURCE=200809L
LDLIBS = -pthread -lm
SRC = src/main.c src/udp_server.c src/tcp_server.c src/telemetry.c src/protocol.c src/alerts.c
OUT = smart_server

.PHONY: all run clean
all: $(OUT)

$(OUT): $(SRC) src/models.h src/udp_server.h src/tcp_server.h src/telemetry.h src/protocol.h src/alerts.h
	$(CC) $(CFLAGS) -pthread $(SRC) -o $@ $(LDLIBS)

run: $(OUT)
	./$(OUT)

clean:
	rm -f $(OUT)
