#!/usr/bin/env python3
from __future__ import annotations

import argparse
import socket
import sys
import signal


sys.path.insert(0, '/media/fat/mr-marquee')

from contextlib import contextmanager
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Iterator
from urllib.parse import urlparse

DEFAULT_PORT = 8090
DEFAULT_CORENAME_FILE = Path("/tmp/CORENAME")
DEFAULT_SETTINGS_FILE = Path("/media/fat/mr-marquee/settings.json")
DEFAULT_STATIC_ROOT =   Path("/media/fat/mr-marquee/banners")
DEFAULT_MDNS_NAME = "mister"

running = True
 
def signal_handler(signum, frame):
    """Handle system signals to stop the daemon gracefully."""
    global running
    logging.info(f"Received signal {signum}. Stopping daemon...")
    running = False
 
# Register signal handlers for SIGTERM (stop) and SIGINT (Ctrl+C)
signal.signal(signal.SIGTERM, signal_handler)
signal.signal(signal.SIGINT, signal_handler)



class RawAndStaticHandler(SimpleHTTPRequestHandler):
    def __init__(
        self,
        *args,
        core_file: Path,
        settings_file: Path,
        **kwargs,
    ) -> None:
        self.core_file = core_file
        self.settings_file = settings_file
        super().__init__(*args, **kwargs)

    def do_GET(self) -> None:
        route = urlparse(self.path).path
        if route.upper() == "/CORENAME":
            self._serve_text_file(self.core_file, include_body=True)
            return
        if route.upper() == "/SETTINGS":
            self._serve_text_file(self.settings_file, include_body=True)
            return
        super().do_GET()

    def do_HEAD(self) -> None:
        route = urlparse(self.path).path
        if route.upper() == "/CORENAME":
            self._serve_text_file(self.core_file, include_body=False)
            return
        if route.upper() == "/SETTINGS":
            self._serve_text_file(self.settings_file, include_body=False)
            return
        super().do_HEAD()

    def end_headers(self) -> None:
        self.send_header("Cache-Control", "no-store")
        super().end_headers()

    def _serve_text_file(self, target: Path, include_body: bool) -> None:
        try:
            payload = target.read_text(encoding="utf-8", errors="replace")
            status_code = 200
        except FileNotFoundError:
            payload = f"{target} was not found.\n"
            status_code = 404
        except OSError as exc:
            payload = f"Unable to read {target}: {exc}\n"
            status_code = 500

        encoded_payload = payload.encode("utf-8")
        self.send_response(status_code)
        self.send_header("Content-Type", "text/plain; charset=utf-8")
        self.send_header("Content-Length", str(len(encoded_payload)))
        self.end_headers()
        if include_body:
            self.wfile.write(encoded_payload)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Serve /tmp/CORENAME and /tmp/SETTINGS as raw text pages and "
            "share files from /media/fat/tty2waveshare/jpg."
        )
    )
    parser.add_argument("--port", type=int, default=DEFAULT_PORT, help="TCP port to bind.")
    parser.add_argument(
        "--corename-file",
        type=Path,
        default=DEFAULT_CORENAME_FILE,
        help="Path to the CORENAME source file.",
    )
    parser.add_argument(
        "--settings-file",
        type=Path,
        default=DEFAULT_SETTINGS_FILE,
        help="Path to the SETTINGS source file.",
    )
    parser.add_argument(
        "--static-root",
        type=Path,
        default=DEFAULT_STATIC_ROOT,
        help="Directory to share recursively for static files.",
    )
    parser.add_argument(
        "--mdns-name",
        default=DEFAULT_MDNS_NAME,
        help="mDNS service instance name for discovery.",
    )
    parser.add_argument(
        "--no-mdns",
        action="store_true",
        help="Disable mDNS advertisement.",
    )
    return parser.parse_args()


def _collect_ipv4_addresses() -> list[str]:
    addresses: set[str] = set()
    try:
        hostname_entries = socket.getaddrinfo(socket.gethostname(), None, socket.AF_INET)
    except socket.gaierror:
        hostname_entries = []

    for entry in hostname_entries:
        ip_address = entry[4][0]
        if not ip_address.startswith("127."):
            addresses.add(ip_address)

    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as probe_socket:
            probe_socket.connect(("8.8.8.8", 80))
            ip_address = probe_socket.getsockname()[0]
            if not ip_address.startswith("127."):
                addresses.add(ip_address)
    except OSError:
        pass

    return sorted(addresses)


@contextmanager
def advertise_mdns(port: int, instance_name: str) -> Iterator[str]:
    try:
        from zeroconf import IPVersion, ServiceInfo, Zeroconf
    except ImportError as exc:
        raise RuntimeError(
            "mDNS discovery requires the 'zeroconf' package. Install dependencies first."
        ) from exc

    safe_instance_name = instance_name.strip().rstrip(".") or DEFAULT_MDNS_NAME
    hostname_label = socket.gethostname().split(".", 1)[0] or DEFAULT_MDNS_NAME
    service_type = "_http._tcp.local."
    service_name = f"{safe_instance_name}.{service_type}"
    addresses = [socket.inet_aton(ip_address) for ip_address in _collect_ipv4_addresses()]

    if not addresses:
        raise RuntimeError(
            "mDNS discovery could not determine a non-loopback IPv4 address to advertise."
        )

    service_info = ServiceInfo(
        type_=service_type,
        name=service_name,
        addresses=addresses,
        port=port,
        properties={"path": "/"},
        server=f"{hostname_label}.local.",
    )

    zeroconf = Zeroconf(ip_version=IPVersion.V4Only)
    zeroconf.register_service(service_info)
    try:
        yield service_name
    finally:
        zeroconf.unregister_service(service_info)
        zeroconf.close()


def main() -> None:
    while running:
        args = parse_args()
        static_root = args.static_root.resolve(strict=False)
        handler = partial(
            RawAndStaticHandler,
            directory=str(static_root),
            core_file=args.corename_file,
            settings_file=args.settings_file,
        )

        try:
            with ThreadingHTTPServer(("0.0.0.0", args.port), handler) as server:
                print(f"Serving on http://0.0.0.0:{args.port}")
                print(f"Raw text endpoint: /CORENAME -> {args.corename_file}")
                print(f"Raw text endpoint: /SETTINGS -> {args.settings_file}")
                print(f"Static files root: {static_root}")

                if args.no_mdns:
                    print("mDNS advertisement: disabled")
                    server.serve_forever()
                    return

                with advertise_mdns(args.port, args.mdns_name) as service_name:
                    print(f"mDNS advertisement: {service_name}")
                    server.serve_forever()
        except RuntimeError as exc:
            raise SystemExit(str(exc))


if __name__ == "__main__":
    main()

