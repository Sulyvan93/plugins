#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
namespace trem {
struct PitchReading { double hz=0, confidence=0; };
// YIN-style normalized difference. Intended for isolated, monophonic sources.
inline PitchReading detectPitch(const float* input,int size,double sr) {
    if(size<256||sr<8000)return {};
    const int stride=std::max(1,(int)std::round(sr/24000.));
    const int n=std::min(4096,size/stride),maxLag=std::min(n/2,(int)(sr/stride/40.));
    const int minLag=std::max(2,(int)(sr/stride/2000.));
    std::array<double,4096> x{}; std::array<double,2049> diff{};
    double mean=0,power=0; for(int i=0;i<n;++i)mean+=input[i*stride];mean/=n;
    for(int i=0;i<n;++i){x[i]=input[i*stride]-mean;power+=x[i]*x[i];}
    if(power/n<1e-8)return {};
    const int window=n-maxLag;double cumulative=0;
    for(int lag=1;lag<=maxLag;++lag){double d=0;for(int i=0;i<window;++i){auto delta=x[i]-x[i+lag];d+=delta*delta;}cumulative+=d;diff[lag]=cumulative>1e-20?d*lag/cumulative:1.;}
    for(int lag=minLag;lag<maxLag;++lag)if(diff[lag]<.15){
        while(lag+1<maxLag&&diff[lag+1]<diff[lag])++lag;
        double denominator=diff[lag-1]-2*diff[lag]+diff[lag+1];
        double shift=std::abs(denominator)>1e-12?.5*(diff[lag-1]-diff[lag+1])/denominator:0;
        return {sr/stride/(lag+std::clamp(shift,-.5,.5)),std::clamp(1-diff[lag],0.,1.)};
    }return {};
}
struct TempoReading {double bpm=0,confidence=0;};
// Autocorrelation of positive energy flux sampled at 200 Hz. Runs off the audio thread.
inline TempoReading detectTempo(const float* envelope,int count) {
    if(count<1200)return {};
    count=std::min(count,4096);std::array<double,4096> onset{};double sum=0,power=0;
    for(int i=1;i<count;++i){onset[i]=std::max(0.,(double)envelope[i]-envelope[i-1]);sum+=onset[i];power+=onset[i]*onset[i];}
    if(power<1e-10||sum<1e-6)return {};
    auto flux=onset;sum=0;
    for(int i=2;i<count-2;++i){onset[i]=(flux[i-2]+2*flux[i-1]+3*flux[i]+2*flux[i+1]+flux[i+2])/9.;sum+=onset[i];}
    const double mean=sum/count;for(int i=0;i<count;++i)onset[i]-=mean;
    double best=0;int bestLag=0;std::array<double,201> correlations{};
    for(int lag=60;lag<=200;++lag){double a=0,b=0,c=0;for(int i=lag;i<count;++i){a+=onset[i]*onset[i-lag];b+=onset[i]*onset[i];c+=onset[i-lag]*onset[i-lag];}
        double correlation=a/std::sqrt(std::max(1e-20,b*c));
        // Prefer the shortest period when several exact multiples agree.
        correlations[lag]=correlation;
        if(correlation>best){best=correlation;bestLag=lag;}
    }
    if(best<.50)return {};
    // Resolve exact-period multiples only at genuine local peaks. The old
    // running +0.06 threshold incorrectly favored the first lag (200 BPM).
    for(int lag=61;lag<bestLag;++lag){double ratio=(double)bestLag/lag;
        if(std::abs(ratio-std::round(ratio))<.04 && ratio>=1.9 && ratio<=4.1 && correlations[lag]>=best-.06 && correlations[lag]>=correlations[lag-1] && correlations[lag]>=correlations[lag+1]){bestLag=lag;best=correlations[lag];break;}}
    return {12000./bestLag,std::clamp(best,0.,1.)};
}
// Consensus across successive one-second windows. Auto uses the 60–120 BPM
// octave; the UI explicitly exposes /2 and x2 because audio cannot always
// distinguish beat vs subdivision. No value is fabricated from host tempo.
class TempoTracker {
public:
    void reset(){history={};position=0;misses=0;reading={};}
    TempoReading update(TempoReading candidate){
        if(candidate.bpm>0&&candidate.confidence>=.65){
            while(candidate.bpm>120)candidate.bpm*=.5;
            while(candidate.bpm<60)candidate.bpm*=2;
            misses=0;
        }else{candidate={};++misses;}
        history[position++%history.size()]=candidate;
        if(misses>=4){reading={};return reading;}
        int bestCount=0;double bestWeight=0;TempoReading best{};
        for(const auto& anchor:history){if(anchor.bpm<=0)continue;int count=0;double sum=0,weight=0;
            for(const auto& item:history)if(item.bpm>0&&std::abs(item.bpm-anchor.bpm)<=anchor.bpm*.035){++count;sum+=item.bpm*item.confidence;weight+=item.confidence;}
            if(count>=3&&(count>bestCount||(count==bestCount&&weight>bestWeight))){bestCount=count;bestWeight=weight;best={sum/weight,weight/count};}}
        if(bestCount>=3)reading=best;
        return reading;
    }
private:
    std::array<TempoReading,8> history{};size_t position=0;int misses=0;TempoReading reading{};
};
inline float heldPeak(float input,float previous,float& age,float dt,float holdSeconds,float returnRate,bool infinite) {
    if(input>=previous){age=0;return input;}age+=dt;
    return infinite||age<holdSeconds?previous:std::max(input,previous-returnRate*dt);
}
}
