clear
m=1;
for a=0:0.5:5
    
%1.初始参数设定模块
%.传感器节点区域界限(单位 M)
xm=100;
ym=100;
%(1)汇聚节坐标给定
sink.x=0.5*xm;
sink.y=0.5*ym;
%区域内传器节数
n=100
%簇头优化比例（当选簇头的概率）
p=0.1;
P=0.1;
%能量模型（单位 焦）
%初始化能量模型
Eo=0.5;
%Eelec=Etx=Erx
ETX=50*0.000000001;
ERX=50*0.000000001;
%Transmit Amplifier types
Efs=10*0.000000000001;
Emp=0.0013*0.000000000001;
%Data Aggregation Energy
EDA=5*0.000000001;
%高能量节点超出一节点能量的百分比

%最大循环次数
rmax=5000
%算出参数 do
do=sqrt(Efs/Emp);
Et=0;
%2.无线传感器网络模型产生模块
%构建无线传感器网络,在区域内均匀投放100个节点,并画出图形
for i=1:1:n
    S1(i).xd=rand(1,1)*xm;
    S2(i).xd=S1(i).xd;
    S3(i).xd=S1(i).xd;
    S4(i).xd=S3(i).xd;
    XR4(i)=S4(i).xd;
    XR3(i)=S3(i).xd;
    XR2(i)=S2(i).xd;
    XR1(i)=S1(i).xd;
    S1(i).yd=rand(1,1)*ym;
    S2(i).yd=S1(i).yd;
    S3(i).yd=S1(i).yd;
    S4(i).yd=S3(i).yd;
    YR4(i)=S4(i).yd;
    S4(i).G=0;
    YR3(i)=S3(i).yd;
    S3(i).G=0;
    YR2(i)=S2(i).yd;
    YR1(i)=S1(i).yd;
    S1(i).G=0;
    S2(i).G=0;
    S1(i).E=Eo*(1+rand*a);
    S2(i).E=S1(i).E;
    S3(i).E=S1(i).E;
    S4(i).E=S3(i).E;
    E3(i)= S3(i).E;
    E4(i)= S4(i).E;
    Et=Et+E3(i);

    %initially there are no cluster heads only nodes
    S1(i).type='N';
    S2(i).type='N';
    S3(i).type='N';
    S4(i).type='N';
end

S1(n+1).xd=sink.x;
S1(n+1).yd=sink.y;
S2(n+1).xd=sink.x;
S2(n+1).yd=sink.y;
%3.网络运行模块
%簇头节点数
countCHs1=0;
cluster1=1;%此定义的目的仅仅是给定一个1开始的下标参数，真正的簇头数应该还减去1
flag_first_dead1=0;
flag_teenth_dead1=0;

%死亡节点数
dead1=0;
first_dead1(m)=0;
teenth_dead1(m)=0;

%活动节点数
allive1=n;
%counter for bit transmitted to Bases Station and to Cluster Heads

%(1)循环模式设定
for r=0:1:rmax     %该 for 循环将下面的所有程序包括在内，直到最后一 end 才结束循环
    r;
  %每过一个轮转周期(本程序为10次)使各节点的S（i）.G参数（该参数用于后面的簇选举，在该轮转周期内已当选过簇头的节点不能再当选）恢复为零
  if(mod(r, round(1/p) )==0)
    for i=1:1:n
        S1(i).G=0;
        S1(i).cl=0;
    end
  end
%(2)死亡节点检查模块
dead1=0;
for i=1:1:n
    %检查有无死亡节点
    if (S1(i).E<=0)
        dead1=dead1+1; 
        %(3)第一个死亡节点的产生时间(用轮次表示)
        %第一个节点死亡时间
        if (dead1==1)
           if(flag_first_dead1==0)
              first_dead1(m)=r;
              flag_first_dead1=1;
           end
        end
        %10%的节点死亡时间
        if(dead1==0.1*n)
           if(flag_teenth_dead1==0)
              teenth_dead1(m)=r;
              flag_teenth_dead1=1;
           end
        end
       
    end
    if S1(i).E>0
        S1(i).type='N';
    end
end

%(4)簇头选举模块
countCHs1=0;
cluster1=1;
for i=1:1:n
   if(S1(i).E>0)
   temp_rand=rand;     
   if ( (S1(i).G)<=0)  
       %簇头的选举，当选的簇头会把各种相关信存入下面程序所给定的变量中
        if(temp_rand<= (p/(1-p*mod(r,round(1/p)))))
            countCHs1=countCHs1+1;
           
             S1(i).type='C';
            S1(i).G=round(1/p)-1;
            C1(cluster1).xd=S1(i).xd;
            C1(cluster1).yd=S1(i).yd;
           distance=sqrt( (S1(i).xd-(S1(n+1).xd) )^2 + (S1(i).yd-(S1(n+1).yd) )^2 );
            C1(cluster1).distance=distance;
            C1(cluster1).id=i;
            X1(cluster1)=S1(i).xd;
            Y1(cluster1)=S1(i).yd;
            cluster1=cluster1+1;
           %计算簇头发送4000bit数据到基站的能量消耗（这里应是所有节点包括簇头每一轮发送4000bit数据）
           distance;
            if (distance>do)
                S1(i).E=S1(i).E- ( (ETX+EDA)*(4000) + Emp*4000*( distance*distance*distance*distance )); 
            end
            if (distance<=do)
                S1(i).E=S1(i).E- ( (ETX+EDA)*(4000)  + Efs*4000*( distance * distance )); 
            end
        end     
    
    end
    % S(i).G=S(i).G-1;  
   
 end 
end

%(5)簇内成员选择簇头模块(即簇的形成模块)
%簇内成员对簇头的选择（即簇的形成）算法
for i=1:1:n
   if ( S1(i).type=='N' && S1(i).E>0 )
     if(cluster1-1>=1)
       min_dis=Inf;
       min_dis_cluster=0;
       for c=1:1:cluster1-1
           temp=min(min_dis,sqrt( (S1(i).xd-C1(c).xd)^2 + (S1(i).yd-C1(c).yd)^2 ) );
           if ( temp<min_dis )
               min_dis=temp;
               min_dis_cluster=c;
           end
       end
       %簇内节点（发送4000bit数据）能量消耗
       
            min_dis;
            if (min_dis>do)
                S1(i).E=S1(i).E- ( ETX*(4000) + Emp*4000*( min_dis * min_dis * min_dis * min_dis)); 
            end
            if (min_dis<=do)
                S1(i).E=S1(i).E- ( ETX*(4000) + Efs*4000*( min_dis * min_dis)); 
            end
        %簇头（接受和融合这一簇内节点4000bit数据）的能量消耗
            S1(C1(min_dis_cluster).id).E = S1(C1(min_dis_cluster).id).E- ( (ERX + EDA)*4000 ); 
          
 
        S1(i).min_dis=min_dis;
        S1(i).min_dis_cluster=min_dis_cluster;
    else
        min_dis=sqrt( (S1(i).xd-S1(n+1).xd)^2 + (S1(i).yd-S1(n+1).yd)^2 );
            if (min_dis>do)
                S1(i).E=S1(i).E- ( ETX*(4000) + Emp*4000*( min_dis * min_dis * min_dis * min_dis)); 
            end
            if (min_dis<=do)
                S1(i).E=S1(i).E- ( ETX*(4000) + Efs*4000*( min_dis * min_dis)); 
            end
          
     end
  end
end

end



%3.网络运行模块
%簇头节点数
countCHs2=0;
cluster=1;%此定义的目的仅仅是给定一个1开始的下标参数，真正的簇头数应该还减去1
flag_first_dead2=0;
flag_teenth_dead2=0;

%死亡节点数
dead2=0;
first_dead2(m)=0;
teenth_dead2(m)=0;

%活动节点数
allive2=n;
%counter for bit transmitted to Bases Station and to Cluster Heads

%(1)循环模式设定
for r=0:1:rmax     %该 for 循环将下面的所有程序包括在内，直到最后一 end 才结束循环
    r;
  %每过一个轮转周期(本程序为10次)使各节点的S（i）.G参数（该参数用于后面的簇选举，在该轮转周期内已当选过簇头的节点不能再当选）恢复为零
  if(mod(r, round(1/p) )==0)
    for i=1:1:n
        S2(i).G=0;
        S2(i).cl=0;
    end
  end
%(2)死亡节点检查模块
dead2=0;
for i=1:1:n
    %检查有无死亡节点
    if (S2(i).E<=0)
        dead2=dead2+1; 
        %(3)第一个死亡节点的产生时间(用轮次表示)
        %第一个节点死亡时间
        if (dead2==1)
           if(flag_first_dead2==0)
              first_dead2(m)=r;
              flag_first_dead2(m)=1;
           end
        end
        %10%的节点死亡时间
        if(dead2==0.1*n)
           if(flag_teenth_dead2==0)
              teenth_dead2(m)=r;
              flag_teenth_dead2=1;
           end
        end
  
    end
    if S2(i).E>0
        S2(i).type='N';
    end
end

%(4)簇头选举模块
countCHs2=0;
cluster2=1;
for i=1:1:n
   if(S2(i).E>0)
   temp_rand=rand;     
   if ( (S2(i).G)<=0)  
       %簇头的选举，当选的簇头会把各种相关信存入下面程序所给定的变量中
        if(temp_rand<= (p/(1-p*mod(r,round(1/p)))))
            countCHs2=countCHs2+1;
          
             S2(i).type='C';
            S2(i).G=round(1/p)-1;
            C2(cluster2).xd=S2(i).xd;
            C2(cluster2).yd=S2(i).yd;
           distance=sqrt( (S2(i).xd-(S2(n+1).xd) )^2 + (S2(i).yd-(S2(n+1).yd) )^2 );
            C2(cluster2).distance=distance;
            C2(cluster2).id=i;
            X2(cluster2)=S2(i).xd;
            Y2(cluster2)=S2(i).yd;
            cluster2=cluster2+1;
           %计算簇头发送4000bit数据到基站的能量消耗（这里应是所有节点包括簇头每一轮发送4000bit数据）
           distance;
            if (distance>do)
                S2(i).E=S2(i).E- ( (ETX+EDA)*(4000) + Emp*4000*( distance*distance*distance*distance )); 
            end
            if (distance<=do)
                S2(i).E=S2(i).E- ( (ETX+EDA)*(4000)  + Efs*4000*( distance * distance )); 
            end
        end     
    
    end
    % S2(i).G=S2(i).G-1;  
   
 end 
end
%(5)簇内成员选择簇头模块(即簇的形成模块)
%簇内成员对簇头的选择（即簇的形成）算法
for i=1:1:n
   if ( S2(i).type=='N' && S2(i).E>0 )
     if(cluster2-1>=1)
       min_dis=sqrt( (S2(i).xd-S2(n+1).xd)^2 + (S2(i).yd-S2(n+1).yd)^2 );
       min_dis_cluster=0;
       for c=1:1:cluster2-1
           temp=min(min_dis,sqrt( (S2(i).xd-C2(c).xd)^2 + (S2(i).yd-C2(c).yd)^2 ) );
           if ( temp<min_dis )
               min_dis=temp;
               min_dis_cluster=c;
           end
       end
       %簇内节点（发送4000bit数据）能量消耗
       if(min_dis_cluster~=0)    
            min_dis;
            if (min_dis>do)
                S2(i).E=S2(i).E- ( ETX*(4000) + Emp*4000*( min_dis * min_dis * min_dis * min_dis)); 
            end
            if (min_dis<=do)
                S2(i).E=S2(i).E- ( ETX*(4000) + Efs*4000*( min_dis * min_dis)); 
            end
        %簇头（接受和融合这一簇内节点4000bit数据）的能量消耗
            S2(C2(min_dis_cluster).id).E = S2(C2(min_dis_cluster).id).E- ( (ERX + EDA)*4000 ); 
            
       else 
            min_dis;
            if (min_dis>do)
                S2(i).E=S2(i).E- ( ETX*(4000) + Emp*4000*( min_dis * min_dis * min_dis * min_dis)); 
            end
            if (min_dis<=do)
                S2(i).E=S2(i).E- ( ETX*(4000) + Efs*4000*( min_dis * min_dis)); 
            end
           
       end
        S2(i).min_dis=min_dis;
        S2(i).min_dis_cluster=min_dis_cluster;
    else
        min_dis=sqrt( (S2(i).xd-S2(n+1).xd)^2 + (S2(i).yd-S2(n+1).yd)^2 );
            if (min_dis>do)
                S2(i).E=S2(i).E- ( ETX*(4000) + Emp*4000*( min_dis * min_dis * min_dis * min_dis)); 
            end
            if (min_dis<=do)
                S2(i).E=S2(i).E- ( ETX*(4000) + Efs*4000*( min_dis * min_dis)); 
            end
           
     end
  end
end

end


d1=0.765*xm/2;
K=sqrt(0.5*n*do/pi)*xm/d1^2;
d2=xm/sqrt(2*pi*K);
Er=4000*(2*n*ETX+n*EDA+K*Emp*d1^4+n*Efs*d2^2);
S3(n+1).xd=sink.x;
S3(n+1).yd=sink.y;
S4(n+1).xd=sink.x;
S4(n+1).yd=sink.y;
%3.网络运行模块
%簇头节点数
countCHs3=0;
cluster3=1;%此定义的目的仅仅是给定一个1开始的下标参数，真正的簇头数应该还减去1
flag_first_dead3=0;
flag_teenth_dead3=0;

%死亡节点数
dead3=0;
first_dead3(m)=0;
teenth_dead3(m)=0;

%活动节点数
allive3=n;
%counter for bit transmitted to Bases Station and to Cluster Heads

%(1)循环模式设定
for r=0:1:rmax     %该 for 循环将下面的所有程序包括在内，直到最后一 end 才结束循环
    r;
  %每过一个轮转周期(本程序为10次)使各节点的S（i）.G参数（该参数用于后面的簇选举，在该轮转周期内已当选过簇头的节点不能再当选）恢复为零
  if(mod(r, round(1/P) )==0)
    for i=1:1:n
        S3(i).G=0;
        S3(i).cl=0;
    end
  end
Ea=Et*(1-r/rmax)/n;
%(2)死亡节点检查模块
dead3=0;
for i=1:1:n
    %检查有无死亡节点
    if (S3(i).E<=0)
        dead3=dead3+1; 
        %(3)第一个死亡节点的产生时间(用轮次表示)
        %第一个节点死亡时间
        if (dead3==1)
           if(flag_first_dead3==0)
              first_dead3(m)=r;
              flag_first_dead3=1;
           end
        end
        %10%的节点死亡时间
        if(dead3==0.1*n)
           if(flag_teenth_dead3==0)
              teenth_dead3(m)=r;
              flag_teenth_dead3=1;
           end
        end
 
    end
    if S3(i).E>0
        S3(i).type='N';
    end
end

%(4)簇头选举模块
countCHs3=0;
cluster3=1;
for i=1:1:n
 if Ea>0
 p(i)=P*n*S3(i).E*E3(i)/(Et*Ea);
 if(S3(i).E>0)
   temp_rand=rand;     
   if ( (S3(i).G)<=0)  
       %簇头的选举，当选的簇头会把各种相关信存入下面程序所给定的变量中
        if(temp_rand<= (p(i)/(1-p(i)*mod(r,round(1/p(i))))))
            countCHs3=countCHs3+1;

             S3(i).type='C';
            S3(i).G=round(1/p(i))-1;
            C3(cluster3).xd=S3(i).xd;
            C3(cluster3).yd=S3(i).yd;
           distance=sqrt( (S3(i).xd-(S3(n+1).xd) )^2 + (S3(i).yd-(S3(n+1).yd) )^2 );
            C3(cluster3).distance=distance;
            C3(cluster3).id=i;
            X3(cluster3)=S3(i).xd;
            Y3(cluster3)=S3(i).yd;
            cluster3=cluster3+1;
           %计算簇头发送4000bit数据到基站的能量消耗（这里应是所有节点包括簇头每一轮发送4000bit数据）
           distance;
            if (distance>do)
                S3(i).E=S3(i).E- ( (ETX+EDA)*(4000) + Emp*4000*( distance*distance*distance*distance )); 
            end
            if (distance<=do)
                S3(i).E=S3(i).E- ( (ETX+EDA)*(4000)  + Efs*4000*( distance * distance )); 
            end
        end     
    
    end
    % S3(i).G=S3(i).G-1;  
   
 end 
 end
end
%(5)簇内成员选择簇头模块(即簇的形成模块)
%簇内成员对簇头的选择（即簇的形成）算法
for i=1:1:n
   if ( S3(i).type=='N' && S3(i).E>0 )
     if(cluster3-1>=1)
       min_dis=Inf;
       min_dis_cluster=0;
       for c=1:1:cluster3-1
           temp=min(min_dis,sqrt( (S3(i).xd-C3(c).xd)^2 + (S3(i).yd-C3(c).yd)^2 ) );
           if ( temp<min_dis )
               min_dis=temp;
               min_dis_cluster=c;
           end
       end
       %簇内节点（发送4000bit数据）能量消耗
 
            min_dis;
            if (min_dis>do)
                S3(i).E=S3(i).E- ( ETX*(4000) + Emp*4000*( min_dis * min_dis * min_dis * min_dis)); 
            end
            if (min_dis<=do)
                S3(i).E=S3(i).E- ( ETX*(4000) + Efs*4000*( min_dis * min_dis)); 
            end
        %簇头（接受和融合这一簇内节点4000bit数据）的能量消耗
            S3(C3(min_dis_cluster).id).E = S3(C3(min_dis_cluster).id).E- ( (ERX + EDA)*4000 ); 
           

            
      
        S3(i).min_dis=min_dis;
        S3(i).min_dis_cluster=min_dis_cluster;
   else
            min_dis=sqrt( (S3(i).xd-S3(n+1).xd)^2 + (S3(i).yd-S3(n+1).yd)^2 );
            if (min_dis>do)
                S3(i).E=S3(i).E- ( ETX*(4000) + Emp*4000*( min_dis * min_dis * min_dis * min_dis)); 
            end
            if (min_dis<=do)
                S3(i).E=S3(i).E- ( ETX*(4000) + Efs*4000*( min_dis * min_dis)); 
            end
           
   end
  end
end

end



countCHs4=0;
cluster=1;%此定义的目的仅仅是给定一个1开始的下标参数，真正的簇头数应该还减去1
flag_first_dead4=0;
flag_teenth_dead4=0;

%死亡节点数
dead4=0;
first_dead4(m)=0;
teenth_dead4(m)=0;

%活动节点数
allive4=n;
%counter for bit transmitted to Bases Station and to Cluster Heads

%(1)循环模式设定
for r=0:1:rmax     %该 for 循环将下面的所有程序包括在内，直到最后一 end 才结束循环
    r;
  %每过一个轮转周期(本程序为10次)使各节点的S（i）.G参数（该参数用于后面的簇选举，在该轮转周期内已当选过簇头的节点不能再当选）恢复为零
  if(mod(r, round(1/P) )==0)
    for i=1:1:n
        S4(i).G=0;
        S4(i).cl=0;
    end
  end
Ea=Et*(1-r/rmax)/n;
%(2)死亡节点检查模块
dead4=0;
for i=1:1:n
    %检查有无死亡节点
    if (S4(i).E<=0)
        dead4=dead4+1; 
        %(3)第一个死亡节点的产生时间(用轮次表示)
        %第一个节点死亡时间
        if (dead4==1)
           if(flag_first_dead4==0)
              first_dead4(m)=r;
              flag_first_dead4=1;
           end
        end
        %10%的节点死亡时间
        if(dead4==0.1*n)
           if(flag_teenth_dead4==0)
              teenth_dead4(m)=r;
              flag_teenth_dead4=1;
           end
        end

    end
    if S4(i).E>0
        S4(i).type='N';
    end
end
%(4)簇头选举模块
countCHs4=0;
cluster4=1;
for i=1:1:n
 if Ea>0
 p(i)=P*n*S4(i).E*E4(i)/(Et*Ea);
 if(S4(i).E>0)
   temp_rand=rand;     
   if ( (S4(i).G)<=0)  
       %簇头的选举，当选的簇头会把各种相关信存入下面程序所给定的变量中
        if(temp_rand<= (p(i)/(1-p(i)*mod(r,round(1/p(i))))))
            countCHs4=countCHs4+1;
             S4(i).type='C';
            S4(i).G=round(1/p(i))-1;
            C4(cluster4).xd=S4(i).xd;
            C4(cluster4).yd=S4(i).yd;
           distance=sqrt( (S4(i).xd-(S4(n+1).xd) )^2 + (S4(i).yd-(S4(n+1).yd) )^2 );
            C4(cluster4).distance=distance;
            C4(cluster4).id=i;
            X4(cluster4)=S4(i).xd;
            Y4(cluster4)=S4(i).yd;
            cluster4=cluster4+1;
           %计算簇头发送4000bit数据到基站的能量消耗（这里应是所有节点包括簇头每一轮发送4000bit数据）
           distance;
            if (distance>do)
                S4(i).E=S4(i).E- ( (ETX+EDA)*(4000) + Emp*4000*( distance*distance*distance*distance )); 
            end
            if (distance<=do)
                S4(i).E=S4(i).E- ( (ETX+EDA)*(4000)  + Efs*4000*( distance * distance )); 
            end
        end     
    
    end
    % S4(i).G=S4(i).G-1;  
   
 end 
 end
end

%(5)簇内成员选择簇头模块(即簇的形成模块)
%簇内成员对簇头的选择（即簇的形成）算法
for i=1:1:n
   if ( S4(i).type=='N' && S4(i).E>0 )
     if(cluster4-1>=1)
       min_dis=sqrt( (S4(i).xd-S4(n+1).xd)^2 + (S4(i).yd-S4(n+1).yd)^2 );
       min_dis_cluster=0;
       for c=1:1:cluster4-1
           temp=min(min_dis,sqrt( (S4(i).xd-C4(c).xd)^2 + (S4(i).yd-C4(c).yd)^2 ) );
           if ( temp<min_dis )
               min_dis=temp;
               min_dis_cluster=c;
           end
       end
       %簇内节点（发送4000bit数据）能量消耗
       if(min_dis_cluster~=0)    
            min_dis;
            if (min_dis>do)
                S4(i).E=S4(i).E- ( ETX*(4000) + Emp*4000*( min_dis * min_dis * min_dis * min_dis)); 
            end
            if (min_dis<=do)
                S4(i).E=S4(i).E- ( ETX*(4000) + Efs*4000*( min_dis * min_dis)); 
            end
        %簇头（接受和融合这一簇内节点4000bit数据）的能量消耗
            S4(C4(min_dis_cluster).id).E = S4(C4(min_dis_cluster).id).E- ( (ERX + EDA)*4000 ); 
            
       else 
            min_dis;
            if (min_dis>do)
                S4(i).E=S4(i).E- ( ETX*(4000) + Emp*4000*( min_dis * min_dis * min_dis * min_dis)); 
            end
            if (min_dis<=do)
                S4(i).E=S4(i).E- ( ETX*(4000) + Efs*4000*( min_dis * min_dis)); 
            end
           
            
       end
        S4(i).min_dis=min_dis;
       S4(i).min_dis_cluster=min_dis_cluster;
   else
            min_dis=sqrt( (S4(i).xd-S4(n+1).xd)^2 + (S4(i).yd-S4(n+1).yd)^2 );
            if (min_dis>do)
                S4(i).E=S4(i).E- ( ETX*(4000) + Emp*4000*( min_dis * min_dis * min_dis * min_dis)); 
            end
            if (min_dis<=do)
                S4(i).E=S4(i).E- ( ETX*(4000) + Efs*4000*( min_dis * min_dis)); 
            end
            
   end
  end
end
end
m=m+1;
end

r=0:0.5:5;
subplot(2,1,1);
plot(r,first_dead1,':k',r,first_dead2,'-r',r,first_dead3,'--b',r,first_dead4,'-.m');
legend('leachm1','leachm2','DEEC1','DEEC2');
xlabel('x(time)');
ylabel('y(dead)');
title('\bf leach(1,2)和DEEC(1,2)的生命周期对比');
subplot(2,1,2);
plot(r,teenth_dead1,':k',r,teenth_dead2,'-r',r,teenth_dead3,'--b',r,teenth_dead4,'-.m');
legend('leachm1','leachm2','DEEC1','DEEC2');
xlabel('x(time)');
ylabel('y(data)');
title('\bf leach(1,2)和DEEC(1,2)的稳定周期对比');