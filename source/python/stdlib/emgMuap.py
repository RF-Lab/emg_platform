# -*- coding: utf-8 -*-
"""
Created on Tue Mar  9 18:11:59 2021

Task: detect Muap impulse

@author: alekseymelnikov
"""

import numpy as np
import matplotlib.pyplot as plt

rawSamples = np.loadtxt('C:/emg_data_/Шаршатов/emg_raw.txt' )

rawSamples = rawSamples - np.mean(rawSamples) 

N           = len( rawSamples )

wnd_size    = 500
muap_left   = 300
muap_right   = 600

labels      = rawSamples*0
borders     = rawSamples*0

muaps       = np.zeros((muap_left+muap_right,500))

numMuaps    = 0
for k in range(0,N):
    
    # берем кусочек сигнала в текущей позиции окна
    xWnd        = np.abs(rawSamples[ k:k+wnd_size ])
    maxValue    = np.max(xWnd)
    maxIndices  = np.argmax(xWnd)
    if (maxIndices==wnd_size//2) and (maxValue>3800) :
        labels[k+wnd_size//2] = 1
        borders[k+wnd_size//2-muap_left] = 1
        borders[k+wnd_size//2+muap_right] = 1
        muaps[:,numMuaps] = rawSamples[ k:k+muap_left+muap_right ]
        numMuaps    = numMuaps + 1
        
muaps = muaps[:,:numMuaps]

muaps_std = np.var( muaps, axis=0 )


plt.figure(1)
plt.clf()
plt.plot(rawSamples)
plt.plot(labels*5000)
plt.plot(borders*5000)
plt.grid(True)
        
plt.figure(2)
plt.plot(muaps[:,10])

hist, bin_edges = np.histogram(muaps_std, density=False, bins=50)
hist = hist/np.sum(hist)

plt.plot( bin_edges[0:51], hist )

plt.plot(b)

