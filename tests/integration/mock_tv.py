import socket
import struct
import threading
from http.server import HTTPServer, BaseHTTPRequestHandler

MCAST_GRP = '239.255.255.250'
MCAST_PORT = 1900
HTTP_PORT = 8001

DEVICE_XML = """<?xml version="1.0"?>
<root xmlns="urn:schemas-upnp-org:device-1-0">
  <device>
    <deviceType>urn:samsung.com:device:RemoteControlReceiver:1</deviceType>
    <friendlyName>Samsung AU7002 55 TV</friendlyName>
    <modelName>UA55AU7002KXXV</modelName>
  </device>
</root>
"""

class MetadataHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/xml')
        self.end_headers()
        self.wfile.write(DEVICE_XML.encode())

def run_http_server():
    server = HTTPServer(('0.0.0.0', HTTP_PORT), MetadataHandler)
    server.serve_forever()

def run_ssdp_server():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.bind(('', MCAST_PORT))
    mreq = struct.pack("4sl", socket.inet_aton(MCAST_GRP), socket.INADDR_ANY)
    sock.setsockopt(socket.IPPROTO_IP, socket.IP_ADD_MEMBERSHIP, mreq)

    while True:
        data, addr = sock.recvfrom(1024)
        if b"M-SEARCH" in data and b"RemoteControlReceiver:1" in data:
            hostname = socket.gethostname()
            ip_addr = socket.gethostbyname(hostname)
            response = (
                "HTTP/1.1 200 OK\r\n"
                f"LOCATION: http://{ip_addr}:{HTTP_PORT}/device.xml\r\n"
                "ST: urn:samsung.com:device:RemoteControlReceiver:1\r\n"
                "\r\n"
            ).encode()
            sock.sendto(response, addr)

if __name__ == "__main__":
    threading.Thread(target=run_http_server, daemon=True).start()
    run_ssdp_server()
