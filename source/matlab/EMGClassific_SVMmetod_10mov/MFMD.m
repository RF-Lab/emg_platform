function [ answ ] = MFMD( binned_signal )
% Returns the median amplitude spectrum in each bin calculated from fft


L = length(binned_signal); % Number of samples
NFFT = 2^nextpow2(L); % Next power of 2 from length of y

    answ = (1/2)*sum(abs(fft(binned_signal,NFFT)/L));

end

