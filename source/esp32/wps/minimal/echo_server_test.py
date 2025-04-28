#! /usr/bin/env python3
import socket
from time import sleep


def main():
    interfaces = socket.getaddrinfo(
        host=socket.gethostname(), port=None, family=socket.AF_INET
    )
    allips = [ip[-1][0] for ip in interfaces]

    msg = b"EMG"

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)  # UDP
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    # sock.bind((ip,0))
    sock.sendto(msg, ("192.168.255.255", 33333))
    data, addr = sock.recvfrom(1024)
    print(data, addr)
    sock.close()


main()
