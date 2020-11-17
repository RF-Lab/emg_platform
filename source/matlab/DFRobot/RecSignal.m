function sample=RecSignal
% Функция для записи в файл измерений
a=serial('COM5');
a.BaudRate=57600;
%a.BaudRate=115200;
fopen(a);
finishup = onCleanup(@() fclose(a));
sec=60;%время выполнения в секундах
t=0;
fid = fopen('щелч_Андрей_Л_1мин_день13.log', 'a');     % открытие файла на запись 
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

