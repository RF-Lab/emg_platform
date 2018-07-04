% Task: EMG SVM classifier example
% Create: Lukyanchikov Andrey, xx-xx-2018
rng(123);
close all;
clear all; 
load('data10mov_no_abs.mat'); %áåç ôèëüòðîâ
All_data=data;
% ïåðåìåøèâàíèå
[str,col]=size(data);
[str_ind,col_ind]=size(data{1,1});
All_data_new=[];
for j=1:str
 [num,~]=size(All_data{j,1});
 p = randperm(num);
 for i=1:num
     All_data_new{j,1}(i,:)=All_data{j,1}(p(i),:);
 end
end
p=[];
 All_data=All_data_new; 
%êîíåö ïåðåìåøèâàíèÿ

data=[];
data{1,1}=All_data{1,1}(1:50,:); %50
data{2,1}=All_data{2,1}(1:50,:);
data{3,1}=All_data{3,1}(1:50,:);
data{4,1}=All_data{4,1}(1:50,:);
data{5,1}=All_data{5,1}(1:50,:);
data{6,1}=All_data{6,1}(1:50,:);
data{7,1}=All_data{7,1}(1:50,:);
data{8,1}=All_data{8,1}(1:50,:);
data{9,1}=All_data{9,1}(1:50,:);
data{10,1}=All_data{10,1}(1:50,:);
%ïðèçíàêè
IEMG=[];MV=[];MAV=[];MAV1=[];MAV2=[];TM4=[];RMS=[];V=[];LOG=[];DASDV=[];MYOP=[];WAMP=[];
MAVSLP=[];SSI=[];VAR=[];STD=[];WL=[];AAC=[];X=[];ff=[];PKF=[];MNF=[];MDF=[];MNP=[];FR=[];SM1=[];SM0=[]; VCF=[];SSC=[];
FMD_vec=[]; FMN_vec=[]; FR_vec=[]; MFMD_vec=[]; MFMN_vec=[]; LPC=[]; ZC=[];histog=[];
t=[];
T=[]; %Âåêòîð êëàññîâ
p=[];
P=[]; %Âåêòîð ïðèçíàêîâ
k=1;
v=2; %êîíñòàíòà v-order 2.1.9
[str,col]=size(data);
collor{1}='.b';
collor{2}='.r';
collor{3}='.k';
collor{4}='.g';
collor{5}='.m';
collor{6}='.b';
collor{7}='.r';
collor{8}='.k';
collor{9}='.g';
collor{10}='.m';
figure;
for f=1:str
    histog=[];
    LPC=[];
    PW=[];
    for c=1:col
        MAV_tmp=0;
         [n_traning,N]=size(data{f,c}); %ïîëó÷àþ êîëè÷åñòâî òðåíèðîâî÷íûõ ïîñëåäîâàòåëüíîñòåé è èõ äëèííó
         
        for i=1:n_traning
     IEMG(i) = sum(abs(data{f,c}(i,:)));
     MV(i) = (1/N)*sum(data{f,c}(i,:));
     MAV(i) = (1/N)*sum(abs(data{f,c}(i,:)));
     MAV1(i)=0;
     MAV2(i)=0;
     MYOP(i)=0;
     for ii=1:N
        if (ii<=0.75*N) && (ii>=0.25*N)
            w = 1;
            w2 = 1;
        else
            if (ii<0.25*N)
                w2 = 4*ii/N;
            else
                w2 = 4*(ii-N)/N;
            end
            w = 0.5;
        end
     MAV1(i)=MAV1(i)+w*abs(data{f,c}(i,ii));
     MAV2(i)=MAV2(i)+w2*abs(data{f,c}(i,ii));
     if(data{f,c}(i,ii)>=25)
         f1 = 1;
     else
         f1 = 0;
     end
     MYOP(i)=MYOP(i) + f1;
     end
     WAMP(i)=0;
     SSC(i)=0;
     for ii=2:N-1
         if(abs(data{f,c}(i,ii)-data{f,c}(i,ii+1))>=10)
             f2 = 1;
         else
             f2 = 0;
         end
         if((abs(data{f,c}(i,ii)-data{f,c}(i,ii-1))*abs(data{f,c}(i,ii)-data{f,c}(i,ii+1)))>=10)
             f3 = 1;
         else
             f3 = 0;
         end
         WAMP(i)=WAMP(i)+f2;
         SSC(i)=SSC(i)+f3;
     end
     count = 0;
     for ii =1:N-1
        if((((data{f,c}(i,ii) > 0) && (data{f,c}(i,ii+1) < 0))...
                || ((data{f,c}(i,ii) < 0) && (data{f,c}(i,ii+1) > 0)))...
                && abs(data{f,c}(i,ii) - data{f,c}(i,ii+1)) >= 5)
        count = count + 1;
        end
     end
    ZC(i) = count;
 
     TM4(i)=1/N*sum(data{f,c}(i,:));
     RMS(i) = sqrt(1/N*sum(data{f,c}(i,:)).^2); 
     V(i)=(1/N*sum((data{f,c}(i,:)).^v))^(1/v);
     LOG(i)=exp(1/N*sum(log10(abs(data{f,c}(i,:)))));
     DASDV(i)=sqrt(1/(N-1)*sum(abs(data{f,c}(i,2:end)-data{f,c}(i,1:end-1)).^2));
     MAVSLP(i) = MAV_tmp - MAV(i);
     MAV_tmp = MAV(i);
     SSI(i) = sum(abs((data{f,c}(i,:)).^2));
     VAR(i) = (1/N)*sum((data{f,c}(i,:)-mean(data{f,c}(i,:))).^2);
     STD(i) = sqrt((1/N)*sum((data{f,c}(i,:)).^2));
     WL(i) = sum(abs(data{f,c}(i,2:end)-data{f,c}(i,1:end-1)));
     AAC(i)=1/N*sum(abs(data{f,c}(i,2:end)-data{f,c}(i,1:end-1)));
     X(i) = max(data{f,c}(i,:));
     
     tmp=fft(data{f,c}(i,:));
     Pow=real(tmp).^2 + imag(tmp).^2;
     frec=1:length(tmp);
     MNF(i)=sum(frec.*Pow)/sum(Pow);
     MDF(i)=1/2*sum(Pow);
     MNP(i)=sum(Pow)/length(tmp);
     SM0(i)=sum(Pow);
     SM1(i)=sum(Pow.*frec);
     FR(i)=sum(Pow(1:(end/2)))/sum(Pow((end/2+1):end));
     VCF(i)=(SM1(i)/SM0(i))^2;
     ff(i)=mean(abs(tmp));
     %tmp=[tmp(1:frec_max(i)-1) tmp(frec_max(i)+1:end)];    
     PKF(i)=max((fft(data{f,c}(i,:))));
     PKF(i)=real(PKF(i))^2+imag(PKF(i))^2;
      FMD_vec(i)=FMD(data{f,c}(i,:));
      FMN_vec(i)=FMN(data{f,c}(i,:));
      MFMD_vec(i)=MFMD(data{f,c}(i,:));
      MFMN_vec(i)=MFMN(data{f,c}(i,:));
      LPC=[LPC; lpc(data{f,c}(i,:),9)];
    t(i) = f; %ïðèñâîåíèå êëàññà
    histog=[histog hist(data{f,c}(i,:),-300:300)'];
        end
        P=[P [IEMG;MV;
            MAV;MAV1;
            MAV2;TM4;
            RMS;
            V;
            LOG;DASDV;
            MYOP;
            WAMP;
            MAVSLP;
            SSI;
            VAR;
            STD;
            WL;AAC;
            X;ff;
            PKF;
            MNF;
            MDF;
            MNP;
            FR;SM1;
            SM0;
            VCF;
            SSC;
            FMD_vec; 
            FMN_vec; MFMD_vec;
            MFMN_vec;
            LPC(:,2:3)';ZC
            ]];
        %P=[P;LPC']
%        P=[P histog];
        T=[T t];
    end
    hold on
    plot(VAR, MAV,collor{f});
    %plot3(SSI,abs(DASDV),MFMN_vec,collor{f});
end
P_tmp=P;
P=[];
for i=1:str
P{i}=P_tmp;
end
 for j=1:str % 10 ëèíåéíûõ êëàññèôèêàòîðîâ SVM "îäèí ïðîòèâ âñåõc"
 tmpT=T;
 tmpT(tmpT~=j) = -1;
 tmpT(tmpT==j) = 1;
SVMModel{j} = fitcsvm(P{j}',tmpT','Standardize',true,'KernelFunction' , 'linear' ,...
    'BoxConstraint',200,'ClassNames',[-1,1]); %50%200
[Y,prob] = predict(SVMModel{j},P{j}');
test_errClassSVM(j,1)=sum(spones(Y-tmpT'));
 end
 %% Òåñòîâàÿ âûáîðêà:
 data=[];
 data{1,1}=All_data{1,1}(51:79,:); %51
 data{2,1}=All_data{2,1}(51:79,:);
 data{3,1}=All_data{3,1}(51:79,:);
 data{4,1}=All_data{4,1}(51:79,:);
 data{5,1}=All_data{5,1}(51:79,:);
 data{6,1}=All_data{6,1}(51:79,:);
 data{7,1}=All_data{7,1}(51:79,:);
 data{8,1}=All_data{8,1}(51:79,:);
 data{9,1}=All_data{9,1}(51:79,:);
 data{10,1}=All_data{10,1}(51:79,:);
 %ïðèçíàêè
IEMG=[];MV=[];MAV=[];MAV1=[];MAV2=[];TM4=[];RMS=[];V=[];LOG=[];DASDV=[];MYOP=[];WAMP=[];
MAVSLP=[];SSI=[];VAR=[];STD=[];WL=[];AAC=[];X=[];ff=[];PKF=[];MNF=[];MDF=[];MNP=[];FR=[];SM1=[];SM0=[]; VCF=[];SSC=[];
FMD_vec=[]; FMN_vec=[]; FR_vec=[]; MFMD_vec=[]; MFMN_vec=[]; LPC=[];ZC=[];    histog=[];
t=[];
T=[]; %Âåêòîð êëàññîâ
p=[];
P=[]; %Âåêòîð ïðèçíàêîâ
k=1;
v=2; %êîíñòàíòà v-order 2.1.9
[str,col]=size(data);
collor{1}='.b';
collor{2}='.r';
collor{3}='.k';
collor{4}='.g';
collor{5}='.m';
collor{6}='.b';
collor{7}='.r';
collor{8}='.k';
collor{9}='.g';
collor{10}='.m';
figure;
for f=1:str
    histog=[];
    LPC=[];
    PW=[];
    for c=1:col
        MAV_tmp=0;
         [n_traning,N]=size(data{f,c}); %ïîëó÷àþ êîëè÷åñòâî òðåíèðîâî÷íûõ ïîñëåäîâàòåëüíîñòåé è èõ äëèííó
         
        for i=1:n_traning
     IEMG(i) = sum(abs(data{f,c}(i,:)));
     MV(i) = (1/N)*sum(data{f,c}(i,:));
     MAV(i) = (1/N)*sum(abs(data{f,c}(i,:)));
     MAV1(i)=0;
     MAV2(i)=0;
     MYOP(i)=0;
     for ii=1:N
        if (ii<=0.75*N) && (ii>=0.25*N)
            w = 1;
            w2 = 1;
        else
            if (ii<0.25*N)
                w2 = 4*ii/N;
            else
                w2 = 4*(ii-N)/N;
            end
            w = 0.5;
        end
     MAV1(i)=MAV1(i)+w*abs(data{f,c}(i,ii));
     MAV2(i)=MAV2(i)+w2*abs(data{f,c}(i,ii));
     if(data{f,c}(i,ii)>=25)
         f1 = 1;
     else
         f1 = 0;
     end
     MYOP(i)=MYOP(i) + f1;
     end
     WAMP(i)=0;
     SSC(i)=0;
     for ii=2:N-1
         if(abs(data{f,c}(i,ii)-data{f,c}(i,ii+1))>=10)
             f2 = 1;
         else
             f2 = 0;
         end
         if((abs(data{f,c}(i,ii)-data{f,c}(i,ii-1))*abs(data{f,c}(i,ii)-data{f,c}(i,ii+1)))>=10)
             f3 = 1;
         else
             f3 = 0;
         end
         WAMP(i)=WAMP(i)+f2;
         SSC(i)=SSC(i)+f3;
     end
          count = 0;
     for ii =1:N-1
        if((((data{f,c}(i,ii) > 0) && (data{f,c}(i,ii+1) < 0))...
                || ((data{f,c}(i,ii) < 0) && (data{f,c}(i,ii+1) > 0)))...
                && abs(data{f,c}(i,ii) - data{f,c}(i,ii+1)) >= 5)
        count = count + 1;
        end
     end
    ZC(i) = count;
     TM4(i)=1/N*sum(data{f,c}(i,:));
     RMS(i) = sqrt(1/N*sum(data{f,c}(i,:)).^2); 
     V(i)=(1/N*sum((data{f,c}(i,:)).^v))^(1/v);
     LOG(i)=exp(1/N*sum(log10(abs(data{f,c}(i,:)))));
     DASDV(i)=sqrt(1/(N-1)*sum(abs(data{f,c}(i,2:end)-data{f,c}(i,1:end-1)).^2));
     MAVSLP(i) = MAV_tmp - MAV(i);
     MAV_tmp = MAV(i);
     SSI(i) = sum(abs((data{f,c}(i,:)).^2));
     VAR(i) = (1/N)*sum((data{f,c}(i,:)-mean(data{f,c}(i,:))).^2);
     STD(i) = sqrt((1/N)*sum((data{f,c}(i,:)).^2));
     WL(i) = sum(abs(data{f,c}(i,2:end)-data{f,c}(i,1:end-1)));
     AAC(i)=1/N*sum(abs(data{f,c}(i,2:end)-data{f,c}(i,1:end-1)));
     X(i) = max(data{f,c}(i,:));
     
     tmp=fft(data{f,c}(i,:));
     Pow=real(tmp).^2 + imag(tmp).^2;
     frec=1:length(tmp);
     MNF(i)=sum(frec.*Pow)/sum(Pow);
     MDF(i)=1/2*sum(Pow);
     MNP(i)=sum(Pow)/length(tmp);
     SM0(i)=sum(Pow);
     SM1(i)=sum(Pow.*frec);
     FR(i)=sum(Pow(1:(end/2)))/sum(Pow((end/2+1):end));
     VCF(i)=(SM1(i)/SM0(i))^2;
     ff(i)=mean(abs(tmp));
     %tmp=[tmp(1:frec_max(i)-1) tmp(frec_max(i)+1:end)];    
     PKF(i)=max((fft(data{f,c}(i,:))));
     PKF(i)=real(PKF(i))^2+imag(PKF(i))^2;
      FMD_vec(i)=FMD(data{f,c}(i,:));
      FMN_vec(i)=FMN(data{f,c}(i,:));
      MFMD_vec(i)=MFMD(data{f,c}(i,:));
      MFMN_vec(i)=MFMN(data{f,c}(i,:));
      LPC=[LPC; lpc(data{f,c}(i,:),9)];
           histog=[histog hist(data{f,c}(i,:),-300:300)'];
     % PW=[PW; pwelch(data{f,c}(i,:))'];
    t(i) = f; %ïðèñâîåíèå êëàññà
        end
        P=[P [IEMG;MV;
            MAV;MAV1;
            MAV2;TM4;
            RMS;
            V;
            LOG;DASDV;
            MYOP;WAMP;
            MAVSLP;
            SSI;
            VAR;
            STD;
            WL;AAC;
            X;ff;
            PKF;
            MNF;
            MDF;
            MNP;
            FR;SM1;
            SM0;
            VCF;
            SSC;
            FMD_vec; 
            FMN_vec; MFMD_vec;
            MFMN_vec;
            LPC(:,2:3)';ZC
            ]];
        %P=[P;LPC']
%         P=[P;histog];
        T=[T t];
    end
    hold on
    plot(VAR, MAV,collor{f});
end
P_tmp=P;
P=[];
for i=1:str
P{i}=P_tmp;
end
[~,col]=size(P{1});
Y=[];
prob=[];
Y_net11=[];
for i=1:col
Y=[];
prob=[];
j=1;
while j<=str
[Y_tmp,prob_tmp] = predict(SVMModel{j},P{j}(:,i)');
Y=[Y Y_tmp];
prob=[prob; prob_tmp];
 j=j+1;
end
[~,ind]=find(Y==1);

if length(ind)==1
    ClassSVM(i) = ind;
else
    if (length(ind)>1) %åñëè áîëüøå 1 êëàññà
      [~,ind]=max(prob(:,2));
      ClassSVM(i) = ind;
    else %åñëè âñå -1
        [~,ind]=min(abs(prob(:,1)));
        ClassSVM(i) = ind;
    end   
 % [~,ind]=max(prob);
end
end

errClassSVM = sum(spones(ClassSVM-T));
S=(errClassSVM/length(T))*100
stop=1;
