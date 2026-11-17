#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace {
juce::NormalisableRange<float> range(float lo,float hi,float step=0.1f) {
    return juce::NormalisableRange<float>(lo,hi,step);
}
}
juce::AudioProcessorValueTreeState::ParameterLayout TremDeMedirProcessor::makeParameters() {
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"target",1}, "Alvo LUFS",range(-36,-6),-14.0f));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"ceiling",1}, "Teto True Peak",range(-12,0),-1.0f));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"gate",1}, "Gate Integrado",juce::StringArray{"Desligado","-8 LU","-10 LU"},2));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"truepeak",1},"True Peak HQ 8x",false));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"history",1}, juce::String::fromUTF8("Captação Histórico"),juce::StringArray{juce::String::fromUTF8("Contínuo"),juce::String::fromUTF8("Somente reprodução"),"Reset no Play"},0));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"meterstyle",1},"Escala Digital",juce::StringArray{juce::String::fromUTF8("Não linear"),"Linear","K-20","K-14","K-12"},0));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"vuRef",1},juce::String::fromUTF8("Referência 0 VU dBFS"),range(-30,-6,0.5f),-18.0f));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"analog",1},juce::String::fromUTF8("Tipo Analógico"),juce::StringArray{"VU","PPM"},0));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"weighting",1},juce::String::fromUTF8("Ponderação RTA"),juce::StringArray{"Nenhuma","A","C"},0));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"rtaSource",1},"Fonte RTA",juce::StringArray{"L+R","Esquerda","Direita"},0));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"lfe",1},juce::String::fromUTF8("Posição LFE"),juce::StringArray{juce::String::fromUTF8("4º canal (ITU)"),juce::String::fromUTF8("6º canal (Logic)"),"Sem LFE"},0));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"alerts",1},"Alertas",true));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"logging",1},juce::String::fromUTF8("CSV automático"),false));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"logInterval",1},"Intervalo log",juce::StringArray{"1 s","5 s","10 s","30 s"},1));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"measurePause",1},"Pausar loudness",false));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"loudUnits",1},"Unidade loudness",juce::StringArray{"LUFS","LU relativo"},0));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"bandCount",1},"Bandas RTA",juce::StringArray{"31 bandas","63 bandas","127 bandas"},1));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"rtaMode",1},"Detector RTA",juce::StringArray{"Peak","RMS Fast","RMS Slow"},1));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"levelMode",1},"Leitura de nivel",juce::StringArray{"Peak + RMS","Peak","RMS"},0));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"holdTime",1},"Retencao de picos",juce::StringArray{"0 s","1 s","2 s","5 s"},2));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"returnRate",1},"Retorno dB/s",juce::StringArray{"6 dB/s","12 dB/s","24 dB/s"},1));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"peakHold",1},"Hold infinito",false));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"rtaRange",1},"Faixa RTA",juce::StringArray{"30 dB","60 dB","90 dB","120 dB"},2));
    p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"tuneRef",1},"Referencia A4 Hz",range(400,480,1),440.f));
    p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"lrMax",1},"Deteccao LR max",false));
    p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"tempoOctave",1},"Oitava BPM",juce::StringArray{"Auto 60-120","/2","x2"},0));
    return p;
}
TremDeMedirProcessor::TremDeMedirProcessor()
    : AudioProcessor(BusesProperties()
       .withInput("Entrada",juce::AudioChannelSet::stereo(),true)
       .withOutput(juce::String::fromUTF8("Saída"),juce::AudioChannelSet::stereo(),true)),
      state(*this,&undoManager,"TREM_PARAMS",makeParameters()) {
    for (auto& v: scopeMono) v.store(0);
    for (auto& v: scopeLeft) v.store(0);
    for (auto& v: scopeRight) v.store(0);
    for (auto& v: historyShort) v.store(-100);
    for (auto& v: historyIntegrated) v.store(-100);
    engine.prepare(48000,2);
    startTimer(1000); // disk writing is never performed on the audio thread
}
TremDeMedirProcessor::~TremDeMedirProcessor() { stopTimer(); }
bool TremDeMedirProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    const auto in=layouts.getMainInputChannelSet(), out=layouts.getMainOutputChannelSet();
    if(in.isDisabled() || out.isDisabled() || in.size()!=out.size()) return false;
    switch(in.size()) { case 1: case 2: case 3: case 4: case 5:
                        case 6: case 7: case 8: case 10: case 12: return true;
                        default: return false; }
}
void TremDeMedirProcessor::prepareToPlay(double sr, int) {
    currentHQ.store(state.getRawParameterValue("truepeak")->load() > 0.5f);
    engine.prepare(sr, getTotalNumInputChannels(),currentHQ.load());
    lastHop=0; historyPosition.store(0); scopePosition.store(0);
    rhythmHop=std::max(1,(int)std::round(sr/200.)); rhythmSamples=0;rhythmEnergy=0;rhythmPosition.store(0);bpm.store(0);bpmConfidence.store(0);clips.store(0);
    publish(engine.snapshot());
}
void TremDeMedirProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    const int n=buffer.getNumSamples(), channels=std::min({buffer.getNumChannels(),getTotalNumInputChannels(),trem::maxChannels});
    for(int c=channels;c<buffer.getNumChannels();++c) buffer.clear(c,0,n);
    if(channels<=0 || n<=0) return;
    const auto request=resetPending.exchange(0);
    if(request) {
        if(request==1) engine.resetLoudness(); else {engine.reset();clips.store(0);}
        lastHop=0; historyPosition.store(0);
    }
    const bool hq = state.getRawParameterValue("truepeak")->load() > .5f;
    if(hq != currentHQ.load()) {
        engine.prepare(getSampleRate(),channels,hq); // changes to HQ restart a measurement
        currentHQ.store(hq);
        lastHop=0; historyPosition.store(0);
    }
    const int gateIndex=(int)state.getRawParameterValue("gate")->load();
    engine.setGate(gateIndex==0?0:gateIndex==1?8:10);
    const int lfeIndex=(int)state.getRawParameterValue("lfe")->load();
    const bool hasLfe = channels==6 || channels==8 || channels==10 || channels==12;
    engine.setSurroundWeights(!hasLfe ? -1 : lfeIndex==0 ? 3 : lfeIndex==1 ? 5 : -1);
    const int historyMode=(int)state.getRawParameterValue("history")->load();
    bool playing=true;
    if(historyMode!=0) {
        if(auto* ph=getPlayHead()) {
            if(auto pos=ph->getPosition()) playing=pos->getIsPlaying();
        }
        if(historyMode==2 && playing && !wasPlaying) {
            engine.resetLoudness(); lastHop=0; historyPosition.store(0);
        }
    }
    wasPlaying=playing;
    std::array<const float*,trem::maxChannels> pointers{};
    for(int c=0;c<channels;++c) pointers[c]=buffer.getReadPointer(c);
    const bool capture=playing || historyMode==0;
    engine.setLoudnessPaused(state.getRawParameterValue("measurePause")->load()>.5f);
    if(capture) engine.process(pointers.data(),channels,n);
    if(auto* ph=getPlayHead())if(auto pos=ph->getPosition())if(auto tempo=pos->getBpm())hostTempo.store((float)*tempo);
    uint64_t blockClips=0;
    for(int i=0;i<n;++i){double e=0;for(int c=0;c<channels;++c){double v=buffer.getSample(c,i);e+=v*v;if(std::abs(v)>=1.)++blockClips;}
        if(capture){rhythmEnergy+=e/channels;if(++rhythmSamples>=rhythmHop){auto at=rhythmPosition.load(std::memory_order_relaxed);rhythm[at%rhythm.size()].store((float)std::sqrt(rhythmEnergy/rhythmSamples),std::memory_order_relaxed);rhythmPosition.store(at+1,std::memory_order_release);rhythmEnergy=0;rhythmSamples=0;}}
    }
    clips.fetch_add(blockClips,std::memory_order_relaxed);
    // Scope remains active even when the host transport is stopped.
    const auto base=scopePosition.load(std::memory_order_relaxed);
    for(int i=0;i<n;++i) {
        const float L=buffer.getSample(0,i);
        const float R=buffer.getSample(channels>1?1:0,i);
        const int idx=(int)((base+(uint64_t)i)%scopeMono.size());
        scopeLeft[(size_t)idx].store(L,std::memory_order_relaxed);
        scopeRight[(size_t)idx].store(R,std::memory_order_relaxed);
        scopeMono[(size_t)idx].store((L+R)*.5f,std::memory_order_relaxed);
    }
    scopePosition.store(base+(uint64_t)n,std::memory_order_release);
    const auto m=engine.snapshot();
    publish(m);
    if(m.completedHops<lastHop) lastHop=0;
    if(m.completedHops>lastHop) {
        const auto where=historyPosition.fetch_add(1,std::memory_order_relaxed)%historyShort.size();
        historyShort[(size_t)where].store(m.shortTerm,std::memory_order_relaxed);
        historyIntegrated[(size_t)where].store(m.integrated,std::memory_order_relaxed);
        lastHop=m.completedHops;
    }
}
void TremDeMedirProcessor::publish(const trem::MeterSnapshot& m) noexcept {
    for(int c=0;c<trem::maxChannels;++c) {
        peaks[c].store(m.peakDb[c],std::memory_order_relaxed);
        maxPeaks[c].store(m.maxPeakDb[c],std::memory_order_relaxed);
        rms[c].store(m.rmsDb[c],std::memory_order_relaxed);
        maxRms[c].store(m.maxRmsDb[c],std::memory_order_relaxed);
        truePeaks[c].store(m.truePeakDb[c],std::memory_order_relaxed);
    }
    momentary.store(m.momentary); shortTerm.store(m.shortTerm); integrated.store(m.integrated);
    momentaryMax.store(m.momentaryMax); shortTermMax.store(m.shortTermMax);
    truePeakMax.store(m.truePeakMax); lra.store(m.lra); plr.store(m.plr); psr.store(m.psr);
    phase.store(m.phase); balance.store(m.balance); vuL.store(m.vuLeft); vuR.store(m.vuRight);
    ppmL.store(m.ppmLeft); ppmR.store(m.ppmRight); elapsed.store(m.elapsedSeconds);
    channelCount.store(m.channels);
}
trem::MeterSnapshot TremDeMedirProcessor::currentMeters() const noexcept {
    trem::MeterSnapshot m;
    for(int c=0;c<trem::maxChannels;++c) {
        m.peakDb[c]=peaks[c].load(); m.maxPeakDb[c]=maxPeaks[c].load();
        m.rmsDb[c]=rms[c].load(); m.maxRmsDb[c]=maxRms[c].load();
        m.truePeakDb[c]=truePeaks[c].load();
    }
    m.momentary=momentary.load();m.shortTerm=shortTerm.load();m.integrated=integrated.load();
    m.momentaryMax=momentaryMax.load();m.shortTermMax=shortTermMax.load();
    m.truePeakMax=truePeakMax.load();m.lra=lra.load();m.plr=plr.load();m.psr=psr.load();
    m.phase=phase.load();m.balance=balance.load();m.vuLeft=vuL.load();m.vuRight=vuR.load();
    m.ppmLeft=ppmL.load();m.ppmRight=ppmR.load();m.elapsedSeconds=elapsed.load();
    m.channels=channelCount.load();
    return m;
}
void TremDeMedirProcessor::copyScope(std::array<float,4096>& mono,
                                  std::array<float,4096>& left,
                                  std::array<float,4096>& right) const noexcept {
    const auto pos=scopePosition.load(std::memory_order_acquire);
    for(size_t i=0;i<4096;++i) {
        const size_t idx=(size_t)((pos+i)%4096);
        mono[i]=scopeMono[idx].load(std::memory_order_relaxed);
        left[i]=scopeLeft[idx].load(std::memory_order_relaxed);
        right[i]=scopeRight[idx].load(std::memory_order_relaxed);
    }
}
void TremDeMedirProcessor::copyHistory(std::array<float,720>& st,
                                       std::array<float,720>& integ) const noexcept {
    const auto pos=historyPosition.load(std::memory_order_acquire);
    for(size_t i=0;i<720;++i) {
        if(pos+i<720) { st[i]=-100;integ[i]=-100;continue; }
        const size_t idx=(size_t)((pos+i)%720);
        st[i]=historyShort[idx].load(std::memory_order_relaxed);
        integ[i]=historyIntegrated[idx].load(std::memory_order_relaxed);
    }
}
void TremDeMedirProcessor::getStateInformation(juce::MemoryBlock& dest) {
    auto xml=state.copyState().createXml();
    if(xml) copyXmlToBinary(*xml,dest);
}
void TremDeMedirProcessor::setStateInformation(const void* data,int size) {
    if(auto xml=getXmlFromBinary(data,size)) {
        if(xml->hasTagName(state.state.getType().toString()))
            state.replaceState(juce::ValueTree::fromXml(*xml));
    }
}
juce::File TremDeMedirProcessor::getLogFile() const {
    const auto folder=juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("Trem de Medir").getChildFile("Logs");
    return folder.getChildFile("TremDeMedir_" + juce::Time::getCurrentTime().formatted("%Y-%m-%d") + ".csv");
}
void TremDeMedirProcessor::timerCallback() {
    std::array<float,4096> frames{};const auto pos=rhythmPosition.load(std::memory_order_acquire);int count=(int)std::min<uint64_t>(pos,frames.size());
    for(int i=0;i<count;++i)frames[i]=rhythm[(pos-count+i)%rhythm.size()].load(std::memory_order_relaxed);
    if(count<1200)tempoTracker.reset();
    const auto estimate=tempoTracker.update(trem::detectTempo(frames.data(),count));bpm.store((float)estimate.bpm);bpmConfidence.store((float)estimate.confidence);
    if(state.getRawParameterValue("logging")->load()<.5f) { logElapsed=0;return; }
    const int index=(int)state.getRawParameterValue("logInterval")->load();
    const int seconds=index==0?1:index==1?5:index==2?10:30;
    if(++logElapsed<seconds) return;
    logElapsed=0;
    auto file=getLogFile();
    file.getParentDirectory().createDirectory();
    const auto priorSize=file.existsAsFile()?file.getSize():0;
    std::unique_ptr<juce::FileOutputStream> stream(file.createOutputStream());
    if(!stream || !stream->openedOk()) return;
    const bool header=priorSize==0;
    stream->setPosition(priorSize);
    if(header) stream->writeText("data_hora,tempo_s,lufs_integrado,lufs_curto,lufs_momentaneo,true_peak_dbtp,lra,plr,psr,correlacao,fator_balanco\n",false,false,"\n");
    const auto m=currentMeters();
    auto line=juce::Time::getCurrentTime().formatted("%Y-%m-%d %H:%M:%S") + "," +
       juce::String(m.elapsedSeconds,1) + "," + juce::String(m.integrated,2) + "," +
       juce::String(m.shortTerm,2) + "," + juce::String(m.momentary,2) + "," +
       juce::String(m.truePeakMax,2) + "," + juce::String(m.lra,2) + "," +
       juce::String(m.plr,2) + "," + juce::String(m.psr,2) + "," +
       juce::String(m.phase,3) + "," + juce::String(m.balance,3) + "\n";
    stream->writeText(line,false,false,"\n");
}
void TremDeMedirProcessor::loadReferencePreset(int index) {
    // User-editable working references. Distribution targets can change.
    const struct Setting {float target,ceiling;} presets[] = {
        {-14,-1}, {-23,-1}, {-16,-1}, {-14,-1}, {-24,-2}, {-27,-2}, {-11,-1}, {-18,-1}
    };
    if(index<0 || index>= (int)(sizeof(presets)/sizeof(presets[0]))) return;
    const auto set=[this](const juce::String& id,float v) {
        if(auto* p=state.getParameter(id)) {
            const auto r=p->getNormalisableRange();
            p->beginChangeGesture();p->setValueNotifyingHost(r.convertTo0to1(v));p->endChangeGesture();
        }
    };
    set("target",presets[index].target);
    set("ceiling",presets[index].ceiling);
}
juce::AudioProcessorEditor* TremDeMedirProcessor::createEditor() {
    return new TremDeMedirEditor(*this);
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new TremDeMedirProcessor(); }
