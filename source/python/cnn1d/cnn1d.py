# -*- coding: utf-8 -*-
"""
Created on Tue Feb 12 00:12:05 2019

Based on:   Nils Ackermann Introduction to 1D Convolutional Neural Networks 
            in Keras for Time Sequences

@changes: 
    Melnikov Aleksey: initial 
"""

# Compatibility layer between Python 2 and Python 3
from __future__ import print_function
from matplotlib import pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns
from scipy import stats

from sklearn import metrics
from sklearn.metrics import classification_report
from sklearn import preprocessing

import keras
from keras.models import Sequential
from keras.layers import Dense, Dropout, Flatten, Reshape, GlobalAveragePooling1D
from keras.layers import Conv2D, MaxPooling2D, Conv1D, MaxPooling1D
from keras.utils import np_utils


print("\n--- Load data ---\n")
num_classes = 5
num_sensors = 1
d1 = pd.read_csv('1_data.txt',header=None)
d2 = pd.read_csv('2_data.txt',header=None)
d3 = pd.read_csv('3_data.txt',header=None)
d4 = pd.read_csv('4_data.txt',header=None)
d5 = pd.read_csv('5_data.txt',header=None)

INPUT_SIZE      = d1.shape[1]

print("\n--- Create neural network model ---\n")

# The number of steps within one time segment

# 1D CNN neural network
model_m = Sequential()
#model_m.add(Reshape((INPUT_SIZE, num_sensors), input_shape=(input_shape,)))
model_m.add(Conv1D(100, 10, activation='relu', input_shape=(INPUT_SIZE, num_sensors)))
model_m.add(Conv1D(100, 10, activation='relu'))
model_m.add(MaxPooling1D(3))
model_m.add(Conv1D(160, 10, activation='relu'))
model_m.add(Conv1D(160, 10, activation='relu'))
model_m.add(GlobalAveragePooling1D())
model_m.add(Dropout(0.5))
model_m.add(Dense(num_classes, activation='softmax'))
print(model_m.summary())

# %%

print("\n--- Fit the model ---\n")

# The EarlyStopping callback monitors training accuracy:
# if it fails to improve for two consecutive epochs,
# training stops early
callbacks_list = [
    keras.callbacks.ModelCheckpoint(
        filepath='best_model.{epoch:02d}-{val_loss:.2f}.h5',
        monitor='val_loss', save_best_only=True),
    keras.callbacks.EarlyStopping(monitor='acc', patience=1)
]

model_m.compile(loss='categorical_crossentropy',
                optimizer='adam', metrics=['accuracy'])


# inputs
x_train = np.vstack((d1.values,d2.values,d3.values, d4.values,d5.values))
# desired output
y1      = np.zeros( (d1.shape[0], num_classes) )
y1[:,0] = 1
y2      = np.zeros( (d2.shape[0], num_classes) )
y2[:,1] = 1
y3      = np.zeros( (d3.shape[0], num_classes) )
y3[:,2] = 1
y4      = np.zeros( (d4.shape[0], num_classes) )
y4[:,3] = 1
y5      = np.zeros( (d5.shape[0], num_classes) )
y5[:,4] = 1

y_train = np.vstack( (y1,y2,y3,y4,y5) )

# Hyper-parameters
BATCH_SIZE = 40
EPOCHS = 50

# Enable validation to use ModelCheckpoint and EarlyStopping callbacks.
history = model_m.fit(x_train,
                      y_train,
                      batch_size=BATCH_SIZE,
                      epochs=EPOCHS,
                      callbacks=callbacks_list,
                      validation_split=0.2,
                      verbose=1)

