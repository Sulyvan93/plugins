#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

namespace {
const juce::Colour bg=juce::Colour::fromRGB(9,16,24);
const juce::Colour panel=juce::Colour::fromRGB(20,32,43);
const juce::Colour panelDark=juce::Colour::fromRGB(13,24,34);
const juce::Colour grid=juce::Colour::fromRGB(45,65,76);
const juce::Colour text=juce::Colour::fromRGB(231,239,239);
const juce::Colour muted=juce::Colour::fromRGB(141,163,173);
const juce::Colour turquoise=juce::Colour::fromRGB(89,218,197);
const juce::Colour yellow=juce::Colour::fromRGB(248,197,109);
const juce::Colour red=juce::Colour::fromRGB(254,111,115);
const juce::Colour blue=juce::Colour::fromRGB(96,159,231);
constexpr float piF = juce::MathConstants<float>::pi;
float normal(float db,float bottom=-60,float top=0) {return juce::jlimit(0.f,1.f,(db-bottom)/(top-bottom));}
void setFont(juce::Graphics& g,float sz,bool bold=false) {
    juce::Font f{juce::FontOptions(sz)};
    f.setBold(bold);
    g.setFont(f);
}
}
TremDeMedirEditor::TremDeMedirEditor(TremDeMedirProcessor& p)
    : AudioProcessorEditor(&p),processor(p) {
    setOpaque(true);
    setSize(1280,920);
    setResizable(true,true);
    setResizeLimits(1020,760,1800,1300);
    for (juce::Component* c : std::initializer_list<juce::Component*>{&preset,&gate,&history,&digitalMode,
                 &analogMode,&weighting,&rtaSource,&lfe,&logInterval,&target,&ceiling,&vuRef,
                 &highQuality,&alerts,&logging,&resetAll,&resetLoud,&undo,&redo,&ab,&copyBank,&loudView,&logFolder}) addAndMakeVisible(c);
    preset.addItemList({"Spotify / vídeos (-14)","EBU R128 (-23)","Apple referência (-16)",
        "YouTube referência (-14)","ATSC A/85 (-24)","Netflix referência (-27)",
        "Spotify Loud (-11)","Estúdio (-18)"},1);
    preset.onChange=[this] {processor.loadReferencePreset(preset.getSelectedItemIndex());};
    auto attachCombo=[this](juce::ComboBox& c,const juce::String& id,const juce::StringArray& items) {
        c.addItemList(items,1);
        style(c);
        combos.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.state,id,c));
    };
    attachCombo(gate,"gate",{"Sem gate","Gate -8 LU","Gate -10 LU"});
    attachCombo(history,"history",{"Contínuo","Play","Reset no Play"});
    attachCombo(digitalMode,"meterstyle",{"Não linear","Linear","K-20","K-14","K-12"});
    attachCombo(analogMode,"analog",{"VU","PPM"});
    attachCombo(weighting,"weighting",{"Sem peso","A","C"});
    attachCombo(rtaSource,"rtaSource",{"L + R","L","R"});
    attachCombo(lfe,"lfe",{"LFE: 4º","LFE: 6º","Sem LFE"});
    attachCombo(logInterval,"logInterval",{"1 s","5 s","10 s","30 s"});
    style(preset);preset.setSelectedId(1,juce::dontSendNotification);
    style(loudView);
    loudView.addItemList({"Histórico", "Histograma"},1);
    loudView.setSelectedId(1,juce::dontSendNotification);
    loudView.onChange=[this] {repaint();};
    auto attachSlider=[this](juce::Slider& s,const juce::String& id) {
        s.setSliderStyle(juce::Slider::LinearHorizontal);
        s.setTextBoxStyle(juce::Slider::TextBoxRight,false,62,24);
        style(s);
        sliders.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,id,s));
    };
    attachSlider(target,"target");attachSlider(ceiling,"ceiling");attachSlider(vuRef,"vuRef");
    auto attachToggle=[this](juce::ToggleButton& b,const juce::String& id,const juce::String& label) {
        b.setButtonText(label);b.setColour(juce::ToggleButton::textColourId,text);
        b.setColour(juce::ToggleButton::tickColourId,turquoise);
        toggles.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,id,b));
    };
    attachToggle(highQuality,"truepeak","TP 8x");
    attachToggle(alerts,"alerts","Alertas");
    attachToggle(logging,"logging","Gravar CSV");
    for(auto* b : {&resetAll,&resetLoud,&undo,&redo,&ab,&copyBank,&logFolder}) style(*b);
    resetAll.onClick=[this] {processor.requestReset();maxBands.fill(-100);};
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
    setWantsKeyboardFocus(true);
    startTimerHz(24);
}
TremDeMedirEditor::~TremDeMedirEditor(){stopTimer();}
void TremDeMedirEditor::style(juce::ComboBox& c) {
    c.setColour(juce::ComboBox::backgroundColourId,panel);
    c.setColour(juce::ComboBox::textColourId,text);
    c.setColour(juce::ComboBox::outlineColourId,grid);
    c.setColour(juce::ComboBox::arrowColourId,turquoise);
}
void TremDeMedirEditor::style(juce::TextButton& b) {
    b.setColour(juce::TextButton::buttonColourId,juce::Colour::fromRGB(32,54,67));
    b.setColour(juce::TextButton::textColourOffId,text);
    b.setColour(juce::TextButton::buttonOnColourId,juce::Colour::fromRGB(35,91,87));
    b.setMouseCursor(juce::MouseCursor::PointingHandCursor);
}
void TremDeMedirEditor::style(juce::Slider& s) {
    s.setColour(juce::Slider::trackColourId,turquoise);
    s.setColour(juce::Slider::thumbColourId,yellow);
    s.setColour(juce::Slider::textBoxTextColourId,text);
    s.setColour(juce::Slider::textBoxBackgroundColourId,panelDark);
    s.setColour(juce::Slider::textBoxOutlineColourId,grid);
}
TremDeMedirEditor::PanelRects TremDeMedirEditor::panels() const {
    const int margin=16,gap=10,header=117,footer=42;
    const int body=getHeight()-header-footer-margin*2-gap*2;
    const int loudH=(int)(body*0.34),middleH=(int)(body*0.34),specH=body-loudH-middleH;
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
    const int presetX=std::max(255,w/4);
    preset.setBounds(presetX,14,170,30);
    target.setBounds(presetX+183,13,190,30);
    ceiling.setBounds(presetX+385,13,190,30);
    resetAll.setBounds(w-140,14,120,30);
    const int x=16,y=72,h=29,gap=4;
    // Progressive widths fit the 1020 px minimum size.
    const int each=(w-32-10*gap)/11;
    int at=x;
    auto put=[&](juce::Component& c,int units=1){const int wd=each*units+gap*(units-1);
        c.setBounds(at,y,wd,h);at+=wd+gap;};
    put(gate);put(history);put(digitalMode);put(analogMode);
    put(weighting);put(rtaSource);put(lfe);put(logInterval);
    put(highQuality);put(alerts);put(logging);
    const auto f=panels().footer;
    const auto loudPanel=panels().loud;
    loudView.setBounds(loudPanel.getX()+245,loudPanel.getY()+7,142,24);
    resetLoud.setBounds(f.getX(),f.getY()+2,115,26);
    undo.setBounds(f.getX()+121,f.getY()+2,77,26);
    redo.setBounds(f.getX()+204,f.getY()+2,77,26);
    ab.setBounds(f.getX()+287,f.getY()+2,74,26);
    copyBank.setBounds(f.getX()+367,f.getY()+2,98,26);
    vuRef.setBounds(f.getX()+555,f.getY()+2,182,27);
    logFolder.setBounds(f.getRight()-133,f.getY()+2,132,26);
}
void TremDeMedirEditor::section(juce::Graphics& g,juce::Rectangle<int> r,const juce::String& heading) const {
    g.setColour(panel);g.fillRoundedRectangle(r.toFloat(),7);
    g.setColour(grid);g.drawRoundedRectangle(r.toFloat().reduced(.5f),7,1);
    g.setColour(turquoise);setFont(g,13,true);
    g.drawText(heading,r.reduced(15,9).removeFromTop(21),juce::Justification::centredLeft);
}
void TremDeMedirEditor::paint(juce::Graphics& g) {
    g.fillAll(bg);
    auto p=panels();
    g.setColour(turquoise);setFont(g,25,true);
    g.drawText("TREM DE MEDIR",juce::Rectangle<int>(20,9,295,37),juce::Justification::centredLeft);
    g.setColour(muted);setFont(g,11);
    g.drawText("MEDIÇÃO DE ÁUDIO • EDIÇÃO INDEPENDENTE",juce::Rectangle<int>(21,44,350,17),juce::Justification::centredLeft);
    setFont(g,11,true);g.setColour(muted);
    const int presetX=std::max(255,getWidth()/4);
    g.drawText("PRESET",juce::Rectangle<int>(presetX,0,110,15),juce::Justification::centredLeft);
    g.drawText("ALVO LUFS",juce::Rectangle<int>(presetX+183,0,100,15),juce::Justification::centredLeft);
    g.drawText("TETO dBTP",juce::Rectangle<int>(presetX+385,0,100,15),juce::Justification::centredLeft);
    section(g,p.loud,"01  •  LOUDNESS E DINÂMICA");
    section(g,p.digital,"02  •  PEAK / RMS MULTICANAL");
    section(g,p.stereo,"03  •  ESTÉREO / CORRELAÇÃO");
    section(g,p.analogue,"04  •  VU / PPM");
    section(g,p.spectrum,"05  •  RTA 31 BANDAS (1/3 DE OITAVA)");
    paintLoudness(g,p.loud);paintDigital(g,p.digital);paintStereo(g,p.stereo);
    paintAnalogue(g,p.analogue);paintSpectrum(g,p.spectrum);
    g.setColour(muted);setFont(g,11);
    g.drawText("0 VU",p.footer.getX()+481,p.footer.getY()+5,70,20,juce::Justification::centredLeft);
    if(getWidth()>1190)
        g.drawText("AUDIO TRANSPARENTE • 72 s",juce::Rectangle<int>(p.footer.getX()+755,p.footer.getY()+5,200,20),juce::Justification::centredLeft);
}
void TremDeMedirEditor::paintReadout(juce::Graphics& g,juce::Rectangle<int> r,const juce::String& name,
                                    float number,const juce::String& suffix,bool alert) const {
    g.setColour(panelDark);g.fillRoundedRectangle(r.toFloat(),5);
    g.setColour(alert?red:muted);setFont(g,11,true);
    g.drawText(name,r.reduced(8,4).removeFromTop(17),juce::Justification::centredLeft);
    g.setColour(alert?red:text);setFont(g,std::min(23.f,r.getHeight()*.34f),true);
    const auto val=number < -95 ? juce::String("—") : juce::String(number,1);
    g.drawText(val,r.getX()+7,r.getY()+22,r.getWidth()-16,28,juce::Justification::centredLeft);
    g.setColour(muted);setFont(g,10);
    g.drawText(suffix,r.getX()+8,r.getBottom()-18,r.getWidth()-16,15,juce::Justification::centredLeft);
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
        g.drawText("DISTRIBUIÇÃO DE LUFS DE CURTO PRAZO",graph.getX()+20,
                   graph.getBottom()-20,graph.getWidth()-35,15,juce::Justification::centredLeft);
    } else {
        for(int db=-60;db<=0;db+=10) {
            float y=plot.getBottom()-normal((float)db)*plot.getHeight();
            g.setColour(grid);g.drawHorizontalLine((int)y,(float)plot.getX(),(float)plot.getRight());
            g.setColour(muted);setFont(g,10);
            g.drawText(juce::String(db),graph.getX()+1,(int)y-7,20,14,juce::Justification::centredRight);
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
        g.setColour(turquoise);g.strokePath(lineI,juce::PathStrokeType(2.f));
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
    struct Readout {const char* title;float value;const char* unit;bool warn;};
    std::array<Readout,9> info = {{{"INTEGRADO",latest.integrated,"LUFS",overI},
      {"CURTO PRAZO",latest.shortTerm,"LUFS",false},{"MOMENTÂNEO",latest.momentary,"LUFS",false},
      {"TRUE PEAK",latest.truePeakMax,"dBTP",overPeak},{"LRA",latest.lra,"LU",false},
      {"PLR",latest.plr,"dB",false},{"PSR",latest.psr,"dB",false},
      {"MÁX. CURTO",latest.shortTermMax,"LUFS",false},{"MÁX. MOM.",latest.momentaryMax,"LUFS",false}}};
    for(int i=0;i<9;++i) paintReadout(g,{stats.getX()+(i%cols)*(tileW+gap),
          stats.getY()+(i/cols)*(tileH+gap),tileW,tileH},info[i].title,info[i].value,
          info[i].unit,info[i].warn);
}
void TremDeMedirEditor::paintDigital(juce::Graphics& g,juce::Rectangle<int> r) {
    auto area=r.reduced(15,12).withTrimmedTop(25);
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
    const float barSpace=std::min(13.f,(area.getWidth()-32.f)/std::max(1,n)-7);
    const float step=(area.getWidth()-38.f)/n;
    for(int c=0;c<n;++c) {
        const int x=(int)(area.getX()+26+c*step);
        const float yPeak=graphBottom-levelPosition(latest.peakDb[c])*fullHeight;
        const float yRms=graphBottom-levelPosition(latest.rmsDb[c])*fullHeight;
        const float yMax=graphBottom-levelPosition(latest.maxPeakDb[c])*fullHeight;
        const float bw=std::max(5.f,barSpace);
        g.setColour(panelDark);g.fillRoundedRectangle((float)x,graphTop,bw,(float)fullHeight,2);
        g.setColour(blue.withAlpha(.75f));g.fillRect((float)x,yRms,bw,(float)(graphBottom-yRms));
        g.setColour(turquoise);g.fillRect((float)x,yPeak,bw,3.f);
        g.setColour(latest.maxPeakDb[c]>-0.1?red:yellow);g.fillRect((float)x,yMax,bw,2.f);
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
        "Vectorscópio: disponível em mono / estéreo.\n\nCanal atual: " + juce::String(latest.channels),
        a,juce::Justification::centred,3);return;}
    const int dimension=std::min(a.getWidth()-20,a.getHeight()-55);
    auto scope=juce::Rectangle<int>(a.getCentreX()-dimension/2,a.getY()+3,dimension,dimension);
    g.setColour(panelDark);g.fillEllipse(scope.toFloat());
    g.setColour(grid);g.drawEllipse(scope.toFloat(),1);
    g.drawLine((float)scope.getCentreX(),(float)scope.getY(),(float)scope.getCentreX(),(float)scope.getBottom());
    g.drawLine((float)scope.getX(),(float)scope.getCentreY(),(float)scope.getRight(),(float)scope.getCentreY());
    g.setColour(turquoise.withAlpha(.43f));
    for(int i=0;i<4096;i+=12) {
        const float l=sourceLeft[(size_t)i],rchan=sourceRight[(size_t)i];
        const float x=(l-rchan)*.7071f,y=(l+rchan)*.7071f;
        const float px=scope.getCentreX()+x*dimension*.45f;
        const float py=scope.getCentreY()-y*dimension*.45f;
        if(scope.contains((int)px,(int)py))g.fillEllipse(px-1,py-1,2,2);
    }
    const auto bottom=a.getBottom()-30;
    g.setColour(muted);setFont(g,11);
    g.drawText("FASE",a.getX()+8,bottom,a.getWidth()/2-8,15,juce::Justification::centredLeft);
    g.drawText("BALANÇO",a.getCentreX()+8,bottom,a.getWidth()/2-8,15,juce::Justification::centredLeft);
    g.setColour(latest.phase<-.1f?red:turquoise);setFont(g,16,true);
    g.drawText(juce::String(latest.phase,2),a.getX()+8,bottom+13,a.getWidth()/2-8,22,juce::Justification::centredLeft);
    g.setColour(text);
    g.drawText(juce::String(latest.balance,2),a.getCentreX()+8,bottom+13,a.getWidth()/2-8,22,juce::Justification::centredLeft);
}
void TremDeMedirEditor::paintAnalogue(juce::Graphics& g,juce::Rectangle<int> r) {
    auto a=r.reduced(13,12).withTrimmedTop(23);
    const bool ppm=processor.state.getRawParameterValue("analog")->load()>.5f;
    const float ref=processor.state.getRawParameterValue("vuRef")->load();
    const float data[2]={ppm?latest.ppmL:latest.vuLeft,ppm?latest.ppmR:latest.vuRight};
    const int meterH=(a.getHeight()-24)/2;
    for(int ch=0;ch<2;++ch) {
        auto m=juce::Rectangle<int>(a.getX(),a.getY()+ch*(meterH+9),a.getWidth(),meterH);
        g.setColour(juce::Colour::fromRGB(229,223,203));g.fillRoundedRectangle(m.toFloat(),7);
        g.setColour(juce::Colour::fromRGB(42,45,46));
        const float cx=m.getCentreX(),cy=m.getBottom()+meterH*.23f;
        const float radius=meterH*1.1f;
        for(int i=-5;i<=3;++i) {
            const float angle=((i+5)/8.f-.5f)*2.f*.88f;
            const float x1=cx+std::sin(angle)*radius*.72f;
            const float y1=cy-std::cos(angle)*radius*.72f;
            const float x2=cx+std::sin(angle)*radius*.81f;
            const float y2=cy-std::cos(angle)*radius*.81f;
            g.setColour(i>0?red:juce::Colour::fromRGB(52,60,62));
            g.drawLine(x1,y1,x2,y2,i==0?2.f:1.f);
            g.setColour(juce::Colours::black);setFont(g,9);
            g.drawText(juce::String(i*2), (int)x2-10,(int)y2-14,20,13,juce::Justification::centred);
        }
        const float relativeDb=data[ch]-ref;
        const float position=juce::jlimit(0.f,1.f,(relativeDb+20.f)/26.f);
        const float angle=(position-.5f)*2.f*.88f;
        g.setColour(juce::Colour::fromRGB(38,45,47));
        g.drawLine(cx,cy,cx+std::sin(angle)*radius*.83f,
                   cy-std::cos(angle)*radius*.83f,2.1f);
        g.fillEllipse(cx-4,cy-4,8,8);
        g.setColour(juce::Colour::fromRGB(35,45,47));setFont(g,10,true);
        g.drawText((ch==0?"L  ":"R  ")+juce::String(relativeDb,1)+(ppm?" PPM":" VU"),
                   m.getX()+5,m.getY()+3,m.getWidth()-10,16,juce::Justification::centredLeft);
    }
}
void TremDeMedirEditor::paintSpectrum(juce::Graphics& g,juce::Rectangle<int> r) {
    auto a=r.reduced(16,15).withTrimmedTop(24);
    auto plot=a.withTrimmedBottom(24);
    const int n=31;
    float barWidth=(float)plot.getWidth()/n;
    for(int i=0;i<n;++i) {
        const float x=plot.getX()+i*barWidth+2;
        const float y=plot.getBottom()-normal(bands[(size_t)i],-90,0)*plot.getHeight();
        const float ymax=plot.getBottom()-normal(maxBands[(size_t)i],-90,0)*plot.getHeight();
        g.setColour(panelDark);g.fillRect(x,(float)plot.getY(),barWidth-4,(float)plot.getHeight());
        g.setColour(i==selectedBand?yellow:blue);
        g.fillRect(x,y,barWidth-4,(float)plot.getBottom()-y);
        g.setColour(i==selectedBand?red:turquoise.withAlpha(.7f));
        g.fillRect(x,ymax,barWidth-4,2.f);
        if(i%3==0 || i==30) {
            const float hz=20*std::pow(2.f,i/3.f);
            juce::String label=hz>=1000?juce::String(hz/1000,1)+"k":juce::String((int)hz);
            g.setColour(muted);setFont(g,10);
            g.drawText(label,(int)x-9,plot.getBottom()+5,(int)barWidth+18,18,juce::Justification::centred);
        }
    }
    g.setColour(text);setFont(g,11,true);
    auto i=juce::jlimit(0,30,hoveredBand<0?selectedBand:hoveredBand);
    const float hz=20*std::pow(2.f,i/3.f);
    g.drawText(juce::String(hz,0)+" Hz  |  " + juce::String(bands[(size_t)i],1)+
               " dBFS  |  Máx. " + juce::String(maxBands[(size_t)i],1)+" dBFS",
               r.getRight()-345,r.getY()+9,330,18,juce::Justification::centredRight);
}
void TremDeMedirEditor::updateSpectrum() {
    processor.copyScope(sourceMono,sourceLeft,sourceRight);
    const int source=(int)processor.state.getRawParameterValue("rtaSource")->load();
    const auto& samples=source==1?sourceLeft:source==2?sourceRight:sourceMono;
    for(int i=0;i<4096;++i) {
        const float hann=.5f-.5f*std::cos(2*piF*i/4095);
        fftBuffer[(size_t)i]=samples[(size_t)i]*hann;
    }
    std::fill(fftBuffer.begin()+4096,fftBuffer.end(),0.f);
    fft.performFrequencyOnlyForwardTransform(fftBuffer.data(),true);
    const double sr=processor.getSampleRate()>0?processor.getSampleRate():48000;
    const double stepHz=sr/4096.;
    const int mode=(int)processor.state.getRawParameterValue("weighting")->load();
    for(int i=0;i<31;++i) {
        const double center=20*std::pow(2.,i/3.);
        const int lo=std::max(1,(int)std::ceil(center*std::pow(2.,-1./6.)/stepHz));
        const int hi=std::min(2047,(int)std::floor(center*std::pow(2.,1./6.)/stepHz));
        double sum=0;int count=0;
        for(int k=lo;k<=hi;++k) {double mag=fftBuffer[(size_t)k]*4/4096.;sum+=mag*mag;++count;}
        float db=count? trem::toDb(std::sqrt(sum/count)) : -100;
        if(mode>0) {
            const double f2=center*center;
            const double base=std::pow(12194.,2)*f2/((f2+20.6*20.6)*(f2+12194.*12194.));
            if(mode==2) db+=(float)(20*std::log10(std::max(base,1e-15))+0.06);
            else {
                const double adj=std::sqrt((f2+107.7*107.7)*(f2+737.9*737.9));
                db+=(float)(20*std::log10(std::max(base*f2/adj,1e-15))+2.0);
            }
        }
        bands[(size_t)i]=bands[(size_t)i]*.55f + db*.45f;
        maxBands[(size_t)i]=std::max(maxBands[(size_t)i]-.12f,bands[(size_t)i]);
    }
}
void TremDeMedirEditor::timerCallback() {
    latest=processor.currentMeters();
    processor.copyHistory(loudHistory,integratedHistory);
    updateSpectrum();
    repaint();
}
void TremDeMedirEditor::mouseDown(const juce::MouseEvent& e) {
    auto s=panels().spectrum;
    if(s.contains(e.getPosition())) {
        auto plot=s.reduced(16,15).withTrimmedTop(24).withTrimmedBottom(24);
        selectedBand=juce::jlimit(0,30,(int)((e.position.x-plot.getX())/plot.getWidth()*31));
        repaint();
    }
}
