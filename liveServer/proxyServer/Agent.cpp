#include "Agent.hh"
#include "metrics.h"
#include "GroupsockHelper.hh"
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

#include <iostream>
using namespace std;

extern int Concurrecy;
void paraseUrl(char *url,char *source)
{
    int len=0;
    char *temp=strchr(url,':');
    url=temp+3;
    if((temp=strchr(url,':'))==NULL){
        temp=strchr(url,'/');
    }
    len=temp-url;
    if(len>15){
        temp=strchr(url,'/');
        len=temp-url;
    }
    char addr[16];
    strncpy(addr,url,len);
    addr[len]='\0';
    unsigned long uaddr=inet_addr(addr);
    sprintf(source,"%lu%s",uaddr,temp+1);
}
 
void *thr_agent(void *arg)
{
    struct thr_parameter *param=(struct thr_parameter *)arg;

    int cpu_num=cpu_num_func().uint16;
    int cpu_speed=cpu_speed_func().uint32;
    int &concurrecy=Concurrecy;
    float cpu_idle;
    float net_rates;

    int connect_fd=socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in srv_addr;
    srv_addr.sin_family=AF_INET;
    srv_addr.sin_addr.s_addr=inet_addr(param->scheduleServerAddr);
    srv_addr.sin_port=htons(param->scheduleServerPort);
    int result = connect(connect_fd,(struct sockaddr*)&srv_addr,sizeof(srv_addr));
    if(result==-1){
        close(connect_fd);
        return ((void*)-1);
    }
    char fResponseBuffer[512]={0};
    sprintf(fResponseBuffer,"S,%d,%d,%d,%s",cpu_num,cpu_speed,param->proxyServerPort,param->sources);
    cout<<fResponseBuffer<<endl;
    write(connect_fd,fResponseBuffer,strlen(fResponseBuffer));
    while(1){
        float cpu_total=cpu_user_func().f
            +cpu_nice_func().f
            +cpu_system_func().f
            +cpu_idle_func().f
            +cpu_wio_func().f
            +cpu_aidle_func().f;
        cpu_idle=cpu_idle_func().f/cpu_total;
        net_rates=(bytes_in_func().f+bytes_out_func().f)/1024;

        bzero(fResponseBuffer,512);
        sprintf(fResponseBuffer,"L,%f,%d,%f",cpu_idle,concurrecy,net_rates);
        cout<<fResponseBuffer<<endl;
        int len=strlen(fResponseBuffer);
        write(connect_fd,fResponseBuffer,len);
        sleep(1);
    }
    return ((void*)0);
}

