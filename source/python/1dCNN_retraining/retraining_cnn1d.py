import os
os.environ["CUDA_VISIBLE_DEVICES"]="0" #for training on gpu

import tensorflow as tf
from tensorflow import keras
import numpy as np
import pickle
import time
from tensorflow.python.keras.utils import to_categorical
from tensorflow.python.keras.models import load_model
from random import shuffle


#path to data files
path = "C:/Users/Юрий/Desktop/data_models/data/"

#path where you want to save retrained model and some other files
sec_path = "C:/Users/Юрий/Desktop/data_models/models/end/retrained_model/"

def create_dataset(file_path, persons):
    path = file_path + "{}_{}.txt"
    sgn = []
    lbl = []
    for i in persons:
        for j in range(9):
            with open(path.format(i, j + 1), "rb") as fp:  # Unpickling
                data = pickle.load(fp)

            for k in range(np.shape(data)[0]):
                sgn.append(data[k])
                lbl.append(j)

    sgn = np.asarray(sgn, dtype=np.float32)
    lbl = np.asarray(lbl, dtype=np.int32)

    c = list(zip(sgn, lbl))
    shuffle(c)
    sgn, lbl = zip(*c)

    sgn = np.asarray(sgn, dtype=np.float64)
    lbl = np.asarray(lbl, dtype=np.int64)

    train_signals = sgn[0:int(0.8 * len(sgn))]
    train_labels = lbl[0:int(0.8 * len(lbl))]
    val_signals = sgn[int(0.6*len(sgn)):int(0.8*len(sgn))]
    val_labels = lbl[int(0.6*len(lbl)):int(0.8*len(lbl))]
    test_signals = sgn[int(0.8*len(sgn)):]
    test_labels = lbl[int(0.8*len(lbl)):]

    train_labels = to_categorical(train_labels)
    val_labels = to_categorical(val_labels)
    test_labels = to_categorical(test_labels)

    return train_signals, train_labels, val_signals, val_labels, test_signals, test_labels

#path to pretrained model
new_model = load_model("C:/Users/Юрий/Desktop/data_models/models/end/pretrained_model/pretrained_model.h5")

#only last 3 layers are trainable now
for layer in new_model.layers[0:-3]:
    layer.trainable = False

#if you want to check trainable layers
# for layer in new_model.layers:
#     print(layer, layer.trainable)

#training new model for a new person
a = [3]
train_signals, train_labels, val_signals, val_labels, test_signals, test_labels = create_dataset(path, a)

num_classes = 9
num_sensors = 1
input_size = train_signals.shape[1]

new_model.compile(loss='categorical_crossentropy',
                optimizer='adam', metrics=['accuracy'])

start_time = time.time()
history = new_model.fit(train_signals, train_labels,
                      steps_per_epoch=5,
                      epochs=25,
                      batch_size=None,
                      validation_data=(val_signals, val_labels),
                      validation_steps=5
)

elapsed_time = time.time() - start_time # training time

loss, accuracy = new_model.evaluate(test_signals, test_labels) # evaluating model on test data

loss = float("{0:.3f}".format(loss))
accuracy = float("{0:.3f}".format(accuracy))
elapsed_time = float("{0:.3f}".format(elapsed_time))

#saving some data
f = open(sec_path + "info.txt", 'w')
f.writelines(["loss: ", str(loss), '\n', "accuracy: ", str(accuracy), '\n', "elapsed_time: ", str(elapsed_time), '\n'])

#saving model
new_model.save(sec_path + "retrained_model.h5")

#saving test data just in case
cc = list(zip(test_signals, test_labels))
with open(sec_path + "retrained_model_test_data.txt", "wb") as fp:
    pickle.dump(cc, fp)

#saving history
with open(sec_path + "retrained_model_history.h5", "wb") as fp:
    pickle.dump(history.history, fp)