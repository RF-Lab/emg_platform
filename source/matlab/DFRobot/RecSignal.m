function sample=RecSignal
% ������� ��� ������ � ���� ���������
a=serial('COM5');
a.BaudRate=57600;
%a.BaudRate=115200;
fopen(a);
finishup = onCleanup(@() fclose(a));
sec=60;%����� ���������� � ��������
t=0;
fid = fopen('����_������_�_1���_����13.log', 'a');     % �������� ����� �� ������ 
myData=[];
tic
disp('-=START=-');
while (fix(t)~=sec)
    while ( a.BytesAvailable < 60 )
    pause(.01);
    end
    myData =[myData;fread(a,60)];
    t=toc;  
end
disp('-=STOP=-');
muData=uint8(myData);
fwrite(fid, myData);
fclose(fid);

end

