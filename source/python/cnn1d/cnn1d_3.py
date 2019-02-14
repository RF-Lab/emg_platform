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


def show_confusion_matrix(validations, predictions):

    matrix = metrics.confusion_matrix(validations, predictions)
    plt.figure(figsize=(6, 4))
    sns.heatmap(matrix,
                cmap="coolwarm",
                linecolor='white',
                linewidths=1,
                xticklabels=LABELS,
                yticklabels=LABELS,
                annot=True,
                fmt="d")
    plt.title("Confusion Matrix")
    plt.ylabel("True Label")
    plt.xlabel("Predicted Label")
    plt.show()


print("\n--- Load data ---\n")
num_classes = 3
num_sensors = 1

d1 = np.loadtxt('1_data.txt' )
d2 = np.loadtxt('2_data.txt' )
d3 = np.loadtxt('3_data.txt' )
d4 = np.loadtxt('4_data.txt' )
d5 = np.loadtxt('5_data.txt' )

# inputs
#x_train = np.vstack((d1,d2,d3, d4,d5))
x_train = np.vstack((d1[0:60,:],d2[0:60,:],d5[0:60,:]))
x_test  = np.vstack((d1[59:,:], d2[59:,:], d5[59:,:]))

# desired output
y1      = np.zeros( (60, num_classes) )
y1[:,0] = 1
y2      = np.zeros( (60, num_classes) )
y2[:,1] = 1
y3      = np.zeros( (60, num_classes) )
y3[:,2] = 1
#y4      = np.zeros( (d4.shape[0], num_classes) )
#y4[:,3] = 1
#y5      = np.zeros( (d5.shape[0], num_classes) )
#y5[:,4] = 1

#y_train = np.vstack( (y1,y2,y3,y4,y5) )
y_train = np.vstack( (y1,y2,y3) )

INPUT_SIZE      = d1.shape[1]

print("\n--- Create neural network model ---\n")

# The number of steps within one time segment

# 1D CNN neural network
model_m = Sequential()
model_m.add(Reshape((INPUT_SIZE, num_sensors), input_shape=(INPUT_SIZE,)))
model_m.add(Conv1D(25, 80, activation='relu', input_shape=(INPUT_SIZE,num_sensors)))
model_m.add(Conv1D(25, 6, activation='relu'))
model_m.add(MaxPooling1D(3))
model_m.add(Conv1D(16, 6, activation='relu'))
model_m.add(Conv1D(12, 4, activation='relu'))
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



#x_train = x_train.astype("float32")


#y_train = np_utils.to_categorical(y_train, num_classes)
print('New y_train shape: ', y_train.shape)
print('x_train shape:', x_train.shape)
# x_train shape: (20869, 120)
print('input_shape:', INPUT_SIZE)
# input_shape: (120)


# Hyper-parameters
BATCH_SIZE = 16
EPOCHS = 10

# Enable validation to use ModelCheckpoint and EarlyStopping callbacks.
history = model_m.fit(x_train,
                      y_train,
                      batch_size=BATCH_SIZE,
                      epochs=EPOCHS,
                      callbacks=callbacks_list,
                      validation_split=0.2,
                      verbose=1)

print("\n--- Learning curve of model training ---\n")

# summarize history for accuracy and loss
plt.figure(figsize=(6, 4))
plt.plot(history.history['acc'], "g--", label="Accuracy of training data")
plt.plot(history.history['val_acc'], "g", label="Accuracy of validation data")
plt.plot(history.history['loss'], "r--", label="Loss of training data")
plt.plot(history.history['val_loss'], "r", label="Loss of validation data")
plt.title('Model Accuracy and Loss')
plt.ylabel('Accuracy and Loss')
plt.xlabel('Training Epoch')
plt.ylim(0)
plt.legend()
plt.show()

print("\n--- Check against test data ---\n")

# Normalize features for training data set

#%%

#%%

# desired output
y1      = np.zeros( (20, num_classes) )
y1[:,0] = 1
y2      = np.zeros( (20, num_classes) )
y2[:,1] = 1
y3      = np.zeros( (20, num_classes) )
y3[:,2] = 1
#y4      = np.zeros( (d4.shape[0], num_classes) )
#y4[:,3] = 1
#y5      = np.zeros( (d5.shape[0], num_classes) )
#y5[:,4] = 1

#y_train = np.vstack( (y1,y2,y3,y4,y5) )
y_test = np.vstack( (y1,y2,y3) )


score = model_m.evaluate(x_test, y_test, verbose=1)

print("\nAccuracy on test data: %0.2f" % score[1])
print("\nLoss on test data: %0.2f" % score[0])

# %%

print("\n--- Confusion matrix for test data ---\n")

y_pred_test = model_m.predict(x_test)
# Take the class with the highest probability from the test predictions
max_y_pred_test = np.argmax(y_pred_test, axis=1)
max_y_test = np.argmax(y_test, axis=1)

show_confusion_matrix(max_y_test, max_y_pred_test)

# %%

print("\n--- Classification report for test data ---\n")

print(classification_report(max_y_test, max_y_pred_test))