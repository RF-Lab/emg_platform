function [ answ ] = MFMN( binned_signal )
% Returns the mean amplitude spectrum in each bin calculated by FFT


Fs = 256; % Sampling frequency
L = length(binned_signal); % Number of samples
NFFT = 2^nextpow2(L); % Next power of 2 from length of y
f = Fs/2*linspace(0,1,NFFT);
    Fy = abs(fft(binned_signal,NFFT)/L);
    answ = sum(Fy.*f) / sum(Fy);

end
