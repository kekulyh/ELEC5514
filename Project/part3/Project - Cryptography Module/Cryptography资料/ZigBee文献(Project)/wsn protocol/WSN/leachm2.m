clear
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
a=1;
%最大循环次数
rmax=5000
%算出参数 do
do=sqrt(Efs/Emp);
%2.无线传感器网络模型产生模块
%构建无线传感器网络,在区域内均匀投放100个节点,并画出图形
for i=1:1:n
    S(i).xd=rand(1,1)*xm;
    XR(i)=S(i).xd;
    S(i).yd=rand(1,1)*ym;
    YR(i)=S(i).yd;
    S(i).G=0;
    S(i).E=Eo*(1+rand*a);
    %initially there are no cluster heads only nodes
    S(i).type='N';
end

S(n+1).xd=sink.x;
S(n+1).yd=sink.y;
%3.网络运行模块
%簇头节点数
countCHs=0;
cluster=1;%此定义的目的仅仅是给定一个1开始的下标参数，真正的簇头数应该还减去1
flag_first_dead=0;
flag_teenth_dead=0;
flag_all_dead=0;
%死亡节点数
dead=0;
first_dead=0;
teenth_dead=0;
all_dead=0;
%活动节点数
allive=n;
%counter for bit transmitted to Bases Station and to Cluster Heads
packets_TO_BS=0;
packets_TO_CH=0;
%(1)循环模式设定
for r=0:1:rmax     %该 for 循环将下面的所有程序包括在内，直到最后一 end 才结束循环
    r
  %每过一个轮转周期(本程序为10次)使各节点的S（i）.G参数（该参数用于后面的簇选举，在该轮转周期内已当选过簇头的节点不能再当选）恢复为零
  if(mod(r, round(1/p) )==0)
    for i=1:1:n
        S(i).G=0;
        S(i).cl=0;
    end
  end
%(2)死亡节点检查模块
dead=0;
for i=1:1:n
    %检查有无死亡节点
    if (S(i).E<=0)
        dead=dead+1; 
        %(3)第一个死亡节点的产生时间(用轮次表示)
        %第一个节点死亡时间
        if (dead==1)
           if(flag_first_dead==0)
              first_dead=r;
              flag_first_dead=1;
           end
        end
        %10%的节点死亡时间
        if(dead==0.1*n)
           if(flag_teenth_dead==0)
              teenth_dead=r;
              flag_teenth_dead=1;
           end
        end
        if(dead==n)
           if(flag_all_dead==0)
              all_dead=r;
              flag_all_dead=1;
           end
        end
    end
    if S(i).E>0
        S(i).type='N';
    end
end
STATISTICS.DEAD(r+1)=dead;
STATISTICS.ALLIVE(r+1)=allive-dead;
%(4)簇头选举模块
countCHs=0;
cluster=1;
for i=1:1:n
   if(S(i).E>0)
   temp_rand=rand;     
   if ( (S(i).G)<=0)  
       %簇头的选举，当选的簇头会把各种相关信存入下面程序所给定的变量中
        if(temp_rand<= (p/(1-p*mod(r,round(1/p)))))
            countCHs=countCHs+1;
            packets_TO_BS=packets_TO_BS+1;
            PACKETS_TO_BS(r+1)=packets_TO_BS;
             S(i).type='C';
            S(i).G=round(1/p)-1;
            C(cluster).xd=S(i).xd;
            C(cluster).yd=S(i).yd;
           distance=sqrt( (S(i).xd-(S(n+1).xd) )^2 + (S(i).yd-(S(n+1).yd) )^2 );
            C(cluster).distance=distance;
            C(cluster).id=i;
            X(cluster)=S(i).xd;
            Y(cluster)=S(i).yd;
            cluster=cluster+1;
           %计算簇头发送4000bit数据到基站的能量消耗（这里应是所有节点包括簇头每一轮发送4000bit数据）
           distance;
            if (distance>do)
                S(i).E=S(i).E- ( (ETX+EDA)*(4000) + Emp*4000*( distance*distance*distance*distance )); 
            end
            if (distance<=do)
                S(i).E=S(i).E- ( (ETX+EDA)*(4000)  + Efs*4000*( distance * distance )); 
            end
        end     
    
    end
    % S(i).G=S(i).G-1;  
   
 end 
end
STATISTICS.COUNTCHS(r+1)=countCHs;
%(5)簇内成员选择簇头模块(即簇的形成模块)
%簇内成员对簇头的选择（即簇的形成）算法
for i=1:1:n
   if ( S(i).type=='N' && S(i).E>0 )
     if(cluster-1>=1)
       min_dis=sqrt( (S(i).xd-S(n+1).xd)^2 + (S(i).yd-S(n+1).yd)^2 );
       min_dis_cluster=0;
       for c=1:1:cluster-1
           temp=min(min_dis,sqrt( (S(i).xd-C(c).xd)^2 + (S(i).yd-C(c).yd)^2 ) );
           if ( temp<min_dis )
               min_dis=temp;
               min_dis_cluster=c;
           end
       end
       %簇内节点（发送4000bit数据）能量消耗
       if(min_dis_cluster~=0)    
            min_dis;
            if (min_dis>do)
                S(i).E=S(i).E- ( ETX*(4000) + Emp*4000*( min_dis * min_dis * min_dis * min_dis)); 
            end
            if (min_dis<=do)
                S(i).E=S(i).E- ( ETX*(4000) + Efs*4000*( min_dis * min_dis)); 
            end
        %簇头（接受和融合这一簇内节点4000bit数据）的能量消耗
            S(C(min_dis_cluster).id).E = S(C(min_dis_cluster).id).E- ( (ERX + EDA)*4000 ); 
            packets_TO_CH=packets_TO_CH+1;
       else 
            min_dis;
            if (min_dis>do)
                S(i).E=S(i).E- ( ETX*(4000) + Emp*4000*( min_dis * min_dis * min_dis * min_dis)); 
            end
            if (min_dis<=do)
                S(i).E=S(i).E- ( ETX*(4000) + Efs*4000*( min_dis * min_dis)); 
            end
            packets_TO_BS=packets_TO_BS+1;
            
       end
        S(i).min_dis=min_dis;
        S(i).min_dis_cluster=min_dis_cluster;
    else
        min_dis=sqrt( (S(i).xd-S(n+1).xd)^2 + (S(i).yd-S(n+1).yd)^2 );
            if (min_dis>do)
                S(i).E=S(i).E- ( ETX*(4000) + Emp*4000*( min_dis * min_dis * min_dis * min_dis)); 
            end
            if (min_dis<=do)
                S(i).E=S(i).E- ( ETX*(4000) + Efs*4000*( min_dis * min_dis)); 
            end
            packets_TO_BS=packets_TO_BS+1;
     end
  end
end
STATISTICS.PACKETS_TO_CH(r+1)=packets_TO_CH;
STATISTICS.PACKETS_TO_BS(r+1)=packets_TO_BS;
end
first_dead
teenth_dead
all_dead
STATISTICS.DEAD(r+1)
STATISTICS.ALLIVE(r+1)
STATISTICS.PACKETS_TO_CH(r+1)
STATISTICS.PACKETS_TO_BS(r+1)
STATISTICS.COUNTCHS(r+1)
r=0:5000;
subplot(2,2,1);
plot(r,STATISTICS.DEAD);
subplot(2,2,2);
plot(r,STATISTICS.ALLIVE);
subplot(2,2,3);
plot(r,STATISTICS.PACKETS_TO_BS);
subplot(2,2,4);
plot(r,STATISTICS.COUNTCHS);
%STATISTICS，结构体数组，包括下面的5个变量；
%countCHs(r+1）,每一轮所选出的簇头数目;
%packets_TO_BS(r+1),基站收到的数据包总数;
%PACKETS_TO_CH(r+1),簇头收到的数据包总数;
%first_dead,第一个节点死亡的时间;
%teenth_dead=r,10%的节点死亡的时间；
%dead(r+1),每一轮的死亡节点数；
%allive(r+1),每一轮的活动节点数。

