"""Basic HTTP dashboard backed by the central server's TCP protocol."""

import html
import os
import socket
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer


TCP_HOST = os.environ.get("TELEMETRY_TCP_HOST", "127.0.0.1")
WEB_PORT = int(os.environ.get("WEB_PORT", "8080"))


def query(command):
    with socket.create_connection((TCP_HOST, 5001), timeout=2) as connection:
        connection.sendall((command + "\n").encode("ascii"))
        chunks = []
        while chunk := connection.recv(4096):
            chunks.append(chunk)
    return b"".join(chunks).decode("ascii", errors="replace")


def page():
    try:
        status = query("GET_SYSTEM_STATUS").split("|")
        nodes = query("GET_NODES").split("|")
        latest = query("GET_LATEST")
        alerts = query("GET_ALERTS").split("|")
        if status[:2] != ["OK", "SYSTEM"] or nodes[:2] != ["OK", "NODES"]:
            raise ValueError("Invalid server response")
        registered = status[3]
        active = nodes[2]
        cards = []
        for node_id in nodes[3:]:
            reading = query("GET_STATUS|" + node_id).split("|")
            if reading[:2] == ["OK", "NODE"]:
                cards.append(
                    "<tr>" + "".join(f"<td>{html.escape(value)}</td>" for value in
                    [reading[2], reading[4], reading[6], reading[8], reading[10]]) + "</tr>"
                )
        recent = []
        if alerts[:2] == ["OK", "ALERTS"]:
            recent = alerts[3:][-10:]
        alert_items = "".join(f"<li>{html.escape(item)}</li>" for item in reversed(recent))
        body = f"""<!doctype html><html lang="es"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta http-equiv="refresh" content="5"><title>Smart Telemetry</title>
<style>body{{font:16px system-ui;max-width:1000px;margin:2rem auto;padding:0 1rem;
background:#f4f7fb;color:#13233a}}h1{{margin-bottom:.2rem}}.stats{{display:flex;gap:1rem;flex-wrap:wrap}}
.card{{background:white;padding:1rem 1.5rem;border-radius:10px;box-shadow:0 2px 8px #ccd3dc}}
table{{border-collapse:collapse;width:100%;background:white}}td,th{{padding:.7rem;border-bottom:1px solid #ddd;text-align:left}}
code{{overflow-wrap:anywhere}}li{{margin:.4rem 0}}</style></head><body>
<h1>Smart Telemetry</h1><p>Servidor en línea · actualización cada 5 segundos</p>
<div class="stats"><div class="card">Nodos registrados: <strong>{html.escape(registered)}</strong></div>
<div class="card">Nodos activos: <strong>{html.escape(active)}</strong></div>
<div class="card">Alertas conservadas: <strong>{html.escape(status[5])}</strong></div></div>
<h2>Nodos activos</h2><table><tr><th>ID</th><th>Temperatura</th><th>Humedad</th>
<th>Energía</th><th>Vibración</th></tr>{''.join(cards)}</table>
<h2>Última medición</h2><p><code>{html.escape(latest)}</code></p>
<h2>Alertas recientes</h2><ul>{alert_items or '<li>Sin alertas</li>'}</ul>
</body></html>"""
        return 200, body
    except (OSError, ValueError, IndexError) as error:
        return 503, ("<!doctype html><html lang='es'><meta charset='utf-8'>"
                     "<title>Servidor no disponible</title><h1>Servidor no disponible</h1>"
                     f"<p>{html.escape(str(error))}</p></html>")


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path != "/":
            self.send_error(404)
            return
        code, body = page()
        data = body.encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)


if __name__ == "__main__":
    ThreadingHTTPServer(("0.0.0.0", WEB_PORT), Handler).serve_forever()
