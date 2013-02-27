#include "RedirectRTSPServer.hh"
#include "RTSPCommon.hh"
#include <GroupsockHelper.hh>
#include <liveMedia.hh>
#include <string.h>
#include "Source.hh"
#include <iostream>
using namespace std;

extern SourceSet *sourceSet;
extern MetricsSet *metricsSet;

RedirectRTSPServer*
RedirectRTSPServer::createNew(UsageEnvironment& env, Port ourPort, Port mPort,
        UserAuthenticationDatabase* authDatabase,
        unsigned reclamationTestSeconds) {
    int ourSocket = setUpOurSocket(env, ourPort);
    int mSocket = setUpOurSocket(env, mPort);
    if (ourSocket == -1|| mSocket == -1) return NULL;

    return new RedirectRTSPServer(env, ourSocket, ourPort, mSocket, mPort, authDatabase, reclamationTestSeconds);
}

RedirectRTSPServer::RedirectRTSPServer(UsageEnvironment& env, int ourSocket,
        Port ourPort, int mSocket, Port mPort,
        UserAuthenticationDatabase* authDatabase, unsigned reclamationTestSeconds)
    :RTSPServer(env, ourSocket, ourPort, authDatabase, reclamationTestSeconds),
    fMonitorServerSocket(mSocket), fMonitorServerPort(mPort){ 
        env.taskScheduler().turnOnBackgroundReadHandling(fMonitorServerSocket,
                (TaskScheduler::BackgroundHandlerProc*)&incomingConnectionHandlerMonitor, this);
}

RedirectRTSPServer::~RedirectRTSPServer() {
}
void RedirectRTSPServer::incomingConnectionHandlerMonitor(void* clientData, int mask){
    RedirectRTSPServer* server=(RedirectRTSPServer*)clientData;     
    server->incomingConnectionHandler();
}

void RedirectRTSPServer::incomingConnectionHandler()
{
    struct sockaddr_in agentAddr;
    SOCKLEN_T agentAddrLen = sizeof agentAddr;
    int agentSocket = accept(fMonitorServerSocket, (struct sockaddr*)&agentAddr, &agentAddrLen);
    if (agentSocket < 0) {
        int err = envir().getErrno();
        if (err != EWOULDBLOCK) {
            envir().setResultErrMsg("accept() failed: ");
        }
        return;
    }
    makeSocketNonBlocking(agentSocket);
    increaseSendBufferTo(envir(), agentSocket, 50*1024);
    cout<<"accept() success and agentSocket is "<<agentSocket<<endl; 
    (void)createNewAgentConnection(agentSocket,agentAddr);  
}

RedirectRTSPServer::AgentConnection
::AgentConnection(RedirectRTSPServer& ourServer, int agentSocket, struct sockaddr_in agentAddr)
    :fOurServer(ourServer),fAgentInputSocket(agentSocket),
    fAgentOutputSocket(agentSocket),fAgentAddr(agentAddr),fIsActive(True){
    resetBuffer();    
    envir().taskScheduler().setBackgroundHandling(fAgentInputSocket, SOCKET_READABLE|SOCKET_EXCEPTION,
                (TaskScheduler::BackgroundHandlerProc*)&incomingRequestHandler, this);
}

RedirectRTSPServer::AgentConnection
::~AgentConnection(){
    closeSockets();
}

void RedirectRTSPServer::AgentConnection
::resetBuffer(){
    bzero(fResponseBuffer,AGENT_BUFFER_SIZE);
    bzero(fRequestBuffer,AGENT_BUFFER_SIZE);
}

void RedirectRTSPServer::AgentConnection
::closeSockets() {
    envir().taskScheduler().disableBackgroundHandling(fAgentInputSocket);
    if (fAgentOutputSocket != fAgentInputSocket) 
        ::closeSocket(fAgentOutputSocket);
    ::closeSocket(fAgentInputSocket);

    fAgentInputSocket = fAgentOutputSocket = -1;
}

void RedirectRTSPServer::AgentConnection
::incomingRequestHandler(void* instance, int /*mask*/){
    AgentConnection* session=(AgentConnection*)instance;
    session->incomingRequestHandler1();
}

void RedirectRTSPServer::AgentConnection
::incomingRequestHandler1(){
    struct sockaddr_in dummy; 
    bzero(fRequestBuffer,AGENT_BUFFER_SIZE);
    int bytesRead = readSocket(envir(), fAgentInputSocket, fRequestBuffer, AGENT_BUFFER_SIZE, dummy);
    handleRequestBytes(bytesRead);
}
void RedirectRTSPServer::AgentConnection
::handleRequestBytes(int newBytesRead){
    if(newBytesRead==-1){
        fIsActive=False;
        cout<<"read socket error"<<endl;
        metricsSet->clearMetrics(fAgentAddr.sin_addr.s_addr);
        cout<<"read socket error1"<<endl;
        sourceSet->clearSource(fAgentAddr.sin_addr.s_addr);
        closeSockets();
        metricsSet->printMetrics();
        sourceSet->printSource();
    }
    else{
        char* ptrBytes=(char*)fRequestBuffer;
        cout<<ptrBytes<<endl;
        parseBytes(ptrBytes);
        metricsSet->printMetrics();
        sourceSet->printSource();
        //sourceSet->updatePrior();
    }
    if(!fIsActive){
        delete this;
    }
}
void RedirectRTSPServer::AgentConnection
::parseBytes(char *bytes){
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
void RedirectRTSPServer::AgentConnection
::parseSources(char *bytes){
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

RedirectRTSPServer::RedirectRTSPClientConnection
::RedirectRTSPClientConnection(RTSPServer& ourServer, int clientSocket, struct sockaddr_in clientAddr)
    :RTSPClientConnection(ourServer,clientSocket,clientAddr){
}

RedirectRTSPServer::RedirectRTSPClientConnection::~RedirectRTSPClientConnection(){

}

void RedirectRTSPServer::RedirectRTSPClientConnection::handleCmd_OPTIONS()
{
    ServerSource *source=sourceSet->lookupSource(urlSuffix);
    if(source==NULL){
        cout<<"handleCmd_bad()"<<endl;
        handleCmd_bad();
    }
    else{
        /*int second=time(NULL);
        if((second-source->fLastReqSec)>20){
            source->metricsset1->calculatePriorMetrics();
        }
        source->fLastReqSec=second;*/
        //source->metricsset1->roundRobinScheduling();
        sourceSet->updatePrior();
        snprintf((char*)fResponseBuffer, sizeof fResponseBuffer,
            "RTSP/1.0 301 Moved\r\nCSeq: %s\r\n"
            "%s"
            "Location: rtsp://%s:%d/%s\r\n"
            "Connection: Close\r\n\r\n",
            fCurrentCSeq,
            dateHeader(),
            source->metricsset1->prioraddr,
            source->metricsset1->priorport,
            urlSuffix);  
        //cout<<(char*)fResponseBuffer<<endl;
    }
}

void RedirectRTSPServer::RedirectRTSPClientConnection::handleCmd_DESCRIBE(char const* urlPreSuffix,char const* urlSuffix,char const* fullRequestStr){
    handleCmd_OPTIONS();
}

static ServerMediaSession* createNewSMS(UsageEnvironment& env,
        char const* fileName, FILE* fid); // forward

ServerMediaSession*
RedirectRTSPServer::lookupServerMediaSession(char const* streamName) {
    // First, check whether the specified "streamName" exists as a local file:
    FILE* fid = fopen(streamName, "rb");
    Boolean fileExists = fid != NULL;

    // Next, check whether we already have a "ServerMediaSession" for this file:
    ServerMediaSession* sms = RTSPServer::lookupServerMediaSession(streamName);
    Boolean smsExists = sms != NULL;

    // Handle the four possibilities for "fileExists" and "smsExists":
    if (!fileExists) {
        if (smsExists) {
            // "sms" was created for a file that no longer exists. Remove it:
            removeServerMediaSession(sms);
        }
        return NULL;
    } else {
        if (!smsExists) {
            // Create a new "ServerMediaSession" object for streaming from the named file.
            sms = createNewSMS(envir(), streamName, fid);
            addServerMediaSession(sms);
        }
        fclose(fid);
        return sms;
    }
}

// Special code for handling Matroska files:
static char newMatroskaDemuxWatchVariable;
static MatroskaFileServerDemux* demux;
static void onMatroskaDemuxCreation(MatroskaFileServerDemux* newDemux, void* /*clientData*/) {
    demux = newDemux;
    newMatroskaDemuxWatchVariable = 1;
}
// END Special code for handling Matroska files:

#define NEW_SMS(description) do {\
    char const* descStr = description\
    ", streamed by the LIVE555 Media Server";\
    sms = ServerMediaSession::createNew(env, fileName, fileName, descStr);\
} while(0)

static ServerMediaSession* createNewSMS(UsageEnvironment& env,
        char const* fileName, FILE* /*fid*/) {
    // Use the file name extension to determine the type of "ServerMediaSession":
    char const* extension = strrchr(fileName, '.');
    if (extension == NULL) return NULL;

    ServerMediaSession* sms = NULL;
    Boolean const reuseSource = False;
    if (strcmp(extension, ".aac") == 0) {
        // Assumed to be an AAC Audio (ADTS format) file:
        NEW_SMS("AAC Audio");
        sms->addSubsession(ADTSAudioFileServerMediaSubsession::createNew(env, fileName, reuseSource));
    } else if (strcmp(extension, ".amr") == 0) {
        // Assumed to be an AMR Audio file:
        NEW_SMS("AMR Audio");
        sms->addSubsession(AMRAudioFileServerMediaSubsession::createNew(env, fileName, reuseSource));
    } else if (strcmp(extension, ".ac3") == 0) {
        // Assumed to be an AC-3 Audio file:
        NEW_SMS("AC-3 Audio");
        sms->addSubsession(AC3AudioFileServerMediaSubsession::createNew(env, fileName, reuseSource));
    } else if (strcmp(extension, ".m4e") == 0) {
        // Assumed to be a MPEG-4 Video Elementary Stream file:
        NEW_SMS("MPEG-4 Video");
        sms->addSubsession(MPEG4VideoFileServerMediaSubsession::createNew(env, fileName, reuseSource));
    } else if (strcmp(extension, ".264") == 0) {
        // Assumed to be a H.264 Video Elementary Stream file:
        NEW_SMS("H.264 Video");
        OutPacketBuffer::maxSize = 100000; // allow for some possibly large H.264 frames
        sms->addSubsession(H264VideoFileServerMediaSubsession::createNew(env, fileName, reuseSource));
    } else if (strcmp(extension, ".mp3") == 0) {
        // Assumed to be a MPEG-1 or 2 Audio file:
        NEW_SMS("MPEG-1 or 2 Audio");
        // To stream using 'ADUs' rather than raw MP3 frames, uncomment the following:
        //#define STREAM_USING_ADUS 1
        // To also reorder ADUs before streaming, uncomment the following:
        //#define INTERLEAVE_ADUS 1
        // (For more information about ADUs and interleaving,
        //  see <http://www.live555.com/rtp-mp3/>)
        Boolean useADUs = False;
        Interleaving* interleaving = NULL;
#ifdef STREAM_USING_ADUS
        useADUs = True;
#ifdef INTERLEAVE_ADUS
        unsigned char interleaveCycle[] = {0,2,1,3}; // or choose your own...
        unsigned const interleaveCycleSize
            = (sizeof interleaveCycle)/(sizeof (unsigned char));
        interleaving = new Interleaving(interleaveCycleSize, interleaveCycle);
#endif
#endif
        sms->addSubsession(MP3AudioFileServerMediaSubsession::createNew(env, fileName, reuseSource, useADUs, interleaving));
    } else if (strcmp(extension, ".mpg") == 0) {
        // Assumed to be a MPEG-1 or 2 Program Stream (audio+video) file:
        NEW_SMS("MPEG-1 or 2 Program Stream");
        MPEG1or2FileServerDemux* demux
            = MPEG1or2FileServerDemux::createNew(env, fileName, reuseSource);
        sms->addSubsession(demux->newVideoServerMediaSubsession());
        sms->addSubsession(demux->newAudioServerMediaSubsession());
    } else if (strcmp(extension, ".ts") == 0) {
        // Assumed to be a MPEG Transport Stream file:
        // Use an index file name that's the same as the TS file name, except with ".tsx":
        unsigned indexFileNameLen = strlen(fileName) + 2; // allow for trailing "x\0"
        char* indexFileName = new char[indexFileNameLen];
        sprintf(indexFileName, "%sx", fileName);
        NEW_SMS("MPEG Transport Stream");
        sms->addSubsession(MPEG2TransportFileServerMediaSubsession::createNew(env, fileName, indexFileName, reuseSource));
        delete[] indexFileName;
    } else if (strcmp(extension, ".wav") == 0) {
        // Assumed to be a WAV Audio file:
        NEW_SMS("WAV Audio Stream");
        // To convert 16-bit PCM data to 8-bit u-law, prior to streaming,
        // change the following to True:
        Boolean convertToULaw = False;
        sms->addSubsession(WAVAudioFileServerMediaSubsession::createNew(env, fileName, reuseSource, convertToULaw));
    } else if (strcmp(extension, ".dv") == 0) {
        // Assumed to be a DV Video file
        // First, make sure that the RTPSinks' buffers will be large enough to handle the huge size of DV frames (as big as 288000).
        OutPacketBuffer::maxSize = 300000;

        NEW_SMS("DV Video");
        sms->addSubsession(DVVideoFileServerMediaSubsession::createNew(env, fileName, reuseSource));
    } else if (strcmp(extension, ".mkv") == 0 || strcmp(extension, ".webm") == 0) {
        // Assumed to be a Matroska file (note that WebM ('.webm') files are also Matroska files)
        NEW_SMS("Matroska video+audio+(optional)subtitles");

        // Create a Matroska file server demultiplexor for the specified file.  (We enter the event loop to wait for this to complete.)
        newMatroskaDemuxWatchVariable = 0;
        MatroskaFileServerDemux::createNew(env, fileName, onMatroskaDemuxCreation, NULL);
        env.taskScheduler().doEventLoop(&newMatroskaDemuxWatchVariable);

        ServerMediaSubsession* smss;
        while ((smss = demux->newServerMediaSubsession()) != NULL) {
            sms->addSubsession(smss);
        }
    }

    return sms;
}
