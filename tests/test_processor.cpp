#ifdef NDEBUG
#undef NDEBUG
#endif
#include <juce_audio_processors/juce_audio_processors.h>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <memory>
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
int main(){
 juce::ScopedJuceInitialiser_GUI init;
 std::unique_ptr<juce::AudioProcessor> plugin(createPluginFilter());
 for(double sr:{44100.,48000.,96000.}){
  plugin->prepareToPlay(sr,256);juce::AudioBuffer<float> audio(2,256);juce::MidiBuffer midi;
  for(int block=0;block<400;++block){for(int c=0;c<2;++c)for(int i=0;i<256;++i)audio.setSample(c,i,(float)(.2*std::sin((block*256+i)*(c+1)*.017)));
   juce::AudioBuffer<float> original;original.makeCopyOf(audio);plugin->processBlock(audio,midi);
   for(int c=0;c<2;++c)assert(std::memcmp(original.getReadPointer(c),audio.getReadPointer(c),256*sizeof(float))==0);
  }plugin->releaseResources();std::printf("PASS transparent stereo %.0f Hz\n",sr);
 }
 assert(plugin->getParameters().size()==25);
 for(auto* p:plugin->getParameters())if(auto* rp=dynamic_cast<juce::RangedAudioParameter*>(p))if(rp->paramID=="bandCount"||rp->paramID=="measurePause"||rp->paramID=="truepeak")rp->setValueNotifyingHost(1.f);
 juce::MemoryBlock state;plugin->getStateInformation(state);assert(state.getSize()>0);
 std::unique_ptr<juce::AudioProcessor> restored(createPluginFilter());restored->setStateInformation(state.getData(),(int)state.getSize());
 auto a=plugin->getParameters(),b=restored->getParameters();assert(a.size()==b.size());for(int i=0;i<a.size();++i)assert(std::abs(a[i]->getValue()-b[i]->getValue())<1e-6f);
 // Reopen a legacy state that contains only the original fourteen parameters.
 auto xml=juce::AudioProcessor::getXmlFromBinary(state.getData(),(int)state.getSize());assert(xml);
 const juce::StringArray oldIds{"target","ceiling","gate","truepeak","history","meterstyle","vuRef","analog","weighting","rtaSource","lfe","alerts","logging","logInterval"};
 for(auto* child=xml->getFirstChildElement();child;){auto* next=child->getNextElement();if(!oldIds.contains(child->getStringAttribute("id")))xml->removeChildElement(child,true);child=next;}
 juce::MemoryBlock oldState;juce::AudioProcessor::copyXmlToBinary(*xml,oldState);
 std::unique_ptr<juce::AudioProcessor> legacy(createPluginFilter());legacy->setStateInformation(oldState.getData(),(int)oldState.getSize());
 for(auto* p:legacy->getParameters())if(auto* rp=dynamic_cast<juce::RangedAudioParameter*>(p)){if(rp->paramID=="measurePause")assert(rp->getValue()==0.f);if(rp->paramID=="bandCount")assert(std::abs(rp->getValue()-.5f)<1e-6f);}
 std::puts("PROCESSOR TESTS PASSED: state round trip and legacy defaults");
}
