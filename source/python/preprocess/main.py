import numpy as np
from scipy import signal
import plotly.graph_objects as go
import plotly.io as pio
pio.templates.default = 'presentation'

class emgTextFile:
    def __init__(self):
        self.data = []
    def __call__(self,fName):
        self.data = np.loadtxt( fName ).T
        return self.data

class emgFilter:
    def __init__(self):
        self.data = []
        self.flt = signal.firwin( 256, [10,45, 55,450], pass_zero=False, fs=1000 )
    def __call__(self,data):
        self.data = np.zeros_like(data)
        for k in range(self.data.shape[0]):
            self.data[k,:] = np.convolve( self.flt, data[k,:]-np.mean(data[k,:]),'same')
        return self.data
    def plot_freqz(self):
        f, h = signal.freqz(self.flt,fs=1000)
        fig = go.Figure()
        fig.add_scatter(x=f,y=10*np.log10(np.square(np.abs(h))),name='Полосовой КИХ фильтр с подавлением 50 Гц',mode='lines')
        fig.update_layout(title='АЧХ фильтра')
        fig.write_html('fir.html')
    
class emgSideRemoval:
    def __init__(self,sideLen):
        self.data = []
        self.sideLen = sideLen
    def __call__(self,data):
        self.data = data.copy()[:,self.sideLen:-self.sideLen]
        return self.data

class emgMvc:
    def __init__(self):
        self.data = []
    def __call__(self,data):
        self.data = np.zeros_like(data)
        for k in range(self.data.shape[0]):
            mvc = np.max(np.abs(data[k,:]))
            self.data[k,:] = data[k,:]/mvc
        return self.data
    
# class emgOnset:
#     def __init__(self,th):
#         self.data = []
#         self.th = th
#         Fs = 1000
#         self.envFlt = signal.iirfilter(N=5, Wn=[30,], rp=None, rs=None, btype='lowpass', analog=False, ftype='butter', output='ba', fs=Fs)
#         f,gd = signal.group_delay( self.envFlt, fs=Fs )
#         self.fltDelay = np.round(np.mean( gd[f<30] ))

#     def calcEnvelope(self,data):
#         # rectifier then mean over all the channels
#         rect = np.mean(np.abs( data ),axis=0)
#         # apply lpf
#         env = signal.lfilter(self.envFlt[0],self.envFlt[1],rect)
#         self.env = np.roll(env,-int(self.fltDelay))

#     def __call__(self,data):
#         self.calcEnvelope(data)
#         self.envDet = np.zeros_like(self.env)
#         self.envDet[self.env>self.th] = 1
#         self.envDetEdges = self.envDet-np.roll(self.envDet,1)
#         D = 100
#         for n in range(len(self.envDet)):
#             if self.envDetEdges[n]==1:
#                 self.envDet[n-D:n] = 1
#             elif self.envDetEdges[n]==-1:
#                 self.envDet[n:n+D] = 1

#         for n in range(len(envDet)):
#         if envDetEdges[n]==1:
#             indices = np.where( envDetEdges[n:]==-1 )
#             print(indices[0])
#             if indices[0].size!=0:
#             offset = indices[0][0]+n
#             # Диапазон индексов ПД n..indices[0]
#             lengthAp = offset-n
#             if lengthAp<maxLength:
#                 aps.append(emgSignal[n-(maxLength-lengthAp)//2:n-(maxLength-lengthAp)//2+maxLength])
#                 fig.add_scatter(x=t,y=aps[-1],name='Сигнал ЭМГ')


    
class emgPipeline:
    def __init__(self,operations):
        self.operations = operations
    def __call__(self,data):
        for op in self.operations:
            data = op(data)
        self.data = data


pp = emgPipeline( [emgTextFile(),emgFilter(),emgSideRemoval(100),emgMvc()] )
pp(r'C:\emg_data\2024\Nastya\dataset\0\emg_raw_0_0.txt')

fig = go.Figure()
fig.add_scatter(y=pp.data[0,:])
fig.add_scatter(y=pp.data[1,:])
fig.write_html('signals.html')

fig = go.Figure()
fig.add_scatter(y=pp.operations[0].data[0,:],name='Сигнал до фильтрации')
fig.add_scatter(y=pp.operations[1].data[0,:],name='Сигнал после фильтрации')
fig.update_layout( xaxis_title=r'Номер отсчета', yaxis_title=r'Амплитуда' )
fig.write_html('filter_signals.html')

