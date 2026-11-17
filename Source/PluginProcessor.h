#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <array>
#include "MeterDSP.h"
#include "AnalysisTools.h"

class TremDeMedirProcessor final : public juce::AudioProcessor, private juce::Timer {
public:
    TremDeMedirProcessor();
    ~TremDeMedirProcessor() override;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Padrão"; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::UndoManager undoManager;
    juce::AudioProcessorValueTreeState state;
    void requestReset(bool loudnessOnly=false) noexcept { resetPending.store(loudnessOnly?1:2); }
    trem::MeterSnapshot currentMeters() const noexcept;
    void copyScope(std::array<float,4096>& mono,
                   std::array<float,4096>& left,
                   std::array<float,4096>& right) const noexcept;
    void copyHistory(std::array<float,720>& shortTerm,
                     std::array<float,720>& integrated) const noexcept;
    juce::File getLogFile() const;
    void loadReferencePreset(int index);
    bool isHighQualityActive() const noexcept { return currentHQ.load(); }
    double detectedBpm() const { return bpm.load(); }
    double tempoConfidence() const { return bpmConfidence.load(); }
    double hostBpm() const { return hostTempo.load(); }
    uint64_t clipSamples() const { return clips.load(); }
private:
    std::array<std::atomic<float>,4096> rhythm{};
    std::atomic<uint64_t> rhythmPosition{0}, clips{0};
    std::atomic<float> bpm{0},bpmConfidence{0},hostTempo{0};
    int rhythmSamples=0,rhythmHop=240; double rhythmEnergy=0;
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();
    void timerCallback() override;
    void publish(const trem::MeterSnapshot&) noexcept;
    trem::MeterEngine engine;
    std::atomic<int> resetPending {0};
    std::atomic<bool> currentHQ {false};
    std::atomic<uint64_t> scopePosition {0}, historyPosition {0};
    std::array<std::atomic<float>,4096> scopeMono{}, scopeLeft{}, scopeRight{};
    std::array<std::atomic<float>,720> historyShort{}, historyIntegrated{};
    std::array<std::atomic<float>,trem::maxChannels> peaks{}, maxPeaks{}, rms{}, maxRms{}, truePeaks{};
    std::atomic<float> momentary {-100}, shortTerm {-100}, integrated {-100};
    std::atomic<float> momentaryMax {-100}, shortTermMax {-100}, truePeakMax {-100};
    std::atomic<float> lra{0}, plr{0}, psr{0}, phase{0}, balance{0};
    std::atomic<float> vuL{-100}, vuR{-100}, ppmL{-100}, ppmR{-100}, elapsed{0};
    std::atomic<int> channelCount{2};
    uint64_t lastHop=0;
    bool wasPlaying=false;
    int logElapsed=0;
    juce::String lastLoggedFile;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TremDeMedirProcessor)
};
