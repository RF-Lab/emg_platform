%Ôîðìèðîâàíèå ñïåêòðîãðàìì
%08.11.2019
%Ëóêüÿí÷èêîâ À.È. dronluk@yandex.ru
clear all;
close all;
%ïàïêè ñ èçîáðàæåíèÿìè(åñëè íåò íóæíî ñîçäàòü)
dir_names_png={'png\train\1\'; 
    'png\train\2\';
    'png\train\3\';
    'png\train\4\';
    'png\train\5\';
    'png\train\6\';
    'png\train\7\';
    'png\train\8\';
    'png\train\9\';};
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
stap=10; % 84% ïåðåêðûòèÿ
wind=hann(len_win); %îêíî Õàííà
size_table=size(nine_movs_six_sub_cut);
size_cell=size(nine_movs_six_sub_cut{1,1}{1,1});
%Ôîðìèðîâàíèå ñïåêòðîãðàìì
for i=1:size_table(1) %Íîìåð ñóáúåêòà áåç ïîñëåäíåãî, ïîñëåäíèé íà òåñò
    for j=1:size_cell(1) %Íîìåð äâèæåíèÿ
        size_ex=size(nine_movs_six_sub_cut{i,1}{1,1}{j,1});
        for ex=1:size_ex(1) %íîìåð èñïûòàíèÿ
            x=nine_movs_six_sub_cut{i,1}{1,1}{j,1}(ex,:); % ñèãíàë
            spectro=zeros(len_bin,time);
                for k=0:time-1
                  if(len_win+(stap*k)>length(x))
                   break;
                  end
                    frames(:,k+1)=x(1+(stap*k):len_win+(stap*k)); % âûäåëåíèå îêíà
                    frames(:,k+1)=frames(:,k+1).*(wind); %îêîííîå âçâåøèâàíèå
                    tmp_spectro(:,k+1)=abs(fft(frames(:,k+1),len_fft)); % ïðåîáðàçîâàíèå Ôóðüå
                    spectro(:,k+1)=tmp_spectro(1:len_bin,k+1); % 
                    spectro(:,k+1)=20*log10(spectro(:,k+1)); % ëîãàðèôìè÷åñêàÿ øêàëà

                end
            spectro=spectro(:,1:k);
            spectro_max = max(max(abs(spectro))); % íîðìèðîâêà îò 0 äî 1
            spectro_norm=uint16(((spectro./spectro_max)+1).*3.2768e+04); % ïåðîáðàçîâàíèå â uint16
            img=imshow(spectro_norm);
            if(i<=size_table(1)-1)
                name_png=strcat(dir_names_png{j,1},num2str(i),'_',num2str(j),'_',num2str(ex),'.png'); % ñóáúåêòû ñ 1 ïî 5
            else
                name_png=strcat(dir_names_png_test{j,1},num2str(i),'_',num2str(j),'_',num2str(ex),'.png'); %6é ñóáúåêò äëÿ òåñòà
            end
            
            imwrite(spectro_norm, name_png, 'png')
            fprintf('Sub= %d ; ¹ mov= %d; ¹ exper= %d  \n',i,j,ex);
        end
    end
end
size(spectro)

