f = fopen('teraterm_1.log','r') ;
N = 1000 ;
numChannels = 6 ;
data = zeros(N,numChannels) ;
numPacket = 1 ;
while(~feof(f))
  sync1 = fread(f,1,'uint8') ;
  if sync1~=hex2dec('A5')
      continue ;
  end
  sync2 = fread(f,1,'uint8') ;
  if sync2~=hex2dec('5A')
      continue ;
  end  
  pktVersion =fread(f,1,'uint8') ;
  if pktVersion~=2
      continue ;
  end 
  pktCount =fread(f,1,'uint8') ;
  %fprintf('Packet counter:%d\n',pktCount) ;
  formatError = 0 ;
  for nChannel =1:numChannels
      if feof(f)
          break ;
      end
      value = fread(f,1,'uint16','ieee-be') ;
      if value<1024
          val10 = value ;
      else
          fprintf('Error:\n%4X %d %d\n', value, value, val10) ;
          formatError = 1 ;
          break ;
      end
      %if value>511
      %    val10 = value - 1024 ; 
      %end
      %if val10>511 || val10<-512
      %    fprintf('Error:\n%4X %d %d\n', value, value, val10) ;
      %    formatError = 1 ;
      %    break ;
      %end
      data(numPacket, nChannel) = val10 ;
  end
  if formatError
      continue ;
  end
  if feof(f)
      break ;
  end
  numPacket = numPacket + 1 ;
  channelState =fread(f,1,'uint8') ;
end
data = data(1:numPacket-1,:) ;
data_m = mean(data.').';
fclose(f) ;
%plot(data_m(1:1:end,6)) ;
plot(data_m);
ylim ([-10 1100])
