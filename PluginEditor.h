#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "PluginProcessor.h"
#include <array>
#include <memory>
#include <vector>

class TremDeMedirEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit TremDeMedirEditor(TremDeMedirProcessor&);
    ~TremDeMedirEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
private:
    void timerCallback() override;
    void updateSpectrum();
    void style(juce::ComboBox&);
    void style(juce::TextButton&);
    void style(juce::Slider&);
    void section(juce::Graphics&,juce::Rectangle<int>, const juce::String&) const;
    void paintLoudness(juce::Graphics&,juce::Rectangle<int>);
    void paintDigital(juce::Graphics&,juce::Rectangle<int>);
    void paintStereo(juce::Graphics&,juce::Rectangle<int>);
    void paintAnalogue(juce::Graphics&,juce::Rectangle<int>);
    void paintSpectrum(juce::Graphics&,juce::Rectangle<int>);
    void paintReadout(juce::Graphics&,juce::Rectangle<int>,const juce::String&,float,
                      const juce::String&,bool alert=false) const;
    struct PanelRects {juce::Rectangle<int> loud, digital, stereo, analogue, spectrum, footer;};
    PanelRects panels() const;
    TremDeMedirProcessor& processor;
    juce::ComboBox preset, gate, history, digitalMode, analogMode, weighting, rtaSource, lfe, logInterval, loudView;
    juce::Slider target, ceiling, vuRef;
    juce::ToggleButton highQuality, alerts, logging;
    juce::TextButton resetAll{"LIMPAR TUDO"},resetLoud{"LIMPAR LUFS"},undo{"DESFAZER"},redo{"REFAZER"},
                     ab{"A / B"},copyBank{"COPIAR A > B"},logFolder{"PASTA CSV"};
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> combos;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliders;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> toggles;
    juce::dsp::FFT fft{12};
    std::array<float,8192> fftBuffer{};
    std::array<float,4096> sourceMono{},sourceLeft{},sourceRight{};
    std::array<float,720> loudHistory{},integratedHistory{};
    std::array<float,31> bands{},maxBands{};
    int hoveredBand=-1,selectedBand=15;
    trem::MeterSnapshot latest{};
    juce::ValueTree bankA,bankB;
    bool activeBankA=true;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TremDeMedirEditor)
};
