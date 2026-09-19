FROM gcc:14-bookworm AS build
WORKDIR /app
COPY server/ server/
RUN make -C server

FROM python:3.12-slim-bookworm
WORKDIR /app
COPY --from=build /app/server/smart_server /app/smart_server
COPY web/dashboard.py /app/dashboard.py
COPY docker-entrypoint.sh /app/docker-entrypoint.sh
RUN chmod +x /app/docker-entrypoint.sh
EXPOSE 5000/udp 5001/tcp 8080/tcp
CMD ["/app/docker-entrypoint.sh"]
