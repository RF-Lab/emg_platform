function sample=RecSignal
% ������� ��� ������ � ���� ���������
a=serial('COM4');
a.BaudRate=57600;
fopen(a);
finishup = onCleanup(@() fclose(a));
sec=5;%����� ���������� � ��������
t=0;
fid = fopen('�������1.log', 'a');     % �������� ����� �� ������ 
myData=[];
tic
disp('-=START=-');
while (fix(t)~=sec)
    while ( a.BytesAvailable < 40 )
    pause(.01);
    end
    myData =[myData;fread(a,40)];
    t=toc;  
end
disp('-=STOP=-');
muData=uint8(myData);
fwrite(fid, myData);
fclose(fid);

end

