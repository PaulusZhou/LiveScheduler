#ifndef METRICS_HH
#define METRICS_HH

#include <inttypes.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include "FakeRTSPClient.hh"
#include <map>

#define PRINTMETRICS
#ifdef PRINTMETRICS
#include<netinet/in.h>
#include<arpa/inet.h>
#include <iostream>
using namespace std;
#endif

#define SIZE_URL 128

struct loadinfo{
    int cpu_num;
    int cpu_speed;
    float cpu_idle;
    int concurrecy;
    float net_rates;
    uint16_t agentport;
    char testsrc[SIZE_URL];
};

class Metricsdata
{
    Metricsdata(Metricsdata *nextMetrics);
    virtual ~Metricsdata();
public:
    uint32_t agentaddr; 
    char *ptragentaddr;
    uint16_t agentport;
    struct loadinfo info;
    long responsetime;
    pthread_t tid;
    char url[SIZE_URL];
    struct thrparameter param;
private:
    friend class MetricsSet;
    friend class MetricsSet1;
    friend class MetricsIterator;
    friend class MetricsIterator1;
    Metricsdata *fNextMetrics;
    Metricsdata *fPreMetrics;
};
    
class MetricsSet
{
public:
   MetricsSet();
   virtual ~MetricsSet();
   void assignMetrics(uint32_t addr,struct loadinfo info);
   void clearMetrics(uint32_t addr);
   Metricsdata* lookupMetrics(uint32_t addr);
#ifdef PRINTMETRICS
   void printMetrics();
#endif
private:
   friend class MetricsIterator;
   Metricsdata fMetrics;
};
class MetricsIterator
{
public:
    MetricsIterator(MetricsSet& metricsSet);
    virtual ~MetricsIterator();
    Metricsdata* next();
    void reset();
private:
    MetricsSet& fOurSet;
    Metricsdata* fNextPtr;
};
struct info{
    float cpu_used;
    float concurrecy;
    float net_rates;
    float responsetime;
};
typedef map<uint32_t,struct info> Regulation;
typedef map<uint32_t,Metricsdata*> Sources;
typedef Sources::value_type v_t;
typedef Sources::iterator It;
class MetricsSet1
{
public:
   MetricsSet1();
   virtual ~MetricsSet1();
   void assignMetrics(Metricsdata *metrics);
   void clearMetrics(uint32_t addr);
   void printMetrics();
   void reset();
   void roundRobinScheduling();
   void setPriorSocket(Metricsdata *metrics);
   void setPriorSocket(uint32_t addr);
   void calculateMinAndMax();
   void regulationData();
   void calculateWeight();
   uint32_t calculatePriorAddr();
   void calculatePriorMetrics();
public:
   Sources sources;
   Regulation r;
   char prioraddr[16];
   int priorport;
   float fMetricsNum;
   It lastassign;
private:
   float fCpuNumMax;
   float fCpuSpeedMax;
   float fCpuUsedMin;
   float fConcurrecyMin;
   float fNetRateMin;
   float fResponsetimeMin;

   float fCpuUsedMax;
   float fConcurrecyMax;
   float fNetRateMax;
   float fResponsetimeMax;

   float RCpuUsedMin;
   float RConcurrecyMin;
   float RNetRateMin;
   float RResponsetimeMin;
   float fCpuWeight;
   float fConcurrecyWeight;
   float fNetRateWeight;
   float fResponsetimeWeight;
};

#endif
