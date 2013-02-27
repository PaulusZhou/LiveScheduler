#ifndef _REDIRECT_RTSP_SERVER_HH
#define _REDIRECT_RTSP_SERVER_HH

#include "RTSPServer.hh"
#include "Metrics.hh"

#define AGENT_BUFFER_SIZE 1024

class RedirectRTSPServer: public RTSPServer {
public:
  static RedirectRTSPServer* createNew(UsageEnvironment& env, Port ourPort, Port mPort, 
				      UserAuthenticationDatabase* authDatabase,
				      unsigned reclamationTestSeconds = 65);

protected:
  RedirectRTSPServer(UsageEnvironment& env, int ourSocket, Port ourPort, int mSocket, Port mPort, 
		    UserAuthenticationDatabase* authDatabase, unsigned reclamationTestSeconds);
  // called only by createNew();
  virtual ~RedirectRTSPServer();

protected: // redefined virtual functions
  virtual ServerMediaSession* lookupServerMediaSession(char const* streamName);

public:
  class AgentConnection
  {
  public:
      AgentConnection(RedirectRTSPServer& ourServer, int agentSocket, struct sockaddr_in agentAddr);
      virtual ~AgentConnection();
  protected:
      UsageEnvironment& envir() { return fOurServer.envir(); }
      void resetBuffer();
      void closeSockets();
      static void incomingRequestHandler(void*, int /*mask*/);
      void incomingRequestHandler1();
      void handleRequestBytes(int newBytesRead);
      void parseBytes(char *bytes);
      void parseSources(char *bytes);
  protected:
      RedirectRTSPServer& fOurServer;
      int fAgentInputSocket,fAgentOutputSocket;
      struct sockaddr_in fAgentAddr;
      Boolean fIsActive;
      struct loadinfo info;
      unsigned char fRequestBuffer[AGENT_BUFFER_SIZE];
      unsigned char fResponseBuffer[AGENT_BUFFER_SIZE];
  };
 
  class RedirectRTSPClientConnection:public RTSPClientConnection
  {
  public:
      RedirectRTSPClientConnection(RTSPServer& ourServer, int clientSocket, struct sockaddr_in clientAddr);
      virtual ~RedirectRTSPClientConnection();
  protected:
      friend class RTSPClientSession;
      virtual void handleCmd_OPTIONS();
      virtual void handleCmd_DESCRIBE(char const* urlPreSuffix,char const* urlSuffix,char const* fullRequestStr);
  };
protected:
  virtual RTSPClientConnection*
  createNewClientConnection(int clientSocket, struct sockaddr_in clientAddr)
  {
      return new RedirectRTSPClientConnection(*this,clientSocket,clientAddr);
  }
  AgentConnection* 
  createNewAgentConnection(int agentSocket, struct sockaddr_in agentAddr)
  {
      return new AgentConnection(*this,agentSocket,agentAddr);
  }
private:
  static void incomingConnectionHandlerMonitor(void* clientData, int mask);
  void incomingConnectionHandler();
private:
  friend class RedirectRTSPClientConnection;
  friend class RTSPClientSession;
  int fMonitorServerSocket;
  Port fMonitorServerPort;
};

#endif
