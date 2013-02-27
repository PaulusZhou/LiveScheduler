#include "version.hh"
#include "Source.hh"

#define AGENT_BUFFER_SIZE 1024

SourceSet *sourceSet;
MetricsSet *metricsSet;
unsigned char fRequestBuffer[AGENT_BUFFER_SIZE];
void incomingRequestHandler(){
    struct sockaddr_in dummy; 
    bzero(fRequestBuffer,AGENT_BUFFER_SIZE);
    int bytesRead = read(fAgentInputSocket, fRequestBuffer, AGENT_BUFFER_SIZE, dummy);
    handleRequestBytes(bytesRead);
}
void handleRequestBytes(int newBytesRead){
    if(newBytesRead==-1){
        cout<<"read socket error"<<endl;
        metricsSet->clearMetrics(fAgentAddr.sin_addr.s_addr);
        cout<<"read socket error1"<<endl;
        sourceSet->clearSource(fAgentAddr.sin_addr.s_addr);
        metricsSet->printMetrics();
        sourceSet->printSource();
    }
    else{
        char* ptrBytes=(char*)fRequestBuffer;
        cout<<ptrBytes<<endl;
        parseBytes(ptrBytes);
        metricsSet->printMetrics();
        sourceSet->printSource();
        sourceSet->updatePrior();
    }
}
void parseBytes(char *bytes){
    char *space,temp[128]={0};
    int i=0;
    if(bytes[0]=='S'){
        bytes+=2;
        for(;i<3;i++)
        {
            space=strchr(bytes,',');
            int len=space-bytes;
            strncpy(temp,bytes,len);
            bytes=space;
            temp[len]='\0';
            switch(i)
            {
                case 0:
                    info.cpu_num=atoi(temp);
                    break;
                case 1:
                    info.cpu_speed=atoi(temp);
                    break;
                case 2:
                    info.agentport=atoi(temp);
                    break;
                default:
                    break;
            }
            bzero(temp,128);
            bytes++;
        }
        parseSources(bytes);
    }
    else if(bytes[0]=='L'){
        bytes+=2;
        for(;i<2;i++)
        {
            space=strchr(bytes,',');
            int len=space-bytes;
            strncpy(temp,bytes,len);
            bytes=space;
            temp[len]='\0';
            switch(i)
            {
                case 0:
                    info.cpu_idle=atof(temp);
                    break;
                case 1:
                    info.concurrecy=atoi(temp);
                    break;
                default:
                    break;
            }
            bzero(temp,128);
            bytes++;
        }
        info.net_rates=atof(bytes);
        metricsSet->assignMetrics(fAgentAddr.sin_addr.s_addr,info);
    }
    else{
        /* error */
    }
}
void parseSources(char *bytes){
    if(bytes==NULL) return;
    cout<<bytes<<endl;
    char *temp=NULL,str[100];
    int len=0;
    while(true){
        temp=strchr(bytes,' ');
        if(temp==NULL){
            cout<<"parseSources:"<<bytes<<endl;
            strcpy(info.testsrc,bytes);
            cout<<"info.testsrc "<<info.testsrc<<endl;
            metricsSet->assignMetrics(fAgentAddr.sin_addr.s_addr,info);
            sourceSet->assignSource(bytes,fAgentAddr.sin_addr.s_addr);
            break;
        }
        else{
            len=(int)temp-(int)bytes;
            strncpy(str,bytes,len);
            str[len]='\0';
            bytes=temp+1;
            cout<<"parseSources:"<<str<<endl;
            sourceSet->assignSource(str,fAgentAddr.sin_addr.s_addr);
        }    
    }
}


int main(int argc, char** argv) {
  sourceSet = new SourceSet();
  metricsSet = new MetricsSet();
  delete sourceSet;
  delete metricsSet;
  return 0;
}
