%Формирование спектрограмм
%08.11.2019
%Лукьянчиков А.И. dronluk@yandex.ru
clear all;
close all;
%папки с изображениями(если нет нужно создать)
dir_names_png={'png\train\1\'; 
    '\png\train\2\';
    '\png\train\3\';
    '\png\train\4\';
    '\png\train\5\';
    '\png\train\6\';
    '\png\train\7\';
    '\png\train\8\';
    '\png\train\9\';};
dir_names_png_test={'png\test\1\';
    'png\test\2\';
    'png\test\3\';
    'png\test\4\';
    'png\test\5\';
    'png\test\6\';
    'png\test\7\';
    'png\test\8\';
    'png\test\9\';};
              
load('nine_movs_six_sub_cut.mat');

len_win=64;
time = 40;
len_fft=len_win*2;
len_bin = ceil(len_fft/2);
stap=10; % 84% перекрытия
wind=hann(len_win); %окно Ханна
size_table=size(nine_movs_six_sub_cut);
size_cell=size(nine_movs_six_sub_cut{1,1}{1,1});
%Формирование спектрограмм
for i=1:size_table(1) %Номер субъекта без последнего, последний на тест
    for j=1:size_cell(1) %Номер движения
        size_ex=size(nine_movs_six_sub_cut{i,1}{1,1}{j,1});
        for ex=1:size_ex(1) %номер испытания
            x=nine_movs_six_sub_cut{i,1}{1,1}{j,1}(ex,:); % сигнал
            spectro=zeros(len_bin,time);
                for k=0:time-1
                  if(len_win+(stap*k)>length(x))
                   break;
                  end
                    frames(:,k+1)=x(1+(stap*k):len_win+(stap*k)); % выделение окна
                    frames(:,k+1)=frames(:,k+1).*(wind); %оконное взвешивание
                    tmp_spectro(:,k+1)=abs(fft(frames(:,k+1),len_fft)); % преобразование Фурье
                    spectro(:,k+1)=tmp_spectro(1:len_bin,k+1); % 
                    spectro(:,k+1)=20*log10(spectro(:,k+1)); % логарифмическая шкала

                end
            spectro=spectro(:,1:k);
            spectro_max = max(max(abs(spectro))); % нормировка от 0 до 1
            spectro_norm=uint16(((spectro./spectro_max)+1).*3.2768e+04); % перобразование в uint16
            img=imshow(spectro_norm);
            if(i<=size_table(1)-1)
                name_png=strcat(dir_names_png{j,1},num2str(i),'_',num2str(j),'_',num2str(ex),'.png'); % субъекты с 1 по 5
            else
                name_png=strcat(dir_names_png_test{j,1},num2str(i),'_',num2str(j),'_',num2str(ex),'.png'); %6й субъект для теста
            end
            
            imwrite(spectro_norm, name_png, 'png')
            fprintf('Sub= %d ; № mov= %d; № exper= %d  \n',i,j,ex);
        end
    end
end
size(spectro)

