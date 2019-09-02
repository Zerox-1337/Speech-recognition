close all;
clear all;
clc;

mtotal=cell(1,3);
prelength=-0.75;
notchlength=-0.95;
mnum=15;
msectionnum=15;
for i=1:3
    if i==1 % Select between sound files here
        [d,fs]= audioread('hello-1.wav');
    elseif i==2
        d = audioread('hello-2.wav');
    elseif i==3
        d = audioread('bye.wav');    
    elseif i==4
        d = audioread('by.wav');
    end
d=d(:,1);
x=d;
% x=filter([1,-1],[1,-0.95],x);
xb = buffer(x, 320);
[M, N] = size(xb);
e=zeros(M,N);
xprime=e;
z1=[];z2=[];z0=[];
m=zeros(N,mnum);
m2=m;
mmean=zeros(msectionnum,mnum);
lamda=m;
for n = 1:N
    
    [xf,z0]=filter(conv([1 -1],[1 prelength]),[1 notchlength],xb(:,n),z0);
    
	r=autocorr(xf,mnum-1);
    A=m(n,:);
    A(1)=1;
    E=r(1);

    for k=0:mnum-2 % Number of coeff. 

      
        for j=0:k
            lamda(n,k+1)=lamda(n,k+1)- (A(j+1)*r( ( k+1-(j) +1) ));

        end
        lamda(n,k+1)=lamda(n,k+1)/E;
        
        
        for n2=0:(k+1)/2
            temp=A((k)+ 1 -(n2)+1)+lamda(n,k+1)*A(n2 +1);
            A(n2+1)=A(n2+1)+lamda(n,k+1)*A((k)+ 1 -(n2)+1);
            A((k)+ 1 -(n2)+1)=temp;
        end
       
        E=E*(1-lamda(n,k+1)^2);

      
   
    end
   
    m(n,:)=lamda(n,:);
%     m2(n,:)=levinson(r);
    [e(:,n),z1]=filter(m(n,:),1,xf,z1);
    eprime=e;
    [xprime(:,n),z2]=filter(1,m(n,:),eprime(:,n),z2);
end


msize=size(m);
sectionsize=floor(msize(1)/msectionnum);

for j=1:msectionnum
mmean(j,:)=mean(m( (j-1)*sectionsize+1:j*sectionsize ,:),1);

end


mtotal{i}=mmean;

%schur step up step down for converting.  system equaion to lattic ETI265
%EITA50
%10 groups average
%corr
%notch ,pre-emphasis
end
figure
diff12=abs(mtotal{1}-mtotal{2});
sum(sum(diff12))
diff13=abs(mtotal{1}-mtotal{3});
sum(sum(diff13))
% diff12=diff12.*(diff12<0.05);
% diff13=diff13.*(diff13<0.05);

plot(diff12,'b');
hold on
plot(diff13,'r');
