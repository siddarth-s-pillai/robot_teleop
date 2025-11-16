import socket

UDP_IP = "0.0.0.0"        # Empty string means listen on all network interfaces
UDP_PORT = 20025    # Must match the port used by the ESP8266

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

print(f"Listening for UDP packets on port {UDP_PORT}...")

while True:
    data, addr = sock.recvfrom(1024)  # Buffer size is 1024 bytes
    print(f"Received packet from {addr}: {data.decode('utf-8')}")