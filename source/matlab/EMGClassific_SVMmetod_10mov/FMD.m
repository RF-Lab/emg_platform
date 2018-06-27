function [ answ ] = FMD( binned_signal )
% Returns the median PSD in each bin


    answ = (1/2)*sum(pwelch(binned_signal));

end

