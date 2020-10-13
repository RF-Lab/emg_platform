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


AD1299_NUM_CH                           =   8
CHANNEL_TO_MONITOR                      =   5
TRANSPORT_BLOCK_HEADER_SIZE             =   16
PKT_COUNT_OFFSET                        =   2
SAMPLES_PER_TRANSPORT_BLOCK             =   128
TRANSPORT_QUE_SIZE                      =   4
TCP_SERVER_PORT                         =   3000
SPS                                     =   250
SAMPLES_TO_COLLECT                      =   SAMPLES_PER_TRANSPORT_BLOCK*2*30

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
                samples                 = unpack('1156i', receivedBuffer[startOfBlock:startOfBlock+TCP_PACKET_SIZE] )
               
                # remove block from received buffer
                receivedBuffer          = receivedBuffer[startOfBlock+TCP_PACKET_SIZE:] 
                
                # get channel offset
                offset_toch    =  int(TRANSPORT_BLOCK_HEADER_SIZE/4 + SAMPLES_PER_TRANSPORT_BLOCK*CHANNEL_TO_MONITOR) 
                
                #print( samples[offset_to4ch:offset_to4ch+SAMPLES_PER_TRANSPORT_BLOCK] )
                dataSamples = samples[offset_toch:offset_toch+SAMPLES_PER_TRANSPORT_BLOCK]
                
                blockSamples = np.array(dataSamples)
                print( 'Block #{0} mean:{1:10.1f},  var:{2:8.1f}, sec:{3:4.0f}'.format(samples[PKT_COUNT_OFFSET],np.mean(blockSamples),np.var(blockSamples)/1e6, numSamples/SPS ) )
                
                npSamples[numSamples:numSamples+SAMPLES_PER_TRANSPORT_BLOCK] = blockSamples
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
    
npRawSamples        = npSamples
 
# remove 50 Hz
#hflt        = signal.firls( 127, [0/125.0, 40/125.0, 44/125.0, 56/125.0, 60/125.0, 90/125.0, 95/125.0, 105/125.0, 110/125.0, 125/125.0],   [1.0, 1.0,  0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0 ])
hflt        = signal.firls( 151, [0, 40,  45, 55, 60, 90, 95, 105, 115, 125],   [1.0, 1.0,  0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0 ],fs = SPS)

#plt.figure(3)
#w, h = signal.freqz(hflt,fs=SPS)
#plt.plot(w, 20 * np.log10(abs(h)), 'b')

npFiltSamples       = np.convolve( hflt, npSamples, 'same' )
    
plt.figure(2)    
plt.clf()
frex,Pxx     = signal.welch( npFiltSamples, fs=SPS )
plt.semilogy( frex,Pxx )
plt.xlabel('Частота, Гц')
plt.title('Спектральная плотность мощности')
plt.grid('true')

plt.figure(1)    
plt.clf()
plt.plot(np.linspace(0,(SAMPLES_TO_COLLECT-1)/SPS,SAMPLES_TO_COLLECT),npFiltSamples)
plt.xlabel('Время, сек')
plt.ylabel('Напряжение, мкв')
plt.title('Канал №{}'.format(CHANNEL_TO_MONITOR))
plt.grid('true')
#plt.ylim((-7e6,7e6))


np.savetxt('emg_raw_data.txt',npRawSamples)
np.savetxt('emg_filt_data.txt',npFiltSamples)
#npSamples = np.loadtxt('emg_data.txt')
