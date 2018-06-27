function [ answ ] = FMN( binned_signal )
% Returns the mean PSD in each bin

[R C] = size(binned_signal);

    [Pxx, W] = pwelch(binned_signal);
    answ = (sum(W.*Pxx))/(sum(Pxx));

end



