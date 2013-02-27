#ifndef _AGENT_HH
#define _AGENT_HH

struct thr_parameter{
    int proxyServerPort;
    char *scheduleServerAddr;
    int scheduleServerPort;
    char *sources;
};
void paraseUrl(char *url,char *source);
void *thr_agent(void *arg);

#endif
