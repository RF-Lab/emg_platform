clc, clear ;
t = tcpclient( '192.168.4.1', 8080) ;
while t.BytesAvailable>=17
    data = read(t,17) ;
    fprintf( '%d\n', data(4)) ;
end
clear t ;