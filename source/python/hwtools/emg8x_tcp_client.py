"""
Created on Wed Sep 16 13:50:46 2020
//
// Copyright 2019-2020 rf-lab.org 
// (MIREA KB-2( frmly. MIREA KB-3, MGUPI IT-6 "Control and simulation in technical systems"))
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// emg8x_tcp_client.py.c - simple TCP client example for dataretrieving from
                            EMG8x board 
// Please refer to:

// EMG platform
//      https://github.com/RF-Lab/emg_platform

// List of modifications:
//
# -*- coding: utf-8 -*-
"""


import socket
import sys
from struct import *
import matplotlib.pyplot as plt

EMG8x_ADDRESS                           = '192.168.1.65' ;


AD1299_NUM_CH                           =8
TRANSPORT_BLOCK_HEADER_SIZE             =16
PKT_COUNT_OFFSET                        =2
SAMPLES_PER_TRANSPORT_BLOCK             =64
TRANSPORT_QUE_SIZE                      =2
TCP_SERVER_PORT                         =3000

TCP_PACKET_SIZE                         = int(((TRANSPORT_BLOCK_HEADER_SIZE)/4+(AD1299_NUM_CH+1)*(SAMPLES_PER_TRANSPORT_BLOCK))*4)


# Create a TCP/IP socket
sock                = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Connect the socket to the port where the server is listening
server_address      = (EMG8x_ADDRESS, TCP_SERVER_PORT)

sock.connect(server_address)

try:
    while True:
        data = sock.recv(TCP_PACKET_SIZE)
        if not data:
            break 
        samples = unpack('580i', data )
        
        # get channel 4 samples
        offset_to4ch    =  int(TRANSPORT_BLOCK_HEADER_SIZE/4 + SAMPLES_PER_TRANSPORT_BLOCK*4) 
        
        #print( samples[offset_to4ch:offset_to4ch+SAMPLES_PER_TRANSPORT_BLOCK] )
        plt.plot(samples[offset_to4ch:offset_to4ch+SAMPLES_PER_TRANSPORT_BLOCK])
        plt.show
        
finally:
    sock.close()