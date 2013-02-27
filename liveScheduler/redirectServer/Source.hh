#ifndef _SOURCE_HH_
#define _SOURCE_HH_

#include "Metrics.hh"
#include <string>
using namespace std;

class ServerSource
{
public:
    ServerSource(ServerSource *nextSource);
    virtual ~ServerSource();
public:
    string serversrc;
    MetricsSet1 *metricsset1;
    int fLastReqSec;
private:
    friend class SourceSet;
    friend class SourceIterator;
    ServerSource* fPreSource;
    ServerSource* fNextSource;
};

class SourceSet
{
public:
    SourceSet();
    virtual ~SourceSet();
    void assignSource(char* src,uint32_t addr);
    void clearSource(uint32_t addr);
    void clearSource(char* src);
    ServerSource* lookupSource(char* src);
    void printSource();
    void updatePrior();
private:
    friend class SourceIterator;
    ServerSource fSource;
};

class SourceIterator
{
public:
    SourceIterator(SourceSet& sourceSet);
    virtual ~SourceIterator();
    ServerSource* next();
    void reset();
private:
    SourceSet& fOurSet;
    ServerSource* fNextPtr;
};

#endif
