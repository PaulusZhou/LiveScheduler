#include "Metrics.hh"

Metricsdata::Metricsdata(Metricsdata *nextMetrics)
    :agentaddr(0),agentport(554),responsetime(0){
    if(nextMetrics==this){
       fNextMetrics=fPreMetrics=this;
    }
    else{
       fPreMetrics=nextMetrics->fPreMetrics;
       fNextMetrics=nextMetrics;
       nextMetrics->fPreMetrics=this;
       fPreMetrics->fNextMetrics=this;
    }
}

Metricsdata::~Metricsdata(){
    fNextMetrics->fPreMetrics=fPreMetrics;
    fPreMetrics->fNextMetrics=fNextMetrics;
    pthread_cancel(tid);
}

MetricsSet::MetricsSet()
    :fMetrics(&fMetrics){
    fMetrics.agentaddr=0;
}

MetricsSet::~MetricsSet(){
    while(fMetrics.fNextMetrics!=&fMetrics)
        delete fMetrics.fNextMetrics;
}

void MetricsSet::assignMetrics(uint32_t addr,struct loadinfo info){
    Metricsdata* metrics=lookupMetrics(addr);
    if(metrics==NULL){
        metrics=new Metricsdata(fMetrics.fNextMetrics);
        metrics->info=info;
        strcpy(metrics->info.testsrc,info.testsrc);
        struct in_addr in;
        in.s_addr=addr;
        metrics->ptragentaddr=inet_ntoa(in);
        sprintf(metrics->url,"rtsp://%s:%d/%s",metrics->ptragentaddr,info.agentport,metrics->info.testsrc);
        metrics->param.url=metrics->url;
        cout<<"thread start with url:"<<metrics->param.url<<endl;
        metrics->param.responsetime= &(metrics->responsetime);
        pthread_create(&(metrics->tid),NULL,thr_getresponsetime,(void*)&(metrics->param));
        metrics->agentaddr=addr;
    }
    else{
        metrics->info.cpu_idle=info.cpu_idle;
        metrics->info.concurrecy=info.concurrecy;
        metrics->info.net_rates=info.net_rates;
    }
}

void MetricsSet::clearMetrics(uint32_t addr){
    Metricsdata* metrics=lookupMetrics(addr);
    if(metrics!=NULL)
    {
        cout<<"delete metrics"<<endl;
        delete metrics;
    }
}

Metricsdata* MetricsSet::lookupMetrics(uint32_t addr){
    Metricsdata* metrics;
    MetricsIterator iter(*this);
    while((metrics=iter.next())!=NULL){
        if(metrics->agentaddr==addr) break;
    }
    return metrics;
}
#ifdef PRINTMETRICS
void MetricsSet::printMetrics(){
    Metricsdata* metrics;
    MetricsIterator iter(*this);
    cout<<"***********METRICS*************"<<endl;
    while((metrics=iter.next())!=NULL){
        struct in_addr in;
        in.s_addr=metrics->agentaddr;
        cout<<"agentaddr:"<<inet_ntoa(in)<<endl;
        cout<<"cpu_num:"<<metrics->info.cpu_num<<endl;
        cout<<"cpu_speed:"<<metrics->info.cpu_speed<<endl;
        cout<<"cpu_idle:"<<metrics->info.cpu_idle<<endl;
        cout<<"concurrecy:"<<metrics->info.concurrecy<<endl;
        cout<<"net_rates:"<<metrics->info.net_rates<<endl;
        cout<<"responsetime:"<<metrics->responsetime<<endl;
    }
    cout<<"*******************************"<<endl;
}
#endif

MetricsIterator::MetricsIterator(MetricsSet& metricsSet)
    :fOurSet(metricsSet) {
    reset();
}

MetricsIterator::~MetricsIterator(){
}

Metricsdata* MetricsIterator::next(){
    Metricsdata* result=fNextPtr;
    if(result == &fOurSet.fMetrics){
        result=NULL;
    }
    else{
        fNextPtr=fNextPtr->fNextMetrics;
    }
    return result;
}

void MetricsIterator::reset(){
    fNextPtr=fOurSet.fMetrics.fNextMetrics;
}


MetricsSet1::MetricsSet1()
    :fMetricsNum(0){
    reset();
    fCpuWeight=0;
    fConcurrecyWeight=0;
    fNetRateWeight=0;
    fResponsetimeWeight=0;
}

MetricsSet1::~MetricsSet1(){
    sources.clear();    
}

void MetricsSet1::assignMetrics(Metricsdata *metrics){
   It it=sources.find(metrics->agentaddr);
   if(it==sources.end()){
        sources.insert(v_t(metrics->agentaddr,metrics));
        lastassign=sources.begin();
        fMetricsNum++;
        printMetrics();
   }
   if(fMetricsNum==1){
       setPriorSocket(metrics);
   }   
}

void MetricsSet1::setPriorSocket(Metricsdata *metrics){
    struct in_addr in;
    in.s_addr=metrics->agentaddr;
    char *temp=inet_ntoa(in);
    cout<<"prioraddr is "<<temp<<endl;
    strcpy(prioraddr,temp);
    priorport=metrics->agentport;
}

void MetricsSet1::setPriorSocket(uint32_t addr){
    struct in_addr in;
    in.s_addr=addr;
    char *temp=inet_ntoa(in);
    cout<<"prioraddr is "<<temp<<endl;
    strcpy(prioraddr,temp);
}

void MetricsSet1::clearMetrics(uint32_t addr){
    It it=sources.find(addr);
    if(it!=sources.end()){
        sources.erase(it);
        fMetricsNum--;
    }
}

void MetricsSet1::printMetrics(){
    for(It it=sources.begin();it!=sources.end();it++)
    {
        Metricsdata *metrics=it->second;
        cout<<"metrics addr:"<<metrics->agentaddr<<":"<<metrics->agentport<<endl;
    }
}

void MetricsSet1::reset(){
    fCpuNumMax=0;
    fCpuSpeedMax=0;
    fCpuUsedMin=0;
    fConcurrecyMin=0;
    fNetRateMin=0;
    fResponsetimeMin=0;
    fCpuUsedMax=0;
    fConcurrecyMax=0;
    fNetRateMax=0;
    fResponsetimeMax=0;

}

void MetricsSet1::roundRobinScheduling(){
    Metricsdata* priormetrics=NULL;
    if(fMetricsNum<=1){
        if(!sources.empty()){
            lastassign=sources.begin();    
        }
        else{
            return;
        }
    }
    else{
        if(lastassign==sources.end()){
            lastassign=sources.begin();
        }
        else{
            lastassign++;
            if(lastassign==sources.end()){
                lastassign=sources.begin();
            }
        }
    }
    priormetrics=lastassign->second;
    setPriorSocket(priormetrics);
}

void MetricsSet1::calculateMinAndMax(){
    Metricsdata *metrics=NULL;
    reset();
    r.clear();
    struct info i;
    for(It it=sources.begin();it!=sources.end();it++){
        metrics=it->second;
        if(it==sources.begin()){
            fCpuNumMax=metrics->info.cpu_num;
            fCpuSpeedMax=metrics->info.cpu_speed;
            fConcurrecyMin=fConcurrecyMax=metrics->info.concurrecy;
            fNetRateMin=fNetRateMax=metrics->info.net_rates;
            fResponsetimeMin=fResponsetimeMax=metrics->responsetime;
        }
        else{
            fCpuNumMax=fCpuNumMax>metrics->info.cpu_num?fCpuNumMax:metrics->info.cpu_num;
            fCpuSpeedMax=fCpuSpeedMax>metrics->info.cpu_speed?fCpuSpeedMax:metrics->info.cpu_speed;

            fConcurrecyMin=fConcurrecyMin<metrics->info.concurrecy?fConcurrecyMin:metrics->info.concurrecy;
            fNetRateMin=fNetRateMin<metrics->info.net_rates?fNetRateMin:metrics->info.net_rates;
            fResponsetimeMin=fResponsetimeMin<metrics->responsetime?fResponsetimeMin:metrics->responsetime;
            fConcurrecyMax=fConcurrecyMax>metrics->info.concurrecy?fConcurrecyMax:metrics->info.concurrecy;
            fNetRateMax=fNetRateMax>metrics->info.net_rates?fNetRateMax:metrics->info.net_rates;
            fResponsetimeMax=fResponsetimeMax>metrics->responsetime?fResponsetimeMax:metrics->responsetime;
        }
        i.cpu_used=0;
        i.concurrecy=metrics->info.concurrecy;
        i.net_rates=metrics->info.net_rates;
        i.responsetime=metrics->responsetime;
        r.insert(Regulation::value_type(metrics->agentaddr,i));
    }
    float cpu_used=0;
    Regulation::iterator itr=r.begin();
    for(It it=sources.begin();it!=sources.end()&&itr!=r.end();it++,itr++){
        metrics=it->second;
        cpu_used=(metrics->info.cpu_idle)*((float)metrics->info.cpu_num/fCpuNumMax)*((float)metrics->info.cpu_speed/fCpuSpeedMax); 
        cpu_used=1-cpu_used;
        if(it==sources.begin()){
            fCpuUsedMin=fCpuUsedMax=cpu_used;
        }
        else{
            fCpuUsedMin=fCpuUsedMin<cpu_used?fCpuUsedMin:cpu_used;
            fCpuUsedMax=fCpuUsedMax>cpu_used?fCpuUsedMax:cpu_used;
        }
        itr->second.cpu_used=cpu_used;
    }
    cout<<"fCpuUsedMin "<<fCpuUsedMin<<" fCpuUsedMax "<<fCpuUsedMax<<endl;
    cout<<"fConcurrecyMin "<<fConcurrecyMin<<" fConcurrecyMax "<<fConcurrecyMax<<endl;
    cout<<"fNetRateMin "<<fNetRateMin<<" fNetRateMax "<<fNetRateMax<<endl;
    cout<<"fResponsetimeMin "<<fResponsetimeMin<<" fResponsetimeMax "<<fResponsetimeMax<<endl;
}

void MetricsSet1::regulationData(){
    float cpu_diff=fCpuUsedMax-fCpuUsedMin;
    float concurrecy_diff=fConcurrecyMax-fConcurrecyMin;
    float netrate_diff=fNetRateMax-fNetRateMin;
    float responsetime_diff=fResponsetimeMax-fResponsetimeMin;
    for(Regulation::iterator it=r.begin();it!=r.end();it++){
        if(cpu_diff!=0){
            it->second.cpu_used=((float)fCpuUsedMax-(float)it->second.cpu_used)/(float)cpu_diff;
        }
        else{
            it->second.cpu_used=0;
        }
        if(concurrecy_diff!=0){
            it->second.concurrecy=((float)fConcurrecyMax-(float)it->second.concurrecy)/(float)concurrecy_diff;
        }
        else{
            it->second.concurrecy=0;
        }
        if(netrate_diff!=0){ 
            it->second.net_rates=(fNetRateMax-it->second.net_rates)/netrate_diff;
        }
        else{
            it->second.net_rates=0;
        }
        if(responsetime_diff!=0){
            it->second.responsetime=(fResponsetimeMax-it->second.responsetime)/responsetime_diff;
        }
        else{
            it->second.responsetime=0;
        }
        if(it==r.begin()){
            RCpuUsedMin=it->second.cpu_used;
            RConcurrecyMin=it->second.concurrecy;
            RNetRateMin=it->second.net_rates;
            RResponsetimeMin=it->second.responsetime;
        }
        else{
            RCpuUsedMin=RCpuUsedMin<it->second.cpu_used?RCpuUsedMin:it->second.cpu_used;
            RConcurrecyMin=RConcurrecyMin<it->second.concurrecy?RConcurrecyMin:it->second.concurrecy;
            RNetRateMin=RNetRateMin<it->second.net_rates?RNetRateMin:it->second.net_rates;
            RResponsetimeMin=RResponsetimeMin<it->second.responsetime?RResponsetimeMin:it->second.responsetime;
        }
        cout<<"\n__________________________________"<<endl;
        cout<<"addr "<<it->first<<endl;
        cout<<"cpu_used "<<it->second.cpu_used<<endl;
        cout<<"concurrecy "<<it->second.concurrecy<<endl;
        cout<<"netrates "<<it->second.net_rates<<endl;
        cout<<"responsetime "<<it->second.responsetime<<endl;
        cout<<"___________________________________\n"<<endl;
    }
}

void MetricsSet1::calculateWeight(){
    float cpu_weight=0,concurrecy_weight=0,netrate_weight=0,responsetime_weight=0;
    if(r.empty()){
        cout<<"r.empty() is true"<<endl;
        return;
    }
    for(Regulation::iterator it=r.begin();it!=r.end();it++){
        cpu_weight+=it->second.cpu_used-RCpuUsedMin;
        concurrecy_weight+=it->second.concurrecy-RConcurrecyMin;
        netrate_weight+=it->second.net_rates-RNetRateMin;
        responsetime_weight+=it->second.responsetime-RResponsetimeMin;
    }
    float weight_total=cpu_weight+concurrecy_weight+netrate_weight+responsetime_weight;
    cout<<"weight_total "<<weight_total<<endl;
    fCpuWeight=cpu_weight/weight_total;
    fConcurrecyWeight=concurrecy_weight/weight_total;
    fNetRateWeight=netrate_weight/weight_total;
    fResponsetimeWeight=responsetime_weight/weight_total;
    cout<<"fCpuWeight "<<fCpuWeight
        <<" fConcurrecyWeight "<<fConcurrecyWeight
        <<" fNetRateWeight "<<fNetRateWeight
        <<" fResponsetimeWeight "<<fResponsetimeWeight<<endl;
}

uint32_t MetricsSet1::calculatePriorAddr(){
    uint32_t prioraddr=0;
    float aggregrateload=0,load=0;
    for(Regulation::iterator it=r.begin();it!=r.end();it++){
        load=fCpuWeight*it->second.cpu_used
            +fConcurrecyWeight*it->second.concurrecy
            +fNetRateWeight*it->second.net_rates
            +fResponsetimeWeight*it->second.responsetime;
        if(it==r.begin()){
            aggregrateload=load;
            prioraddr=it->first;
        }
        else if(aggregrateload<load){
            aggregrateload=load;
            prioraddr=it->first;
        }
        cout<<"addr:"<<it->first<<" and load "<<load<<endl;;
    }
    cout<<"prioraddr is "<<prioraddr<<endl;
    return prioraddr;
}

void MetricsSet1::calculatePriorMetrics(){
    Metricsdata* priormetrics=NULL;
    if(fMetricsNum<=1){
        if(!sources.empty()){
            priormetrics=sources.begin()->second;    
            setPriorSocket(priormetrics);
        }
    }
    else{
        calculateMinAndMax();
        regulationData();
        calculateWeight();
        uint32_t prioraddr=calculatePriorAddr();
        setPriorSocket(prioraddr);
    }
}

