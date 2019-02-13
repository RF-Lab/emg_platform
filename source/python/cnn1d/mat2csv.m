clear all; 
load('..\..\..\data\data10mov_no_abs.mat') ;

for n=1:length(data)
    data_cell = data{n} ;
    f = fopen( sprintf('%1d_data.txt',n),'w+t' ) ;
    for k=1:size(data_cell)
        fprintf( f, '%16.10f, ', data_cell(k,:) ) ;
        fprintf( f, '\n' ) ;
    end
    
    fclose(f) ;
end