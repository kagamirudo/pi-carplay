#!/usr/bin/env python3
"""A stand-in for the GitHub releases API, for the pi-carplay-update tests.

    fake_github.py ROOT PORT_FILE

Serves GET /<path> from ROOT/<path>.json (the query string is ignored).
Optional siblings change the answer: <path>.status holds the HTTP status,
<path>.headers holds extra "Name: value" lines, <path>.sleep a delay in
seconds. Listens on a free localhost port and writes it to PORT_FILE.
"""

import http.server
import os
import sys
import time
import urllib.parse


class Handler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        root = self.server.root
        rel = urllib.parse.urlsplit(self.path).path.strip("/")
        base = os.path.realpath(os.path.join(root, rel))
        if not base.startswith(root + os.sep):
            self.send_error(400)
            return

        def read(ext, default=""):
            try:
                with open(base + ext, encoding="utf-8") as f:
                    return f.read()
            except OSError:
                return default

        delay = read(".sleep")
        if delay:
            time.sleep(float(delay))
        body = read(".json", None)
        status = int(read(".status", "200" if body is not None else "404"))
        payload = (body if body is not None else '{"message": "Not Found"}').encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(payload)))
        for line in read(".headers").splitlines():
            name, _, value = line.partition(":")
            if name:
                self.send_header(name.strip(), value.strip())
        self.end_headers()
        self.wfile.write(payload)

    def log_message(self, *args):
        pass


def main():
    root, port_file = sys.argv[1:]
    server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), Handler)
    server.root = os.path.realpath(root)
    with open(port_file + ".tmp", "w", encoding="utf-8") as f:
        f.write(str(server.server_address[1]))
    os.replace(port_file + ".tmp", port_file)
    server.serve_forever()


if __name__ == "__main__":
    main()
