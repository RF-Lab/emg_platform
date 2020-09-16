import numpy as np
import matplotlib.pyplot as plt
data = np.zeros((50000,6))
idx = 0
with open("сжим_н10_30sec.log", "rb") as f:
  byte = np.fromfile(f, dtype=np.uint8, count=1)
  while byte:
    if (byte == 0xa5):
      byte = np.fromfile(f, dtype=np.uint8, count=1)
      if (byte == 0x5a):
        ver = np.fromfile(f, dtype=np.uint8, count=1)
        cnt = np.fromfile(f, dtype=np.uint8, count=1)
        val = np.fromfile(f, dtype='>u2', count=6)
        if val.shape==(6,):
          data[idx,:] = val
          idx = idx + 1
        else:
          break
        sta = np.fromfile(f, dtype=np.uint8, count=1)
    byte = np.fromfile(f, dtype=np.uint8, count=1)
print(idx)
plt.plot( data[0:idx,:] )
