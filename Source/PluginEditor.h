#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "PluginProcessor.h"
#include <array>
#include <memory>
#include <vector>

class GlassLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
    void drawComboBox(juce::Graphics&,int,int,bool,int,int,int,int,juce::ComboBox&) override;
    juce::PopupMenu::Options getOptionsForComboBoxPopupMenu(juce::ComboBox&,juce::Label&) override;
};
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
    void paintTools(juce::Graphics&);
    int bandNumber() const;
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
    GlassLookAndFeel glass;
    juce::ComboBox preset, gate, history, digitalMode, analogMode, weighting, rtaSource, lfe, logInterval, loudView;
    juce::ComboBox loudUnits,bandCount,rtaMode,levelMode,holdTime,returnRate,rtaRange;
    juce::Slider target, ceiling, vuRef,tuneRef;
    juce::ToggleButton highQuality, alerts, logging,measurePause,peakHold,lrMax;
    juce::TextButton tap{"TAP"},resetPeaks{"RESET PEAK"};
    juce::TextButton resetAll{"LIMPAR TUDO"},resetLoud{"LIMPAR LUFS"},undo{"DESFAZER"},redo{"REFAZER"},
                     ab{"A / B"},copyBank{"COPIAR A > B"},logFolder{"PASTA CSV"};
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>> combos;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliders;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> toggles;
    juce::dsp::FFT fft{12};
    std::array<float,8192> fftBuffer{};
    std::array<float,4096> sourceMono{},sourceLeft{},sourceRight{};
    std::array<float,720> loudHistory{},integratedHistory{};
    std::array<float,127> bands{},maxBands{},bandAge{};
    std::array<float,trem::maxChannels> heldLevels{},levelAge{},displayLevels{};
    trem::PitchReading pitch{};
    std::array<double,5> tapTimes{};
    double tapTempo=0,lastFrameMs=0;int tapCount=0,analysisFrame=0;
    int previousBands=0; float frameSeconds=1.f/24.f;
    int hoveredBand=-1,selectedBand=15;
    trem::MeterSnapshot latest{};
    juce::ValueTree bankA,bankB;
    bool activeBankA=true;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TremDeMedirEditor)
};
