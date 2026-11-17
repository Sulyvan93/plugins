#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

namespace {
// Minas Gerais: white field and an unlettered red triangle.
const juce::Colour bg=juce::Colour::fromRGB(12,15,22);
const juce::Colour panel=juce::Colour::fromRGB(30,36,47);
const juce::Colour panelDark=juce::Colour::fromRGB(16,22,32);
const juce::Colour grid=juce::Colour::fromRGB(51,62,78);
const juce::Colour text=juce::Colour::fromRGB(238,242,248);
const juce::Colour muted=juce::Colour::fromRGB(156,170,189);
const juce::Colour accent=juce::Colour::fromRGB(244,103,119);
const juce::Colour yellow=juce::Colour::fromRGB(242,194,110);
const juce::Colour red=juce::Colour::fromRGB(255,101,117);
const juce::Colour blue=juce::Colour::fromRGB(111,189,243);
constexpr float piF = juce::MathConstants<float>::pi;
float normal(float db,float bottom=-60,float top=0) {return juce::jlimit(0.f,1.f,(db-bottom)/(top-bottom));}
void setFont(juce::Graphics& g,float sz,bool bold=false) {
    juce::Font f{juce::FontOptions(sz)};
    f.setBold(bold);
    g.setFont(f);
}
}
void GlassLookAndFeel::drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour&,bool over,bool down) {
    auto r=b.getLocalBounds().toFloat().reduced(.5f);auto top=panel.brighter(over?.32f:.12f);
    g.setGradientFill(juce::ColourGradient(top,0,0,panelDark,0,r.getBottom(),false));g.fillRoundedRectangle(r,9);
    g.setColour((down?accent:juce::Colours::white).withAlpha(down?.7f:.16f));g.drawRoundedRectangle(r,9,1);
}
void GlassLookAndFeel::drawComboBox(juce::Graphics& g,int w,int h,bool,int,int,int,int,juce::ComboBox&) {
    auto r=juce::Rectangle<float>(.5f,.5f,w-1.f,h-1.f);
    g.setGradientFill(juce::ColourGradient(panel.brighter(.08f),0,0,panelDark,0,(float)h,false));g.fillRoundedRectangle(r,8);
    g.setColour(juce::Colours::white.withAlpha(.13f));g.drawRoundedRectangle(r,8,1);
    juce::Path arrow;arrow.startNewSubPath(w-19.f,h*.43f);arrow.lineTo(w-14.f,h*.59f);arrow.lineTo(w-9.f,h*.43f);
    g.setColour(muted);g.strokePath(arrow,juce::PathStrokeType(1.5f));
}
juce::PopupMenu::Options GlassLookAndFeel::getOptionsForComboBoxPopupMenu(juce::ComboBox& box,juce::Label&) {
    return juce::PopupMenu::Options().withTargetComponent(&box).withParentComponent(box.getTopLevelComponent()).withMinimumWidth(box.getWidth()).withStandardItemHeight(28);
}
TremDeMedirEditor::TremDeMedirEditor(TremDeMedirProcessor& p)
    : AudioProcessorEditor(&p),processor(p) {
    setLookAndFeel(&glass);
    glass.setColour(juce::PopupMenu::backgroundColourId,panelDark);
    glass.setColour(juce::PopupMenu::textColourId,text);
    glass.setColour(juce::PopupMenu::highlightedBackgroundColourId,panel);
    glass.setColour(juce::PopupMenu::highlightedTextColourId,accent);
    setOpaque(true);
    setSize(1280,1100);
    setResizable(true,true);
    setResizeLimits(1120,1040,1800,1500);
    for (juce::Component* c : std::initializer_list<juce::Component*>{&preset,&gate,&history,&digitalMode,
                 &analogMode,&weighting,&rtaSource,&lfe,&logInterval,&target,&ceiling,&vuRef,
                 &highQuality,&alerts,&logging,&resetAll,&resetLoud,&undo,&redo,&ab,&copyBank,&loudView,&logFolder,&loudUnits,&bandCount,&rtaMode,&levelMode,&holdTime,&returnRate,&rtaRange,&tempoOctave,&measurePause,&peakHold,&lrMax,&tuneRef,&tap,&resetPeaks}) addAndMakeVisible(c);
    preset.addItemList({juce::String::fromUTF8("Spotify / vídeos (-14)"),"EBU R128 (-23)",juce::String::fromUTF8("Apple referência (-16)"),
        juce::String::fromUTF8("YouTube referência (-14)"),"ATSC A/85 (-24)",juce::String::fromUTF8("Netflix referência (-27)"),
        "Spotify Loud (-11)",juce::String::fromUTF8("Estúdio (-18)")},1);
    preset.onChange=[this] {processor.loadReferencePreset(preset.getSelectedItemIndex());};
    auto attachCombo=[this](juce::ComboBox& c,const juce::String& id,const juce::StringArray& items) {
        c.addItemList(items,1);
        style(c);
        combos.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state,id,c));
    };
    attachCombo(gate,"gate",{"Sem gate","Gate -8 LU","Gate -10 LU"});
    attachCombo(history,"history",{juce::String::fromUTF8("Contínuo"),"Play","Reset no Play"});
    attachCombo(digitalMode,"meterstyle",{juce::String::fromUTF8("Não linear"),"Linear","K-20","K-14","K-12"});
    attachCombo(analogMode,"analog",{"VU","PPM"});
    attachCombo(weighting,"weighting",{"Sem peso","A","C"});
    attachCombo(rtaSource,"rtaSource",{"Mono L+R","L","R"});
    attachCombo(lfe,"lfe",{juce::String::fromUTF8("LFE: 4º"),juce::String::fromUTF8("LFE: 6º"),"Sem LFE"});
    attachCombo(logInterval,"logInterval",{"1 s","5 s","10 s","30 s"});
    attachCombo(tempoOctave,"tempoOctave",{"Auto","/2","x2"});
    tempoOctave.setTooltip("Auto: pulso de 60 a 120 BPM. /2 e x2 resolvem a oitava ritmica.");
    attachCombo(loudUnits,"loudUnits",{"LUFS","LU relativo"});
    attachCombo(bandCount,"bandCount",{"31 bandas","63 bandas","127 bandas"});
    attachCombo(rtaMode,"rtaMode",{"Peak","RMS Fast","RMS Slow"});
    attachCombo(levelMode,"levelMode",{"Peak + RMS","Peak","RMS"});
    attachCombo(holdTime,"holdTime",{"Hold 0 s","Hold 1 s","Hold 2 s","Hold 5 s"});
    attachCombo(returnRate,"returnRate",{"6 dB/s","12 dB/s","24 dB/s"});
    attachCombo(rtaRange,"rtaRange",{"30 dB","60 dB","90 dB","120 dB"});
    style(preset);preset.setSelectedId(1,juce::dontSendNotification);
    style(loudView);
    loudView.addItemList({juce::String::fromUTF8("Histórico"), "Histograma"},1);
    loudView.setSelectedId(1,juce::dontSendNotification);
    loudView.onChange=[this] {repaint();};
    auto attachSlider=[this](juce::Slider& s,const juce::String& id) {
        s.setSliderStyle(juce::Slider::LinearHorizontal);
        s.setTextBoxStyle(juce::Slider::TextBoxRight,false,62,24);
        style(s);
        sliders.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,id,s));
    };
    attachSlider(target,"target");attachSlider(ceiling,"ceiling");attachSlider(vuRef,"vuRef");attachSlider(tuneRef,"tuneRef");
    tuneRef.setNumDecimalPlacesToDisplay(0);
    auto attachToggle=[this](juce::ToggleButton& b,const juce::String& id,const juce::String& label) {
        b.setButtonText(label);b.setColour(juce::ToggleButton::textColourId,text);
        b.setColour(juce::ToggleButton::tickColourId,accent);
        toggles.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,id,b));
    };
    attachToggle(highQuality,"truepeak","TP 8x");
    attachToggle(alerts,"alerts","Alertas");
    attachToggle(logging,"logging","Gravar CSV");
    attachToggle(measurePause,"measurePause","PAUSAR LUFS");
    attachToggle(peakHold,"peakHold","Hold infinito");
    attachToggle(lrMax,"lrMax","LR MAX");
    for(auto* b : {&tap,&resetPeaks,&resetAll,&resetLoud,&undo,&redo,&ab,&copyBank,&logFolder}) style(*b);
    auto clearPeaks=[this] {maxBands.fill(-100);heldLevels.fill(-100);bandAge.fill(0);levelAge.fill(0);};
    resetAll.onClick=[this,clearPeaks] {processor.requestReset();clearPeaks();};
    resetPeaks.onClick=clearPeaks;
    tap.onClick=[this] {double now=juce::Time::getMillisecondCounterHiRes()/1000.;if(tapCount&&now-tapTimes[(tapCount-1)%5]>2.)tapCount=0;tapTimes[tapCount%5]=now;++tapCount;if(tapCount>=3){int n=std::min(tapCount,5);double first=tapTimes[(tapCount-n)%5];tapTempo=60.*(n-1)/(now-first);}};
    resetLoud.onClick=[this] {processor.requestReset(true);};
    undo.onClick=[this] {processor.undoManager.undo();};
    redo.onClick=[this] {processor.undoManager.redo();};
    bankA=processor.state.copyState();bankB=bankA.createCopy();
    ab.onClick=[this] {
        if(activeBankA) {bankA=processor.state.copyState();processor.state.replaceState(bankB.createCopy());}
        else {bankB=processor.state.copyState();processor.state.replaceState(bankA.createCopy());}
        activeBankA=!activeBankA;
        ab.setButtonText(activeBankA?"A ATIVO":"B ATIVO");
        copyBank.setButtonText(activeBankA?"COPIAR A > B":"COPIAR B > A");
    };
    copyBank.onClick=[this] {
        if(activeBankA) bankB=processor.state.copyState();
        else bankA=processor.state.copyState();
    };
    logFolder.onClick=[this] {processor.getLogFile().getParentDirectory().createDirectory();
        processor.getLogFile().getParentDirectory().revealToUser();};
    maxBands.fill(-100);
    bands.fill(-100);
    heldLevels.fill(-100);displayLevels.fill(-100);
    setWantsKeyboardFocus(false);
    startTimerHz(24);
}
TremDeMedirEditor::~TremDeMedirEditor(){stopTimer();setLookAndFeel(nullptr);}
void TremDeMedirEditor::style(juce::ComboBox& c) {
    c.setColour(juce::ComboBox::backgroundColourId,panel);
    c.setColour(juce::ComboBox::textColourId,text);
    c.setColour(juce::ComboBox::outlineColourId,grid);
    c.setColour(juce::ComboBox::arrowColourId,accent);
}
void TremDeMedirEditor::style(juce::TextButton& b) {
    b.setColour(juce::TextButton::buttonColourId,panel);
    b.setColour(juce::TextButton::textColourOffId,text);
    b.setColour(juce::TextButton::buttonOnColourId,juce::Colour::fromRGB(249,225,228));
    b.setMouseCursor(juce::MouseCursor::PointingHandCursor);
}
void TremDeMedirEditor::style(juce::Slider& s) {
    s.setColour(juce::Slider::trackColourId,accent);
    s.setColour(juce::Slider::thumbColourId,yellow);
    s.setColour(juce::Slider::textBoxTextColourId,text);
    s.setColour(juce::Slider::textBoxBackgroundColourId,panelDark);
    s.setColour(juce::Slider::textBoxOutlineColourId,grid);
}
TremDeMedirEditor::PanelRects TremDeMedirEditor::panels() const {
    const int margin=16,gap=12,header=250,footer=42;
    const int body=getHeight()-header-footer-margin*2-gap*2;
    const int loudH=(int)(body*0.33),middleH=(int)(body*0.34),specH=body-loudH-middleH;
    auto loud=juce::Rectangle<int>(margin,header,getWidth()-2*margin,loudH);
    const int y2=loud.getBottom()+gap,available=getWidth()-2*margin-2*gap;
    const int digitalW=(int)(available*.43),stereoW=(int)(available*.27),analogW=available-digitalW-stereoW;
    auto d=juce::Rectangle<int>(margin,y2,digitalW,middleH);
    auto s=juce::Rectangle<int>(d.getRight()+gap,y2,stereoW,middleH);
    auto a=juce::Rectangle<int>(s.getRight()+gap,y2,analogW,middleH);
    auto r=juce::Rectangle<int>(margin,y2+middleH+gap,getWidth()-2*margin,specH);
    auto f=juce::Rectangle<int>(margin,getHeight()-footer+5,getWidth()-2*margin,footer-5);
    return {loud,d,s,a,r,f};
}
void TremDeMedirEditor::resized() {
    const int w=getWidth();
    // Give the brand and reference controls separate rows at every window size.
    const int column=(w-52)/3;
    preset.setBounds(16,75,column,30);
    target.setBounds(26+column,75,column,30);
    ceiling.setBounds(36+2*column,75,column,30);
    resetAll.setBounds(w-140,14,120,30);
    measurePause.setBounds(w-300,16,152,30);
    const int x=16,y=120,h=28,gap=6;
    // Progressive widths fit the 1020 px minimum size.
    const int each=(w-32-8*gap)/9;
    int at=x;
    auto put=[&](juce::Component& c,int units=1){const int wd=each*units+gap*(units-1);
        c.setBounds(at,y,wd,h);at+=wd+gap;};
    put(gate);put(history);put(digitalMode);put(analogMode);
    put(lfe);put(logInterval);
    put(highQuality);put(alerts);put(logging);
    tuneRef.setBounds(237,190,165,26);
    tap.setBounds((int)(w*.64f),190,65,26);
    tempoOctave.setBounds((int)(w*.64f),165,65,22);
    const auto spec=panels().spectrum;
    const int item=(spec.getWidth()-40)/9;
    juce::Component* sc[]={&bandCount,&rtaMode,&rtaSource,&weighting,&rtaRange,&holdTime,&returnRate,&peakHold,&lrMax};
    for(int i=0;i<9;++i)sc[i]->setBounds(spec.getX()+16+i*item,spec.getY()+36,item-5,26);
    const auto digital=panels().digital;
    levelMode.setBounds(digital.getX()+16,digital.getY()+34,140,25);
    resetPeaks.setBounds(digital.getRight()-135,digital.getY()+34,118,25);
    const auto f=panels().footer;
    const auto loudPanel=panels().loud;
    loudView.setBounds(loudPanel.getX()+255,loudPanel.getY()+8,142,24);
    loudUnits.setBounds(loudPanel.getX()+408,loudPanel.getY()+8,116,24);
    resetLoud.setBounds(f.getX(),f.getY()+2,115,26);
    undo.setBounds(f.getX()+121,f.getY()+2,77,26);
    redo.setBounds(f.getX()+204,f.getY()+2,77,26);
    ab.setBounds(f.getX()+287,f.getY()+2,74,26);
    copyBank.setBounds(f.getX()+367,f.getY()+2,98,26);
    vuRef.setBounds(f.getX()+555,f.getY()+2,182,27);
    logFolder.setBounds(f.getRight()-133,f.getY()+2,132,26);
}
void TremDeMedirEditor::section(juce::Graphics& g,juce::Rectangle<int> r,const juce::String& heading) const {
    auto bounds=r.toFloat().reduced(.5f);
    juce::DropShadow(juce::Colours::black.withAlpha(.25f),12,{0,4}).drawForRectangle(g,r.reduced(3));
    g.setGradientFill(juce::ColourGradient(panel.withAlpha(.94f),bounds.getX(),bounds.getY(),panelDark.withAlpha(.96f),bounds.getRight(),bounds.getBottom(),false));
    g.fillRoundedRectangle(bounds,14);
    g.setColour(juce::Colours::white.withAlpha(.13f));g.drawRoundedRectangle(bounds,14,1);
    g.setColour(muted);setFont(g,12,true);
    g.drawText(heading,r.reduced(16,10).removeFromTop(20),juce::Justification::centredLeft);
}
void TremDeMedirEditor::paint(juce::Graphics& g) {
    g.fillAll(bg);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff342533),0,0,bg,(float)getWidth(),(float)getHeight(),false));g.fillAll();
    auto p=panels();
    g.setColour(juce::Colours::white.withAlpha(.035f));g.fillRoundedRectangle(16.f,5.f,getWidth()-32.f,53.f,14.f);
    g.setColour(juce::Colours::white.withAlpha(.94f));g.fillRoundedRectangle(20.f,10.f,43.f,41.f,11.f);
    juce::Path triangle;
    triangle.addTriangle(41.f,15.f,24.f,44.f,58.f,44.f);
    g.setColour(red);g.fillPath(triangle);
    g.setColour(text);setFont(g,22,true);
    g.drawText("TREM DE MEDIR",juce::Rectangle<int>(80,7,295,34),juce::Justification::centredLeft);
    g.setColour(muted);setFont(g,11);
    g.drawText(juce::String::fromUTF8("MINAS GERAIS • MEDIÇÃO DE ÁUDIO"),juce::Rectangle<int>(81,38,350,17),juce::Justification::centredLeft);
    setFont(g,11,true);g.setColour(muted);
    const int column=(getWidth()-52)/3;
    g.drawText("PRESET",juce::Rectangle<int>(16,59,110,15),juce::Justification::centredLeft);
    g.drawText("ALVO LUFS",juce::Rectangle<int>(26+column,59,100,15),juce::Justification::centredLeft);
    g.drawText("TETO dBTP",juce::Rectangle<int>(36+2*column,59,100,15),juce::Justification::centredLeft);
    section(g,p.loud,juce::String::fromUTF8("01  •  LOUDNESS"));
    section(g,p.digital,juce::String::fromUTF8("02  •  PEAK / RMS MULTICANAL"));
    section(g,p.stereo,juce::String::fromUTF8("03  •  ESTÉREO / CORRELAÇÃO"));
    section(g,p.analogue,juce::String::fromUTF8("04  •  VU / PPM"));
    section(g,p.spectrum,juce::String::fromUTF8("05  •  SPECTRUM ANALYZER"));
    paintTools(g);
    paintLoudness(g,p.loud);paintDigital(g,p.digital);paintStereo(g,p.stereo);
    paintAnalogue(g,p.analogue);paintSpectrum(g,p.spectrum);
    g.setColour(muted);setFont(g,11);
    g.drawText("0 VU",p.footer.getX()+481,p.footer.getY()+5,70,20,juce::Justification::centredLeft);
    if(getWidth()>1190)
        g.drawText(juce::String::fromUTF8("ARM64 • AUDIO TRANSPARENTE"),juce::Rectangle<int>(p.footer.getX()+755,p.footer.getY()+5,200,20),juce::Justification::centredLeft);
}
void TremDeMedirEditor::paintReadout(juce::Graphics& g,juce::Rectangle<int> r,const juce::String& name,
                                    float number,const juce::String& suffix,bool alert) const {
    g.setColour(panelDark);g.fillRoundedRectangle(r.toFloat(),5);
    g.setColour(alert?red:muted);setFont(g,10,true);
    g.drawText(name,r.reduced(8,3).removeFromTop(13),juce::Justification::centredLeft);
    g.setColour(alert?red:text);setFont(g,std::min(23.f,(r.getHeight()-29.f)*.8f),true);
    const bool relative=suffix=="LUFS"&&processor.state.getRawParameterValue("loudUnits")->load()>.5f;
    const float displayNumber=relative?number-processor.state.getRawParameterValue("target")->load():number;
    const auto val=number < -95 ? juce::String(juce::String::fromUTF8("—")) : juce::String(displayNumber,1);
    g.drawText(val,r.getX()+8,r.getY()+16,r.getWidth()-16,r.getHeight()-29,juce::Justification::centredLeft);
    g.setColour(muted);setFont(g,10);
    g.drawText(relative?juce::String("LU"):suffix,r.getX()+8,r.getBottom()-13,r.getWidth()-16,12,juce::Justification::centredLeft);
}
void TremDeMedirEditor::paintLoudness(juce::Graphics& g,juce::Rectangle<int> r) {
    auto inner=r.reduced(15).withTrimmedTop(25);
    auto graph=inner.removeFromLeft((int)(inner.getWidth()*.58f)).reduced(3,4);
    auto stats=inner.reduced(9,0);
    g.setColour(panelDark);g.fillRoundedRectangle(graph.toFloat(),5);
    const int margin=21;
    auto plot=graph.reduced(margin,13).withTrimmedTop(5).withTrimmedBottom(24);
    const float targetDb=processor.state.getRawParameterValue("target")->load();
    if(loudView.getSelectedId()==2) {
        std::array<int,61> counts{};
        int high=1;
        for(float st:loudHistory) if(st>-95) {
            const int i=juce::jlimit(0,60,(int)std::floor(st+60));
            high=std::max(high,++counts[(size_t)i]);
        }
        const float bw=(float)plot.getWidth()/61.f;
        for(int i=0;i<61;++i) {
            const float x=plot.getX()+i*bw;
            const float h=(float)counts[(size_t)i]/high*plot.getHeight();
            g.setColour(blue);g.fillRect(x,plot.getBottom()-h,std::max(1.f,bw-1),h);
        }
        const float tx=plot.getX()+normal(targetDb)*plot.getWidth();
        g.setColour(yellow);g.drawLine(tx,(float)plot.getY(),tx,(float)plot.getBottom(),2.f);
        g.setColour(muted);setFont(g,10);
        g.drawText(juce::String::fromUTF8("DISTRIBUIÇÃO DE LUFS DE CURTO PRAZO"),graph.getX()+20,
                   graph.getBottom()-20,graph.getWidth()-35,15,juce::Justification::centredLeft);
    } else {
        for(int db=-60;db<=0;db+=10) {
            float y=plot.getBottom()-normal((float)db)*plot.getHeight();
            g.setColour(grid);g.drawHorizontalLine((int)y,(float)plot.getX(),(float)plot.getRight());
            g.setColour(muted);setFont(g,10);
            g.drawText(juce::String(processor.state.getRawParameterValue("loudUnits")->load()>.5f?db-(int)targetDb:db),graph.getX()+1,(int)y-7,20,14,juce::Justification::centredRight);
        }
        const float yTarget=plot.getBottom()-normal(targetDb)*plot.getHeight();
        const float dashes[]={4.f,4.f};
        g.setColour(yellow.withAlpha(.7f));g.drawDashedLine({(float)plot.getX(),yTarget,
            (float)plot.getRight(),yTarget},dashes,2,1.f);
        juce::Path lineST,lineI;
        bool firstST=true,firstI=true;
        for(int i=0;i<(int)loudHistory.size();++i) {
            const float x=plot.getX()+(float)i/(loudHistory.size()-1)*plot.getWidth();
            if(loudHistory[(size_t)i]>-95) {
                const float y=plot.getBottom()-normal(loudHistory[(size_t)i])*plot.getHeight();
                if(firstST) {lineST.startNewSubPath(x,y);firstST=false;} else lineST.lineTo(x,y);
            } else firstST=true;
            if(integratedHistory[(size_t)i]>-95) {
                const float y=plot.getBottom()-normal(integratedHistory[(size_t)i])*plot.getHeight();
                if(firstI) {lineI.startNewSubPath(x,y);firstI=false;} else lineI.lineTo(x,y);
            } else firstI=true;
        }
        g.setColour(blue);g.strokePath(lineST,juce::PathStrokeType(1.6f));
        g.setColour(accent);g.strokePath(lineI,juce::PathStrokeType(2.f));
        g.setColour(muted);setFont(g,10);
        // Minimap / universe view of the currently retained loudness history.
        const auto mini=juce::Rectangle<float>((float)plot.getX(),(float)plot.getBottom()+6,
                                               (float)plot.getWidth(),10.f);
        g.setColour(grid);g.fillRoundedRectangle(mini,2.f);
        g.setColour(blue.withAlpha(.8f));
        for(int i=0;i<(int)loudHistory.size();i+=2) if(loudHistory[(size_t)i]>-95) {
            const float x=mini.getX()+(float)i/loudHistory.size()*mini.getWidth();
            const float h=normal(loudHistory[(size_t)i])*mini.getHeight();
            g.fillRect(x,mini.getBottom()-h,1.f,h);
        }
        g.setColour(muted);setFont(g,10);
        g.drawText("-72 s",graph.getX()+24,graph.getBottom()-16,70,14,juce::Justification::centredLeft);
        g.drawText("AGORA",graph.getRight()-76,graph.getBottom()-16,66,14,juce::Justification::centredRight);
    }
    const int cols=3,gap=6;
    const int tileW=(stats.getWidth()-(cols-1)*gap)/cols;
    const int tileH=(stats.getHeight()-gap*2)/3;
    const float ceilingVal=processor.state.getRawParameterValue("ceiling")->load();
    const bool alertsOn=processor.state.getRawParameterValue("alerts")->load()>.5f;
    const bool overI=alertsOn && latest.integrated > targetDb+1;
    const bool overPeak=alertsOn && latest.truePeakMax > ceilingVal;
    struct Readout {juce::String title;float value;const char* unit;bool warn;};
    std::array<Readout,9> info = {{{"INTEGRADO",latest.integrated,"LUFS",overI},
      {"CURTO PRAZO",latest.shortTerm,"LUFS",false},{juce::String::fromUTF8("MOMENTÂNEO"),latest.momentary,"LUFS",false},
      {"TRUE PEAK",latest.truePeakMax,"dBTP",overPeak},{"LRA",latest.lra,"LU",false},
      {"PLR",latest.plr,"dB",false},{"PSR",latest.psr,"dB",false},
      {juce::String::fromUTF8("MÁX. CURTO"),latest.shortTermMax,"LUFS",false},{juce::String::fromUTF8("MÁX. MOM."),latest.momentaryMax,"LUFS",false}}};
    for(int i=0;i<9;++i) paintReadout(g,{stats.getX()+(i%cols)*(tileW+gap),
          stats.getY()+(i/cols)*(tileH+gap),tileW,tileH},info[i].title,info[i].value,
          info[i].unit,info[i].warn);
}
void TremDeMedirEditor::paintDigital(juce::Graphics& g,juce::Rectangle<int> r) {
    auto area=r.reduced(15,12).withTrimmedTop(56);
    const int n=juce::jlimit(1,trem::maxChannels,latest.channels);
    const int graphTop=area.getY()+12, graphBottom=area.getBottom()-22;
    const int fullHeight=std::max(20,graphBottom-graphTop);
    const int scaleMode=(int)processor.state.getRawParameterValue("meterstyle")->load();
    const auto levelPosition=[scaleMode](float db) {
        if(scaleMode==1) return normal(db,-50,0);
        if(scaleMode>=2) return normal(db,-60,0); // K anchors are marked below
        // Extended resolution in the top 20 dB, resembling classic nonlinear metering.
        if(db>=-20) return .5f + normal(db,-20,0)*.5f;
        return normal(db,-60,-20)*.5f;
    };
    const float barSpace=std::min(64.f,(area.getWidth()-32.f)/std::max(1,n)-7);
    const float step=(area.getWidth()-38.f)/n;
    for(int c=0;c<n;++c) {
        const float bw=std::max(5.f,barSpace);
        const int x=(int)(area.getX()+26+c*step+(step-bw)*.5f);
        const float yPeak=graphBottom-levelPosition(displayLevels[c])*fullHeight;
        const float yRms=graphBottom-levelPosition(latest.rmsDb[c])*fullHeight;
        const float yMax=graphBottom-levelPosition(heldLevels[c])*fullHeight;
        const int levelType=(int)processor.state.getRawParameterValue("levelMode")->load();
        g.setColour(panelDark);g.fillRoundedRectangle((float)x,graphTop,bw,(float)fullHeight,2);
        g.setColour(levelType==1?accent:blue.withAlpha(.85f));g.fillRect((float)x,levelType==1?yPeak:yRms,bw,(float)(graphBottom-(levelType==1?yPeak:yRms)));
        if(levelType!=2){g.setColour(accent);g.fillRect((float)x,yPeak,bw,3.f);}
        g.setColour(heldLevels[c]>-0.1?red:yellow);g.fillRect((float)x,yMax,bw,2.f);
        g.setColour(muted);setFont(g,10);
        g.drawText(juce::String(c+1),x-4,graphBottom+3,(int)bw+8,15,juce::Justification::centred);
    }
    for(int db=-60;db<=0;db+=10) {
        const auto y=graphBottom-(int)(levelPosition((float)db)*fullHeight);
        g.setColour(grid);g.drawHorizontalLine(y,(float)area.getX()+23,(float)area.getRight());
        g.setColour(muted);setFont(g,9);
        g.drawText(juce::String(db),area.getX(),y-6,23,12,juce::Justification::centredRight);
    }
    if(scaleMode>=2) {
        const int anchor=scaleMode==2?-20:scaleMode==3?-14:-12;
        const int yy=graphBottom-(int)(levelPosition((float)anchor)*fullHeight);
        const float dashed[]={3.f,3.f};
        g.setColour(yellow);g.drawDashedLine({(float)area.getX()+23,(float)yy,(float)area.getRight(),(float)yy},dashed,2,1.f);
    }
    g.setColour(muted);setFont(g,11);
    g.drawText("Peak:  " + juce::String(latest.maxPeakDb[0],1) + " dBFS     RMS: " +
            juce::String(latest.rmsDb[0],1) + " dBFS",area.getX()+40,area.getY()-7,area.getWidth()-40,19,juce::Justification::centredLeft);
}
void TremDeMedirEditor::paintStereo(juce::Graphics& g,juce::Rectangle<int> r) {
    auto a=r.reduced(13,13).withTrimmedTop(23);
    if(latest.channels>2) {g.setColour(muted);setFont(g,13);g.drawFittedText(
        juce::String::fromUTF8("Vectorscópio: disponível em mono / estéreo.\n\nCanal atual: ") + juce::String(latest.channels),
        a,juce::Justification::centred,3);return;}
    const int dimension=std::min(a.getWidth()-20,a.getHeight()-55);
    auto scope=juce::Rectangle<int>(a.getCentreX()-dimension/2,a.getY()+3,dimension,dimension);
    g.setColour(panelDark);g.fillEllipse(scope.toFloat());
    g.setColour(grid);g.drawEllipse(scope.toFloat(),1);
    g.drawLine((float)scope.getCentreX(),(float)scope.getY(),(float)scope.getCentreX(),(float)scope.getBottom());
    g.drawLine((float)scope.getX(),(float)scope.getCentreY(),(float)scope.getRight(),(float)scope.getCentreY());
    g.setColour(accent.withAlpha(.43f));
    for(int i=0;i<4096;i+=12) {
        const float l=sourceLeft[(size_t)i],rchan=sourceRight[(size_t)i];
        const float x=(l-rchan)*.7071f,y=(l+rchan)*.7071f;
        const float px=scope.getCentreX()+x*dimension*.45f;
        const float py=scope.getCentreY()-y*dimension*.45f;
        if(scope.contains((int)px,(int)py))g.fillEllipse(px-1,py-1,2,2);
    }
    const float bx=(float)a.getX()+12,by=(float)scope.getBottom()+8,bw=(float)a.getWidth()-24;
    g.setGradientFill(juce::ColourGradient(red,bx,by,blue,bx+bw,by,false));g.fillRoundedRectangle(bx,by,bw,4,2);
    float marker=bx+(latest.phase+1.f)*.5f*bw;g.setColour(text);g.fillRoundedRectangle(marker-1,by-3,2,10,1);
    const auto bottom=a.getBottom()-30;
    g.setColour(muted);setFont(g,11);
    g.drawText("FASE",a.getX()+8,bottom,a.getWidth()/2-8,15,juce::Justification::centredLeft);
    g.drawText(juce::String::fromUTF8("BALANÇO"),a.getCentreX()+8,bottom,a.getWidth()/2-8,15,juce::Justification::centredLeft);
    g.setColour(latest.phase<-.1f?red:accent);setFont(g,16,true);
    g.drawText(juce::String(latest.phase,2),a.getX()+8,bottom+13,a.getWidth()/2-8,22,juce::Justification::centredLeft);
    g.setColour(text);
    g.drawText(juce::String(latest.balance,2),a.getCentreX()+8,bottom+13,a.getWidth()/2-8,22,juce::Justification::centredLeft);
}
void TremDeMedirEditor::paintAnalogue(juce::Graphics& g,juce::Rectangle<int> r) {
    auto a=r.reduced(13,12).withTrimmedTop(23);
    const bool ppm=processor.state.getRawParameterValue("analog")->load()>.5f;
    const float ref=processor.state.getRawParameterValue("vuRef")->load();
    const float data[2]={ppm?latest.ppmLeft:latest.vuLeft,ppm?latest.ppmRight:latest.vuRight};
    const int meterH=(a.getHeight()-24)/2;
    for(int ch=0;ch<2;++ch) {
        auto m=juce::Rectangle<int>(a.getX(),a.getY()+ch*(meterH+9),a.getWidth(),meterH);
        juce::Graphics::ScopedSaveState meterClip(g);
        g.reduceClipRegion(m);
        g.setColour(juce::Colour::fromRGB(24,31,43));g.fillRoundedRectangle(m.toFloat(),7);
        g.setColour(juce::Colour::fromRGB(42,45,46));
        const float cx=m.getCentreX(),cy=m.getBottom()+meterH*.23f;
        const float radius=meterH*1.1f;
        for(int i=-5;i<=3;++i) {
            const float angle=((i+5)/8.f-.5f)*2.f*.88f;
            const float x1=cx+std::sin(angle)*radius*.72f;
            const float y1=cy-std::cos(angle)*radius*.72f;
            const float x2=cx+std::sin(angle)*radius*.81f;
            const float y2=cy-std::cos(angle)*radius*.81f;
            g.setColour(i>0?red:muted);
            g.drawLine(x1,y1,x2,y2,i==0?2.f:1.f);
            g.setColour(muted);setFont(g,9);
            g.drawText(juce::String(i*2), (int)x2-10,(int)y2-14,20,13,juce::Justification::centred);
        }
        const float relativeDb=data[ch]-ref;
        const float position=juce::jlimit(0.f,1.f,(relativeDb+20.f)/26.f);
        const float angle=(position-.5f)*2.f*.88f;
        g.setColour(text);
        g.drawLine(cx,cy,cx+std::sin(angle)*radius*.83f,
                   cy-std::cos(angle)*radius*.83f,2.1f);
        g.fillEllipse(cx-4,cy-4,8,8);
        g.setColour(muted);setFont(g,10,true);
        g.drawText((ch==0?"L  ":"R  ")+juce::String(relativeDb,1)+(ppm?" PPM":" VU"),
                   m.getX()+5,m.getY()+3,m.getWidth()-10,16,juce::Justification::centredLeft);
    }
}
void TremDeMedirEditor::paintSpectrum(juce::Graphics& g,juce::Rectangle<int> r) {
    auto a=r.reduced(16,15).withTrimmedTop(62);
    auto plot=a.withTrimmedBottom(24);
    const int n=bandNumber();
    const float floorDb=-(30.f+30.f*processor.state.getRawParameterValue("rtaRange")->load());
    float barWidth=(float)plot.getWidth()/n;
    for(int i=0;i<n;++i) {
        const float x=plot.getX()+i*barWidth+2;
        const float y=plot.getBottom()-normal(bands[(size_t)i],floorDb,0)*plot.getHeight();
        const float ymax=plot.getBottom()-normal(maxBands[(size_t)i],floorDb,0)*plot.getHeight();
        g.setColour(panelDark);g.fillRect(x,(float)plot.getY(),barWidth-4,(float)plot.getHeight());
        g.setColour(i==selectedBand?yellow:blue);
        g.fillRect(x,y,barWidth-4,(float)plot.getBottom()-y);
        g.setColour(i==selectedBand?red:accent.withAlpha(.7f));
        g.fillRect(x,ymax,barWidth-4,2.f);
        if(i%std::max(1,n/10)==0 || i==n-1) {
            const float hz=20*std::pow(1000.f,(float)i/(n-1));
            juce::String label=hz>=1000?juce::String(hz/1000,1)+"k":juce::String((int)hz);
            g.setColour(muted);setFont(g,10);
            g.drawText(label,(int)x-9,plot.getBottom()+5,(int)barWidth+18,18,juce::Justification::centred);
        }
    }
    g.setColour(text);setFont(g,11,true);
    auto i=juce::jlimit(0,n-1,hoveredBand<0?selectedBand:hoveredBand);
    const float hz=20*std::pow(1000.f,(float)i/(n-1));
    g.drawText(juce::String((int)std::round(hz))+" Hz  |  " + juce::String(bands[(size_t)i],1)+
               juce::String::fromUTF8(" dBFS  |  Máx. ") + juce::String(maxBands[(size_t)i],1)+" dBFS",
               r.getRight()-345,r.getY()+9,330,18,juce::Justification::centredRight);
}
int TremDeMedirEditor::bandNumber() const {int choice=(int)processor.state.getRawParameterValue("bandCount")->load();return choice==0?31:choice==1?63:127;}
void TremDeMedirEditor::updateSpectrum() {
    processor.copyScope(sourceMono,sourceLeft,sourceRight);
    const int source=processor.state.getRawParameterValue("lrMax")->load()>.5f?3:(int)processor.state.getRawParameterValue("rtaSource")->load();
    const auto transform=[this](const std::array<float,4096>& samples){
        for(int i=0;i<4096;++i)fftBuffer[i]=samples[i]*(.5f-.5f*std::cos(2*piF*i/4095));
        std::fill(fftBuffer.begin()+4096,fftBuffer.end(),0.f);fft.performFrequencyOnlyForwardTransform(fftBuffer.data(),true);
    };
    std::array<float,2048> leftMagnitudes{};
    if(source==3){transform(sourceLeft);std::copy_n(fftBuffer.begin(),2048,leftMagnitudes.begin());}
    transform(source==1?sourceLeft:source==2||source==3?sourceRight:sourceMono);
    if(source==3)for(int k=0;k<2048;++k)fftBuffer[k]=std::max(fftBuffer[k],leftMagnitudes[k]);
    const double sr=processor.getSampleRate()>0?processor.getSampleRate():48000,stepHz=sr/4096.;
    const int n=bandNumber(),mode=(int)processor.state.getRawParameterValue("weighting")->load(),detector=(int)processor.state.getRawParameterValue("rtaMode")->load();
    if(n!=previousBands){bands.fill(-100);maxBands.fill(-100);bandAge.fill(0);previousBands=n;selectedBand=std::min(selectedBand,n-1);}
    const float holds[]={0,1,2,5};const float hold=holds[(int)processor.state.getRawParameterValue("holdTime")->load()];
    const float rate=6*std::pow(2.f,processor.state.getRawParameterValue("returnRate")->load());
    const bool infinite=processor.state.getRawParameterValue("peakHold")->load()>.5f;
    for(int i=0;i<n;++i){
        const double center=20*std::pow(1000.,(double)i/(n-1));
        if(center>sr*.5){bands[i]=maxBands[i]=-100;continue;}
        const double halfWidth=std::pow(1000.,.5/(n-1));
        int lo=std::clamp((int)std::ceil(center/halfWidth/stepHz),1,2047),hi=std::clamp((int)std::floor(center*halfWidth/stepHz),1,2047);
        if(lo>hi)lo=hi=std::clamp((int)std::round(center/stepHz),1,2047);
        double sum=0,peak=0;for(int k=lo;k<=hi;++k){double mag=fftBuffer[k]*4/4096.;sum+=mag*mag;peak=std::max(peak,mag);}
        float db=trem::toDb(detector==0?peak:std::sqrt(sum/3.));
        if(mode>0){double f2=center*center,base=12194.*12194.*f2/((f2+20.6*20.6)*(f2+12194.*12194.));db+=(float)(20*std::log10(std::max(mode==2?base:base*f2/std::sqrt((f2+107.7*107.7)*(f2+737.9*737.9)),1e-15))+(mode==2?.06:2));}
        db=std::max(-120.f,db);float alpha=std::exp(-frameSeconds/(detector==2?.6f:detector==1?.15f:.04f));
        bands[i]=bands[i]*alpha+db*(1-alpha);
        maxBands[i]=trem::heldPeak(bands[i],maxBands[i],bandAge[i],frameSeconds,hold,rate,infinite);
    }
}
void TremDeMedirEditor::paintTools(juce::Graphics& g){
    auto r=juce::Rectangle<int>(16,160,getWidth()-32,77);section(g,r,"");
    const int middle=(int)(getWidth()*.38f),session=(int)(getWidth()*.73f);
    g.setColour(grid);g.drawVerticalLine(middle,173,224);g.drawVerticalLine(session,173,224);
    g.setColour(muted);setFont(g,10,true);g.drawText("TUNER / MONO",32,169,210,15,juce::Justification::centredLeft);
    g.drawText("A4 Hz",246,171,150,15,juce::Justification::centredLeft);
    auto noSignal=juce::String::fromUTF8("—");juce::String note=noSignal,details="Sem sinal tonal";double pitchCents=0;
    if(pitch.hz>0){double midi=69+12*std::log2(pitch.hz/processor.state.getRawParameterValue("tuneRef")->load());int nearest=(int)std::round(midi);double cents=(midi-nearest)*100;pitchCents=cents;const char* names[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};note=juce::String(names[(nearest%12+12)%12])+juce::String(nearest/12-1);details=juce::String(pitch.hz,1)+" Hz  "+(cents>=0?"+":"")+juce::String(cents,1)+" ct";}
    g.setColour(text);setFont(g,27,true);g.drawText(note,32,186,85,34,juce::Justification::centredLeft);
    g.setColour(blue);setFont(g,12);g.drawText(details,116,193,120,24,juce::Justification::centredLeft);
    g.setColour(grid);g.drawLine(32,225,221,225,2);g.drawVerticalLine(126,221,229);
    if(pitch.hz>0){float marker=126.f+(float)juce::jlimit(-50.,50.,pitchCents)*1.8f;g.setColour(std::abs(pitchCents)<5?blue:accent);g.fillEllipse(marker-3,222,6,6);}
    g.setColour(muted);setFont(g,10,true);g.drawText("TEMPO / AUDIO",middle+16,169,220,15,juce::Justification::centredLeft);
    g.setColour(text);setFont(g,26,true);g.drawText(processor.detectedBpm()>0?juce::String(processor.detectedBpm()*(tempoOctave.getSelectedId()==2?.5:tempoOctave.getSelectedId()==3?2.:1.),1):noSignal,middle+16,187,112,32,juce::Justification::centredLeft);
    g.setColour(muted);setFont(g,10);g.drawText("BPM EXPERIMENTAL",middle+16,219,112,12,juce::Justification::centredLeft);
    g.drawText(processor.hostBpm()>0?"HOST "+juce::String(processor.hostBpm(),1)+" BPM":juce::String::fromUTF8("HOST —"),middle+139,188,140,16,juce::Justification::centredLeft);
    g.drawText(tapTempo>0?"TAP "+juce::String(tapTempo,1):processor.detectedBpm()>0?"CONFIANCA "+juce::String((int)std::round(processor.tempoConfidence()*100))+"%":latest.rmsDb[0]>-70?"BPM incerto: use TAP":"Aguardando pulsacao",middle+139,207,150,16,juce::Justification::centredLeft);
    g.setColour(muted);setFont(g,10,true);g.drawText("SESSAO DE MEDICAO",session+16,169,230,16,juce::Justification::centredLeft);
    g.setColour(processor.clipSamples()>0?red:blue);setFont(g,15,true);g.drawText(juce::String(latest.elapsedSeconds,1)+" s   |   "+juce::String((juce::int64)processor.clipSamples())+" amostras clip",session+16,192,getWidth()-session-46,24,juce::Justification::centredLeft);
}
void TremDeMedirEditor::timerCallback() {
    const auto now=juce::Time::getMillisecondCounterHiRes();frameSeconds=lastFrameMs>0?juce::jlimit(.001f,.2f,(float)((now-lastFrameMs)/1000.)):1.f/24;lastFrameMs=now;
    latest=processor.currentMeters();
    const float holds[]={0,1,2,5};float hold=holds[(int)processor.state.getRawParameterValue("holdTime")->load()],rate=6*std::pow(2.f,processor.state.getRawParameterValue("returnRate")->load());
    for(int c=0;c<trem::maxChannels;++c){displayLevels[c]=std::max(latest.peakDb[c],displayLevels[c]-rate*frameSeconds);heldLevels[c]=trem::heldPeak(displayLevels[c],heldLevels[c],levelAge[c],frameSeconds,hold,rate,processor.state.getRawParameterValue("peakHold")->load()>.5f);}
    processor.copyHistory(loudHistory,integratedHistory);
    updateSpectrum();
    if(++analysisFrame%4==0){double left=0,right=0;for(int i=0;i<4096;++i){left+=sourceLeft[i]*sourceLeft[i];right+=sourceRight[i]*sourceRight[i];}const auto& mono=left>=right?sourceLeft:sourceRight;pitch=trem::detectPitch(mono.data(),4096,processor.getSampleRate());}
    repaint();
}
void TremDeMedirEditor::mouseDown(const juce::MouseEvent& e) {
    auto s=panels().spectrum;
    if(s.contains(e.getPosition())) {
        auto plot=s.reduced(16,15).withTrimmedTop(62).withTrimmedBottom(24);
        selectedBand=juce::jlimit(0,bandNumber()-1,(int)((e.position.x-plot.getX())/plot.getWidth()*bandNumber()));
        repaint();
    }
}
