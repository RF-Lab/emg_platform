clc, clear ;
% ESP board address
ip_addr = '192.168.4.1' ;
tcp_port = 8080 ;
t = tcpclient( ip_addr, tcp_port ) ;
while t.BytesAvailable>=17
    data = read(t,17) ; % read packet
    if data(1)~=hex2dec('A5')
        break ;
    end
    if data(2)~=hex2dec('5A')
        break ;
    end
    % print packet counter
    fprintf( '%d\n', data(4)) ;
end
clear t ; % close connection
