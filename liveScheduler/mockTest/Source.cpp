#include "Source.hh"

extern MetricsSet *metricsSet;

ServerSource::ServerSource(ServerSource *nextSource){
    if(nextSource==this){
       fNextSource=fPreSource=this;
    }
    else{
       fPreSource=nextSource->fPreSource;
       fNextSource=nextSource;
       nextSource->fPreSource=this;
       fPreSource->fNextSource=this;
       metricsset1=new MetricsSet1();
    }
    fLastReqSec=0;
}

ServerSource::~ServerSource(){
    fPreSource->fNextSource=fNextSource;
    fNextSource->fPreSource=fPreSource;
    delete metricsset1;
}

SourceSet::SourceSet()
    :fSource(&fSource){

}

SourceSet::~SourceSet(){

}

void SourceSet::assignSource(char* src,uint32_t addr){
    ServerSource *source=lookupSource(src);
    if(source==NULL){
        source=new ServerSource(fSource.fNextSource);
        source->serversrc=string(src);
    }
    Metricsdata *metricsdata=metricsSet->lookupMetrics(addr);
    if(metricsdata!=NULL){
        source->metricsset1->assignMetrics(metricsdata);
    }
}

void SourceSet::clearSource(uint32_t addr){
    ServerSource* source;
    SourceIterator iter(*this);
    while((source=iter.next())!=NULL){
        source->metricsset1->clearMetrics(addr);
        if(source->metricsset1->sources.empty()){
            delete source;
        }
        else{
            source->metricsset1->calculatePriorMetrics();
        }
    }        
}

void SourceSet::clearSource(char* src){
    ServerSource* source=lookupSource(src);
    if(source!=NULL)
        delete source;
}

ServerSource* SourceSet::lookupSource(char* src){
    ServerSource* source;
    SourceIterator iter(*this);
    while((source=iter.next())!=NULL){
        if(source->serversrc==string(src)) break;
    }
    return source;
}
void SourceSet::printSource(){
    ServerSource* source;
    SourceIterator iter(*this);
    cout<<"*********printSource()**********"<<endl;
    while((source=iter.next())!=NULL){
        cout<<source->serversrc<<endl;
    }
    cout<<"*******************************"<<endl;
}
void SourceSet::updatePrior(){
    ServerSource* source;
    SourceIterator iter(*this);
    while((source=iter.next())!=NULL){
        source->metricsset1->calculatePriorMetrics();
    }
}

SourceIterator::SourceIterator(SourceSet& sourceSet)
    :fOurSet(sourceSet) {
    reset();
}

SourceIterator::~SourceIterator(){
}

ServerSource* SourceIterator::next(){
    ServerSource* result=fNextPtr;
    if(result == &fOurSet.fSource){
        result=NULL;
    }
    else{
        fNextPtr=fNextPtr->fNextSource;
    }
    return result;
}

void SourceIterator::reset(){
    fNextPtr=fOurSet.fSource.fNextSource;
}
