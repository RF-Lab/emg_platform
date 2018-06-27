function [ answ ] = FR( binned_signal )
% Frequency Ratio
%
% Returns the ratio of the lowest frequency in a bin to the highest
% frequency in the bin

L = length(binned_signal); % Number of samples
NFFT = 2^nextpow2(L); % Next power of 2 from length of y
    Fy = abs(fft(binned_signal,NFFT)/L);
    answ = min(Fy)/max(Fy);

end

