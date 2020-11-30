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

EMG8x_ADDRESS                           = '192.168.1.42' ;
CHANNELS_TO_MONITOR                     =  (6,7,8)


AD1299_NUM_CH                           =   8
TRANSPORT_BLOCK_HEADER_SIZE             =   16
PKT_COUNT_OFFSET                        =   2
SAMPLES_PER_TRANSPORT_BLOCK             =   128
TRANSPORT_QUE_SIZE                      =   4
TCP_SERVER_PORT                         =   3000
SPS                                     =   500
SAMPLES_TO_COLLECT                      =   SAMPLES_PER_TRANSPORT_BLOCK*4*10

TCP_PACKET_SIZE                         = int(((TRANSPORT_BLOCK_HEADER_SIZE)/4+(AD1299_NUM_CH+1)*(SAMPLES_PER_TRANSPORT_BLOCK))*4)


# Create a TCP/IP socket
sock                = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Connect the socket to the port where the server is listening
server_address      = (EMG8x_ADDRESS, TCP_SERVER_PORT)


sock.connect(server_address)

receivedBuffer                          = bytes()

rawSamples                              = np.zeros((SAMPLES_TO_COLLECT,len(CHANNELS_TO_MONITOR)))
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
                
                chCount = 0
                for chIdx in CHANNELS_TO_MONITOR:
                                    
                    # get channel offset
                    offset_toch    =  int(TRANSPORT_BLOCK_HEADER_SIZE/4 + SAMPLES_PER_TRANSPORT_BLOCK*chIdx) 
                
                    #print( samples[offset_to4ch:offset_to4ch+SAMPLES_PER_TRANSPORT_BLOCK] )
                    dataSamples = samples[offset_toch:offset_toch+SAMPLES_PER_TRANSPORT_BLOCK]
                
                    blockSamples = np.array(dataSamples)
                    print( 'Ch#{0} Block #{1} mean:{2:10.1f},  var:{3:8.1f}, sec:{4:4.0f}'.format(chIdx, samples[PKT_COUNT_OFFSET],np.mean(blockSamples),np.var(blockSamples)/1e6, numSamples/SPS ) )
                
                    rawSamples[numSamples:numSamples+SAMPLES_PER_TRANSPORT_BLOCK,chCount] = blockSamples
                    
                    chCount += 1
                
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
    
# remove 50 Hz
#hflt        = signal.firls( 127, [0/125.0, 40/125.0, 44/125.0, 56/125.0, 60/125.0, 90/125.0, 95/125.0, 105/125.0, 110/125.0, 125/125.0],   [1.0, 1.0,  0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0 ])
hflt        = signal.firls( 257, [ 0,10,  15,40,  43,57,  60,95,  98,104,  107,143, 146,155, 158,193, 196,205, 208,230, 233,SPS/2 ],   
                           [     0.,0., 1.0,1.0, .0,.0, 1.0,1.0, 0.0,0.0, 1.0,1.0,  0.0,0.0, 1.0,1.0, .0,.0,   1.0,1.0, .0,.0  ],fs = SPS)

#plt.figure(3)
#w, h = signal.freqz(hflt,fs=SPS)
#plt.plot(w, 20 * np.log10(abs(h)), 'b')

filtSamples     = rawSamples * 0

for chCount in range(len(CHANNELS_TO_MONITOR)):
    filtSamples[:,chCount]      = np.convolve( hflt, rawSamples[:,chCount], 'same' )

side_len        = 500     
plt.figure(1)    
plt.clf()
for chCount in range(len(CHANNELS_TO_MONITOR)):
    plt.subplot(len(CHANNELS_TO_MONITOR),1,chCount+1)
#    plt.plot(np.linspace(0,(SAMPLES_TO_COLLECT-2*side_len-1)/SPS,SAMPLES_TO_COLLECT-2*side_len), 
#                 rawSamples[side_len:-side_len,chCount])
    plt.plot(np.linspace(0,(SAMPLES_TO_COLLECT-2*side_len-1)/SPS,SAMPLES_TO_COLLECT-2*side_len), 
                 filtSamples[side_len:-side_len,chCount])
    plt.xlabel('Время, сек')
    plt.ylabel('Напряжение, мкв')
    plt.title('Канал {}'.format(CHANNELS_TO_MONITOR[chCount]))
    plt.grid('true')


plt.figure(2)    
plt.clf()
for chCount in range(len(CHANNELS_TO_MONITOR)):
    #plt.subplot(len(CHANNELS_TO_MONITOR),1,chCount+1)
    frex,Pxx     = signal.welch( rawSamples[side_len:-side_len,chCount], fs=SPS )
    plt.semilogy( frex,Pxx )
    frex,Pxx     = signal.welch( filtSamples[side_len:-side_len,chCount], fs=SPS )
    plt.semilogy( frex,Pxx )
    plt.xlabel('Частота, Гц')
    plt.title('Спектральная плотность мощности')
    plt.grid('true')


np.savetxt('emg_raw_snapping.txt',rawSamples )
np.savetxt('emg_filt_snapping.txt',filtSamples )
