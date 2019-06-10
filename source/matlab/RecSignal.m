function sample=RecSignal
% Функция для записи в файл измерений
a=serial('COM4');
a.BaudRate=57600;
fopen(a);
finishup = onCleanup(@() fclose(a));
sec=5;%время выполнения в секундах
t=0;
fid = fopen('средний1.log', 'a');     % открытие файла на запись 
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

