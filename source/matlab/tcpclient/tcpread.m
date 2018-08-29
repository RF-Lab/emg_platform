clc, clear ;
t = tcpclient( '192.168.4.1', 8080) ;
while t.BytesAvailable>=17
    data = read(t,17) ;
    if data(1)~=17
        break ;
    end
    fprintf( '%d\n', data(4)) ;
end
clear t ;
