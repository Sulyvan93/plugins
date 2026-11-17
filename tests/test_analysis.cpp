#ifdef NDEBUG
#undef NDEBUG
#endif
#include "AnalysisTools.h"
#include "MeterDSP.h"
#include <cassert>
#include <cstdio>
#include <vector>
int main(){
    constexpr double pi=3.14159265358979323846;
    for(double sr:{44100.,48000.,96000.})for(double hz:{55.,110.,220.,440.,880.,1000.}){
        std::array<float,4096> x{};for(int i=0;i<4096;++i)x[i]=(float)(.3*std::sin(2*pi*hz*i/sr)+.07*std::sin(4*pi*hz*i/sr));
        auto p=trem::detectPitch(x.data(),4096,sr);double cents=1200*std::log2(p.hz/hz);
        std::printf("Pitch %.0f Hz @ %.0f: %.3f Hz, error %.2f ct\n",hz,sr,p.hz,cents);assert(p.confidence>.8&&std::abs(cents)<2.);
    }
    std::array<float,4096> silence{};assert(trem::detectPitch(silence.data(),4096,48000).hz==0);
    for(double bpm:{72.,90.,120.,150.,180.}){
        std::array<float,4000> env{};for(int i=0;i<4000;++i){double phase=std::fmod(i/200.,60./bpm);env[i]=(float)std::exp(-phase*45.);}
        auto t=trem::detectTempo(env.data(),4000);std::printf("Tempo %.0f -> %.2f BPM, confidence %.2f\n",bpm,t.bpm,t.confidence);assert(t.confidence>.25&&std::abs(t.bpm-bpm)<2.);
    }
    assert(trem::detectTempo(silence.data(),4096).bpm==0);
    assert(trem::detectTempo(silence.data(),500).bpm==0);
    std::array<float,4096> noise{};unsigned seed=123;for(auto& v:noise){seed=1664525*seed+1013904223;v=(float)((double)seed/4294967295.-.5);}
    assert(trem::detectPitch(noise.data(),4096,48000).hz==0);
    float age=0;float held=trem::heldPeak(-20,-10,age,.5f,1,12,false);assert(held==-10);held=trem::heldPeak(-20,held,age,.6f,1,12,false);assert(held<-10&&held>=-20);
    held=trem::heldPeak(-30,held,age,2,0,24,true);assert(held>-30);
    trem::MeterEngine m;m.prepare(48000,2);std::array<float,256> tone{};for(int i=0;i<256;++i)tone[i]=.2f*std::sin(2*pi*1000*i/48000.);const float* channels[]={tone.data(),tone.data()};for(int i=0;i<1000;++i)m.process(channels,2,256);
    auto before=m.snapshot();m.setLoudnessPaused(true);std::array<float,256> zero{};const float* zeros[]={zero.data(),zero.data()};for(int i=0;i<1000;++i)m.process(zeros,2,256);auto after=m.snapshot();assert(before.integrated==after.integrated);assert(before.completedHops==after.completedHops);assert(before.elapsedSeconds==after.elapsedSeconds);assert(after.rmsDb[0]<-60);m.setLoudnessPaused(false);m.process(channels,2,256);assert(m.snapshot().elapsedSeconds>after.elapsedSeconds);
    trem::TempoTracker tracker;
    assert(tracker.update({160,.7}).bpm==0);
    assert(tracker.update({161,.72}).bpm==0);
    auto consensus=tracker.update({159,.73});assert(std::abs(consensus.bpm-80)<1);
    for(int i=0;i<4;++i)consensus=tracker.update({});assert(consensus.bpm==0);
    for(int i=0;i<8;++i){tracker.reset();assert(tracker.update({200,.2}).bpm==0);}
    std::puts("ANALYSIS TESTS PASSED");
}
