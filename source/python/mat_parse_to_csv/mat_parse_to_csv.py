#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Sat Mar 21 12:10:05 2020

@author: leonidkotov
"""

import scipy.io
import csv
import numpy as np


# Полный путь до файла Андрюхи
mat_path = '/Users/leonidkotov/Desktop/МИРЭА/Future_all.mat'

# Вставлять заголовки стобцов?
with_headers = True

# Заголовки столбцов
headers = ["personId", "classId", "testNumber", "futureValue"]


if __name__ == "__main__":
    # Читаем файл матлаба
    mat = scipy.io.loadmat(mat_path)
    
    # Достаём данные из файла и начинаем их парсить
    P_all = mat['P_all']
    
    data = []
    if with_headers:
        data.append(headers)
    
    personId = 0
    classId = 0
    testId = 0
    for person_object in P_all:
        for person in person_object: 
            for class_future_object in person:
                for class_future in class_future_object:                    
                    for test_number in np.array(class_future).transpose():
                        for futureValue in test_number:
                            data.append([personId, classId, testId, futureValue])
                        testId += 1
                    testId = 0
                classId += 1
        personId += 1
        classId = 0
    
    with open('Future_all.csv', 'w', newline='') as f:
        writer = csv.writer(f, delimiter=",")
        writer.writerows(data)
        
    print("Successfully completed!")