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
import numpy as np
from scipy import signal

EMG8x_ADDRESS                           = '192.168.1.65' ;


AD1299_NUM_CH                           =8
TRANSPORT_BLOCK_HEADER_SIZE             =16
PKT_COUNT_OFFSET                        =2
SAMPLES_PER_TRANSPORT_BLOCK             =256
TRANSPORT_QUE_SIZE                      =2
TCP_SERVER_PORT                         =3000
SAMPLES_TO_COLLECT                      = 4096

TCP_PACKET_SIZE                         = int(((TRANSPORT_BLOCK_HEADER_SIZE)/4+(AD1299_NUM_CH+1)*(SAMPLES_PER_TRANSPORT_BLOCK))*4)


# Create a TCP/IP socket
sock                = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Connect the socket to the port where the server is listening
server_address      = (EMG8x_ADDRESS, TCP_SERVER_PORT)


sock.connect(server_address)

receivedBuffer                          = bytes()

npSamples                               = np.zeros((SAMPLES_TO_COLLECT,))
# Collected samples
numSamples                              = 0 

try:
    while True:
        
        if len(receivedBuffer)>=TCP_PACKET_SIZE*2:
            
            # find sync bytes
            startOfBlock = receivedBuffer.find('EMG8x'.encode())
            
            if startOfBlock>=0:
        
                # SAMPLES_PER_TRANSPORT_BLOCK*(AD1299_NUM_CH+1)+TRANSPORT_BLOCK_HEADER_SIZE/4
                samples                 = unpack('2308i', receivedBuffer[startOfBlock:startOfBlock+TCP_PACKET_SIZE] )
                
                print( 'Received block #{}'.format(samples[PKT_COUNT_OFFSET]) )
                
                # remove block from received buffer
                receivedBuffer          = receivedBuffer[startOfBlock+TCP_PACKET_SIZE:] 
                
                # get channel 4 samples
                offset_to4ch    =  int(TRANSPORT_BLOCK_HEADER_SIZE/4 + SAMPLES_PER_TRANSPORT_BLOCK*4) 
                
                #print( samples[offset_to4ch:offset_to4ch+SAMPLES_PER_TRANSPORT_BLOCK] )
                dataSamples = samples[offset_to4ch:offset_to4ch+SAMPLES_PER_TRANSPORT_BLOCK]
                
                npSamples[numSamples:numSamples+SAMPLES_PER_TRANSPORT_BLOCK] = np.array(dataSamples)
                numSamples += SAMPLES_PER_TRANSPORT_BLOCK
                if numSamples>=SAMPLES_TO_COLLECT:
                    break 
        else:
            receivedData                    = sock.recv( TCP_PACKET_SIZE )
            if not receivedData:
                # probably connection closed
                break
            
            receivedBuffer += receivedData ;
        
           
finally:
    sock.close()
    
# remove corrupted samples
# (not clear)
npSamples[abs(npSamples)>100000] = 0

plt.figure(1)    
plt.plot(npSamples)
plt.xlabel('Время, сек')
plt.title('Канал №4')
plt.grid('true')

plt.figure(2)    
frex,Pxx     = signal.welch( npSamples, fs=1024 )
plt.semilogy( frex,Pxx )
plt.xlabel('Частота, Гц')
plt.title('Спектральная плотность мощности')
plt.grid('true')
