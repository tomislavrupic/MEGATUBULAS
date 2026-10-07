#include "Editor.h"
#include <BinaryData.h>
#include "AnimationClock.h"
#include "VisualResponse.h"
namespace {const auto cream=juce::Colour(0xffe9dfce),amber=juce::Colour(0xffffa747),violet=juce::Colour(0xffac80ff),cyan=juce::Colour(0xff63dfff);
juce::Font font(float n,bool bold=false){return juce::Font(juce::FontOptions(n,bold?juce::Font::bold:juce::Font::plain));}
void text(juce::Graphics& g,juce::String s,juce::Rectangle<float> r,float size,juce::Colour c=cream){g.setColour(c);g.setFont(font(size));g.drawText(s,r,juce::Justification::centred);}
}
MetalLook::MetalLook(){setColour(juce::Slider::textBoxTextColourId,cream);setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);setColour(juce::Slider::textBoxBackgroundColourId,juce::Colours::transparentBlack);setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff19191b));setColour(juce::ComboBox::textColourId,cream);setColour(juce::ComboBox::outlineColourId,juce::Colour(0xff625a50));setColour(juce::TextButton::buttonColourId,juce::Colour(0xff1e1f22));setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff49305e));setColour(juce::TextButton::textColourOffId,cream);setColour(juce::TextButton::textColourOnId,cream);}
void MetalLook::drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float position,float start,float end,juce::Slider& s){
 const float cx=float(x)+float(w)*.5f,cy=float(y)+float(h)*.5f;
 const float r=(float(std::min(w,h))-32.f)*.5f;
 const float angle=start+position*(end-start);
 const auto colour=s.findColour(juce::Slider::rotarySliderFillColourId);
 auto point=[&](float a,float radius){return juce::Point<float>(cx+std::sin(a)*radius,cy-std::cos(a)*radius);};
 // Fixed panel markings, with a restrained illuminated position dot.
 for(int tick=0;tick<=20;++tick){const float a=start+(end-start)*float(tick)/20.f;const bool major=tick%5==0;
  g.setColour(cream.withAlpha(major?.78f:.34f));g.drawLine({point(a,r+5),point(a,r+(major?12.f:8.f))},major?1.5f:1.f);
 }
 const auto lamp=point(angle,r+15);g.setColour(colour.withAlpha(.13f));g.fillEllipse(lamp.x-4,lamp.y-4,8,8);g.setColour(colour);g.fillEllipse(lamp.x-1.6f,lamp.y-1.6f,3.2f,3.2f);
 // Raised black phenolic skirt: scalloped silhouette and fluted grip.
 g.setColour(juce::Colours::black.withAlpha(.5f));g.fillEllipse(cx-r-5,cy-r+3,2*r+10,2*r+10);
 g.setColour(juce::Colours::black.withAlpha(.7f));g.fillEllipse(cx-r-1,cy-r+3,2*r+2,2*r+2);
 juce::Path skirt;
 for(int i=0;i<=256;++i){const float a=angle+float(i)*juce::MathConstants<float>::twoPi/256.f;
  const float radius=r*(.966f+.034f*std::cos((a-angle)*16.f));const auto p=point(a,radius);
  if(i==0)skirt.startNewSubPath(p);else skirt.lineTo(p);
 }skirt.closeSubPath();
 g.setGradientFill(juce::ColourGradient(juce::Colour(0xff454440),cx-r*.65f,cy-r,juce::Colour(0xff08090a),cx+r*.55f,cy+r,false));g.fillPath(skirt);
 g.setColour(juce::Colour(0xff65625c).withAlpha(.65f));g.strokePath(skirt,juce::PathStrokeType(.85f));
 for(int i=0;i<32;++i){const float a=angle+float(i)*juce::MathConstants<float>::twoPi/32.f;
  const float light=.10f+.15f*std::max(0.f,-std::cos(a));
  g.setColour(juce::Colours::white.withAlpha(light));g.drawLine({point(a-.016f,r*.78f),point(a-.016f,r*.955f)},1.2f);
  g.setColour(juce::Colours::black.withAlpha(.8f));g.drawLine({point(a+.018f,r*.79f),point(a+.018f,r*.955f)},1.5f);
 }
 const float body=r*.79f;g.setGradientFill(juce::ColourGradient(juce::Colour(0xff2e3030),cx,cy-body,juce::Colour(0xff0d0e10),cx,cy+body,false));g.fillEllipse(cx-body,cy-body,2*body,2*body);
 g.setColour(juce::Colour(0xff5d5b55));g.drawEllipse(cx-body,cy-body,2*body,2*body,.7f);
 // Smaller aluminium insert, with a beveled rim and fine circular machining.
 const float cap=r*.61f;g.setColour(juce::Colours::black);g.fillEllipse(cx-cap-2,cy-cap-2,2*cap+4,2*cap+4);
 g.setGradientFill(juce::ColourGradient(juce::Colour(0xffeee7d6),cx-cap,cy-cap,juce::Colour(0xff504f4c),cx+cap,cy+cap,false));g.fillEllipse(cx-cap,cy-cap,2*cap,2*cap);
 const float face=cap-2.5f;g.setGradientFill(juce::ColourGradient(juce::Colour(0xffceccc4),cx-face*.75f,cy-face,juce::Colour(0xff6f706e),cx+face*.8f,cy+face,false));g.fillEllipse(cx-face,cy-face,2*face,2*face);
 for(int i=2;i<24;++i){const float rr=face*float(i)/24.f;g.setColour((i%2?juce::Colours::white:juce::Colours::black).withAlpha(.045f));g.drawEllipse(cx-rr,cy-rr,2*rr,2*rr,.5f);}
 g.setColour(juce::Colours::white.withAlpha(.24f));g.drawEllipse(cx-face,cy-face,2*face,2*face,.7f);
 // Ivory index stays readable on the black shoulder at every value.
 g.setColour(juce::Colours::black.withAlpha(.6f));g.drawLine({point(angle,r*.65f),point(angle,r*.93f)},4.7f);
 g.setColour(cream);g.drawLine({point(angle,r*.66f),point(angle,r*.92f)},3.f);
}
Lattice::Lattice(MicroProcessor& p):processor(p){
 artwork=juce::ImageCache::getFromMemory(BinaryData::latticestripv2_png,BinaryData::latticestripv2_pngSize);
 static const auto sharedFrames=[] {
  auto result=std::make_shared<std::vector<juce::Image>>();result->reserve(31);
  for(int i=0;i<=120;i+=4){int bytes=0;auto name="micro_"+juce::String(i).paddedLeft('0',5)+"_png";auto* data=BinaryData::getNamedResource(name.toRawUTF8(),bytes);if(!data)continue;
   auto frame=juce::ImageFileFormat::loadFrom(data,size_t(bytes));if(!frame.isValid())continue;
   frame=frame.rescaled(900,513,juce::Graphics::highResamplingQuality);
   // Texture import: discard low-alpha codec background residue, retaining the supplied RGB/alpha artwork.
   for(int y=0;y<frame.getHeight();++y)for(int x=0;x<frame.getWidth();++x){auto c=frame.getPixelAt(x,y);float a=c.getFloatAlpha();a=juce::jlimit(0.f,1.f,(a-.04f)/.96f);frame.setPixelAt(x,y,c.withAlpha(a));}
   result->push_back(frame.getClippedImage({0,85,900,315}));
  }return std::shared_ptr<const std::vector<juce::Image>>(result);
 }();frames=sharedFrames;
 if(frames&&!frames->empty())blendedFrame=juce::Image(juce::Image::ARGB,frames->front().getWidth(),frames->front().getHeight(),true);
 lastTick=juce::Time::getMillisecondCounterHiRes();drivePosition=processor.state.getRawParameterValue("drive")->load()/100;
 setInterceptsMouseClicks(false,false);beads.reserve(3*13*24);
 for(int tube=0;tube<3;++tube)for(int strand=0;strand<13;++strand)for(int i=0;i<24;++i){float a=float(strand)*juce::MathConstants<float>::twoPi/13+float(i)*.22f;beads.push_back({float(tube)*.33f+.025f+float(i)*.0105f+std::cos(a)*.022f,.5f+std::sin(a)*.31f,std::cos(a),tube});}
 std::sort(beads.begin(),beads.end(),[](auto a,auto b){return a.z<b.z;});
 // Fixed depth 3, 14 segments per seed, cached in normalized coordinates.
 auto branch=[&](auto&& self,float x,float y,float angle,float len,int depth)->void{float nx=x+std::cos(angle)*len,ny=y+std::sin(angle)*len;branches.startNewSubPath(x,y);branches.lineTo(nx,ny);if(depth>0){self(self,nx,ny,angle+.6f,len*.58f,depth-1);self(self,nx,ny,angle-.6f,len*.58f,depth-1);}};
 for(int i=0;i<16;++i)branch(branch,i/16.f,.5f,i%2?-.8f:.8f,.045f,3);
}
void Lattice::tick(){bool moving=processor.state.getRawParameterValue("motion")->load()>.5f;
 const char* ids[]={"drive","memory","coupling"};float change=0;for(size_t i=0;i<3;++i){float v=processor.state.getRawParameterValue(ids[i])->load()/100;change+=std::abs(v-priorParameters[i]);priorParameters[i]=v;}activity=std::max(activity*.88f,std::min(1.f,change*8));
 double now=juce::Time::getMillisecondCounterHiRes(),dt=std::clamp((now-lastTick)*.001,0.,.25);lastTick=now;drivePosition+=float(1-std::exp(-dt/.05))*(priorParameters[0]-drivePosition);
 const float memory=priorParameters[1],variation=processor.state.getRawParameterValue("variation")->load()/100;
 afterglow+=float(1-std::exp(-dt/.3))*(std::min(1.f,processor.memoryMeter.load())-afterglow);
 const float peak=std::max(processor.inMeter.load(),processor.outMeter.load());
 const float lightTarget=juce::jlimit(0.f,1.f,(20.f*std::log10(std::max(peak,1.e-9f))+54.f)/48.f);
 const double lightTau=lightTarget>audioLight?.08:.35+.65*double(memory);
 audioLight+=float(1-std::exp(-dt/lightTau))*(lightTarget-audioLight);
 playbackSpeed+=float(1-std::exp(-dt/.4))*(micro::animationSpeed(memory,variation)-playbackSpeed);
 if(moving){phase+=float(dt*.3);loopSeconds=std::fmod(loopSeconds+dt*playbackSpeed,2.);noiseTime+=float(dt*(.35+.25*variation));}
 for(int y=0;y<noiseHeight;++y)for(int x=0;x<noiseWidth;++x)noiseField[size_t(y*noiseWidth+x)]=micro::organicVisualNoise(float(x)*5/float(noiseWidth-1),float(y)*2/float(noiseHeight-1),noiseTime);
 ++renderRevision;
 int event=processor.eventCount.load();if(event!=lastEvent){pulse=.3f;lastEvent=event;}else pulse*=.9f;repaint();}

void Lattice::drawLayer(juce::Graphics& g,int tube,bool connections){
 auto w=float(getWidth()),h=float(getHeight());float drive=processor.state.getRawParameterValue("drive")->load()/100,cp=processor.state.getRawParameterValue("coupling")->load()/100;
 if(connections){g.setColour(amber.withAlpha(.15f+.5f*cp));g.strokePath(branches,juce::PathStrokeType(.7f+cp),juce::AffineTransform::scale(w,h));for(int j=0;j<6;++j){juce::Path p;p.startNewSubPath(0,h*(.15f+j*.13f));p.cubicTo(w*.32f,h*(.8f-j*.12f),w*.65f,h*(.15f+j*.12f),w,h*(.8f-j*.13f));g.setColour((j%2?violet:cyan).withAlpha(.07f+.22f*cp));g.strokePath(p,juce::PathStrokeType(.8f+cp));}return;}
 auto c=std::array{amber,violet,cyan}[size_t(tube)];
 for(auto b:beads)if(b.tube==tube){float breathing=processor.state.getRawParameterValue("motion")->load()>.5f?std::sin(phase+float(tube))*.012f*afterglow:0;float x=b.x*w,y=(b.y+breathing)*h,r=2.2f+1.5f*(b.z+1)*.5f;
  float alpha=.25f+.75f*(b.z+1)*.5f;
  g.setColour(c.withAlpha((.04f+.08f*drive+.06f*afterglow+.04f*pulse)*alpha));g.fillEllipse(x-r*3,y-r*3,r*6,r*6);
  g.setGradientFill(juce::ColourGradient(c.brighter(.4f).withAlpha(alpha),x-r*.4f,y-r*.5f,juce::Colour(0xff242426).withAlpha(alpha),x+r,y+r,true));g.fillEllipse(x-r,y-r,r*2,r*2);
  g.setColour(cream.withAlpha(alpha*.7f));g.fillEllipse(x-r*.4f,y-r*.6f,r*.5f,r*.5f);
 }
}
void Lattice::drawArtwork(juce::Graphics& g){
 float brightness=std::min(1.f,.64f+.05f*drivePosition+.25f*audioLight+.06f*afterglow);
 if(frames&&!frames->empty()){
  float position=std::min(float(frames->size()-1),micro::animationFrame(drivePosition,loopSeconds,processor.state.getRawParameterValue("motion")->load()>.5f));
  auto i=size_t(position),next=std::min(i+1,frames->size()-1);float fraction=position-float(i);
  if(lastRenderRevision!=renderRevision){
   juce::Image::BitmapData a((*frames)[i],juce::Image::BitmapData::readOnly),b((*frames)[next],juce::Image::BitmapData::readOnly),out(blendedFrame,juce::Image::BitmapData::writeOnly);
   const unsigned weight=unsigned(juce::jlimit(0,256,int(std::round(fraction*256))));
   for(int y=0;y<out.height;++y){auto* pa=reinterpret_cast<const juce::PixelARGB*>(a.getLinePointer(y));auto* pb=reinterpret_cast<const juce::PixelARGB*>(b.getLinePointer(y));auto* dest=reinterpret_cast<juce::PixelARGB*>(out.getLinePointer(y));
    const float ny=float(y)*float(noiseHeight-1)/float(out.height-1);const int iy=std::min(noiseHeight-2,int(ny));const float fy=ny-float(iy);
    for(int x=0;x<out.width;++x){const float nx=float(x)*float(noiseWidth-1)/float(out.width-1);const int ix=std::min(noiseWidth-2,int(nx));const float fx=nx-float(ix);
     const size_t offset=size_t(iy*noiseWidth+ix);const float n0=noiseField[offset]*(1-fx)+noiseField[offset+1]*fx,n1=noiseField[offset+noiseWidth]*(1-fx)+noiseField[offset+noiseWidth+1]*fx;
     const float illumination=.90f+(.06f+.20f*audioLight)*(n0*(1-fy)+n1*fy);
     const auto alpha=micro::mixVisualByte(pa[x].getAlpha(),pb[x].getAlpha(),weight);
     auto channel=[&](unsigned aa,unsigned bb){return juce::uint8(std::min(int(alpha),int(std::round(micro::mixVisualByte(aa,bb,weight)*illumination))));};
     dest[x].setARGB(alpha,channel(pa[x].getRed(),pb[x].getRed()),channel(pa[x].getGreen(),pb[x].getGreen()),channel(pa[x].getBlue(),pb[x].getBlue()));
    }
   }lastRenderRevision=renderRevision;
  }
  // One premultiplied RGBA blend avoids the mid-crossfade dimming of two source-over draws.
  auto r=getLocalBounds().toFloat();g.setOpacity(brightness);g.drawImage(blendedFrame,r,juce::RectanglePlacement::centred);
 }else{g.setOpacity(brightness);float imageH=getWidth()*float(artwork.getHeight())/float(artwork.getWidth());g.drawImage(artwork,{0,(getHeight()-imageH)*.5f,float(getWidth()),imageH});}
 g.setOpacity(1);
}
void Lattice::paint(juce::Graphics& g){drawArtwork(g);drawLayer(g,0,true);}

MicroEditor::MicroEditor(MicroProcessor& p):AudioProcessorEditor(p),processor(p),lattice(p){
 setLookAndFeel(&look);setResizable(true,true);setResizeLimits(960,640,1800,1450);setSize(1320,880);
 texture=juce::ImageCache::getFromMemory(BinaryData::framebackgroundv1_png,BinaryData::framebackgroundv1_pngSize);
 addAndMakeVisible(lattice);
 const char* ids[]={"drive","memory","coupling","blend","output","low","lowMid","highMid","high","variation","input","wetLevel"};
 const char* names[]={"DRIVE","MEMORY","COUPLING","BLEND","OUTPUT","LOW 100 Hz","LOW MID 500 Hz","HIGH MID 1.5 kHz","HIGH 5 kHz","VARIATION","INPUT TRIM","WET LEVEL"};
 const char* hints[]={"Three nonlinear stages. Small-signal gain calibrated; increasing drive compresses peaks.","Fast attack with 12 ms to 2.4 seconds release. Higher Memory deepens sag and lets notes recover more slowly; also slows visual movement.","Earlier stages push the next: bass-focused feed-forward, stronger drive interaction and asymmetry. Higher Coupling adds weight and harmonic texture.","Latency-aligned dry/wet mix. Conversion filters still affect phase.","Output trim. No hidden limiter; red output meter means over 0 dBFS.","Low shelf, nominal 100 Hz.","Bell EQ, nominal 500 Hz, Q 0.707.","Bell EQ, nominal 1.5 kHz, Q 0.707.","High shelf, nominal 5 kHz.","0: deterministic base voice. Higher: constrained stochastic candidate selection, with faster organic visual movement.","Trim applied before analysis and to both paths.","Level of the distorted path only, before the Blend. Output trims the final mix."};
 for(size_t i=0;i<knobs.size();++i){auto& k=knobs[i];addAndMakeVisible(k);k.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);k.setTextBoxStyle(juce::Slider::TextBoxBelow,false,90,22);k.setColour(juce::Slider::rotarySliderFillColourId,i==0||i==4?amber:i==1||i==3||i==10?cyan:violet);k.setTooltip(hints[i]);k.setTitle(names[i]);k.setWantsKeyboardFocus(true);auto* param=p.state.getParameter(ids[i]);k.setDoubleClickReturnValue(true,param->convertFrom0to1(param->getDefaultValue()));k.setTextValueSuffix(i==4||i>=5&&i<=8||i>=10?" dB":" %");sliders.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,ids[i],k));auto& l=labels[i];addAndMakeVisible(l);l.setText(names[i],juce::dontSendNotification);l.setFont(font(i<5?14.f:11.f,true));l.setColour(juce::Label::textColourId,cream);l.setJustificationType(juce::Justification::centred);}
 preLow.addItemList({"BASS: TIGHT","BASS: FLAT","BASS: FULL"},1);preHigh.addItemList({"TREBLE: SOFT","TREBLE: FLAT","TREBLE: BRIGHT"},1);
 preLowLink=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"preLow",preLow);preHighLink=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"preHigh",preHigh);
 mode.addItemList({"WARM","TENSE","OPEN"},1);quality.addItemList({"4x FIR","8x FIR"},1);preset.addItemList({"Bass Foundation","Synth Amber","Drum Tension","Gentle Mix Colour","Fractal Motion"},1);preset.setText("STARTING POINTS",juce::dontSendNotification);preset.onChange=[this]{if(preset.getSelectedItemIndex()>=0)processor.setCurrentProgram(preset.getSelectedItemIndex());};
 modeLink=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"mode",mode);qualityLink=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"quality",quality);
 for(auto* c:{&mode,&quality,&preset,&source,&preLow,&preHigh}){addAndMakeVisible(c);c->setWantsKeyboardFocus(true);}
 source.addItemList({"Seeded PRNG","Imported entropy","Saved sequence"},1);source.setSelectedId(p.entropyConfig().kind+1,juce::dontSendNotification);source.onChange=[this]{auto c=processor.entropyConfig();c.kind=source.getSelectedId()-1;if(!processor.setEntropy(c)){detail.setText("Import a non-empty sequence first (or queue busy).",juce::dontSendNotification);source.setSelectedId(processor.entropyConfig().kind+1,juce::dontSendNotification);}};
 for(auto* b:{&freeze,&bypass,&match,&explore,&resetButton,&advanced,&importButton,&seedButton}){addAndMakeVisible(b);b->setWantsKeyboardFocus(true);}
 freeze.setClickingTogglesState(true);bypass.setClickingTogglesState(true);
 buttons.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"freeze",freeze));buttons.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"bypass",bypass));
 for(auto* b:{&motion,&ablate})addAndMakeVisible(b);
 buttons.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"motion",motion));buttons.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"ablate",ablate));
 match.onClick=[this]{processor.matchRequested=true;};resetButton.onClick=[this]{processor.resetRequested=true;};explore.onClick=[this]{processor.exploreRequested=true;};advanced.onClick=[this]{expanded=!expanded;setSize(getWidth(),int(float(getWidth())*(expanded?1060.f:880.f)/1320));resized();};importButton.onClick=[this]{doImport();};
 freeze.setTooltip("Locks the selected voice; audio-dependent memory still evolves. Reset clears histories.");quality.setTooltip("Applied on the next host prepare/restart. Never rebuilds filters on the audio thread.");match.setTooltip("Measures dry/wet RMS for two seconds then holds ±12 dB. Silence retains previous gain.");explore.setTooltip("Requests a fresh decision at the next 20 ms control tick. Freeze prevents selection.");
 addAndMakeVisible(seed);seed.setText(juce::String(p.entropyConfig().seed));seed.setInputRestrictions(10,"0123456789");seedButton.onClick=[this]{auto n=seed.getText().getLargeIntValue();if(n>=0&&n<=4294967295LL){auto c=processor.entropyConfig();c.seed=uint32_t(n);if(!processor.setEntropy(c))detail.setText("Configuration queue busy",juce::dontSendNotification);}};
 for(auto* l:{&status,&detail}){addAndMakeVisible(l);l->setColour(juce::Label::textColourId,cream.withAlpha(.8f));l->setFont(font(11));}
 startTimerHz(30);timerCallback();resized();
}
MicroEditor::~MicroEditor(){stopTimer();setLookAndFeel(nullptr);}
void MicroEditor::doImport(){chooser=std::make_unique<juce::FileChooser>("Import bounded entropy JSON",juce::File{},"*.json");chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[safe=juce::Component::SafePointer<MicroEditor>(this)](const juce::FileChooser& fc){if(!safe)return;auto file=fc.getResult();if(!file.existsAsFile())return;juce::String error;if(file.getSize()>65536||!safe->processor.importEntropy(file.loadFileAsString(),error))safe->detail.setText(error.isEmpty()?"File exceeds 64 KiB":error,juce::dontSendNotification);else{safe->source.setSelectedId(safe->processor.entropyConfig().kind+1,juce::dontSendNotification);safe->detail.setText("Imported. Provenance supplied by file, not independently verified.",juce::dontSendNotification);}});}
void MicroEditor::timerCallback(){lattice.tick();auto c=processor.entropyConfig();juce::String s=c.kind==0?"SEEDED PRNG / NOT QUANTUM":c.kind==1?"IMPORTED / PROVENANCE UNVERIFIED":"SAVED SEQUENCE";s+="  /  VOICE "+juce::String(processor.selection.load()+1);if(processor.entropyExhausted.load())s+="  /  EXHAUSTED / HOLD";int desired=processor.state.getRawParameterValue("quality")->load()>.5f?8:4;if(desired!=processor.activeQuality.load())s+="  /  QUALITY PENDING RESTART";if(processor.matchState.load()==1)s+="  /  MEASURING RMS";if(processor.matchState.load()==2)s+="  /  MATCH "+juce::String(processor.matchDb.load(),1)+" dB HELD";if(processor.matchState.load()==3)s+="  /  SILENT MATCH / UNCHANGED";status.setText("v"+juce::String(JucePlugin_VersionString)+" / "+s,juce::dontSendNotification);repaint();}
void MicroEditor::resized(){float sx=getWidth()/1320.f,sy=getHeight()/(expanded?1060.f:880.f);auto bounds=[sx,sy](juce::Component& c,float x,float y,float w,float h){c.setBounds(juce::Rectangle<float>(x*sx,y*sy,w*sx,h*sy).toNearestInt());};
 const float xs[]={220,490,715,970,1145};for(int i=0;i<5;++i){float sz=i==0?190:155;bounds(knobs[size_t(i)],xs[i]-sz*.5f,106,sz,175);bounds(labels[size_t(i)],xs[i]-100,283,200,24);}
 bounds(lattice,245,313,830,288);
 for(int i=5;i<10;++i){float x=i==9?850.f:140.f+(i-5)*175.f;bounds(knobs[size_t(i)],x-60,633,120,110);bounds(labels[size_t(i)],x-82,744,164,20);}
 bounds(mode,1020,655,200,34);bounds(preset,985,710,260,30);
 bounds(quality,445,770,110,32);bounds(match,575,770,120,32);bounds(bypass,715,770,120,32);bounds(freeze,855,770,110,32);bounds(explore,985,770,110,32);bounds(resetButton,1115,770,105,32);
 bounds(status,80,820,1000,20);bounds(advanced,1050,816,200,24);
 bool visible=expanded;for(auto* c:std::array<juce::Component*,13>{&knobs[10],&labels[10],&knobs[11],&labels[11],&preLow,&preHigh,&source,&seed,&seedButton,&importButton,&motion,&ablate,&detail})c->setVisible(visible);
 bounds(knobs[10],65,895,120,100);bounds(labels[10],65,995,120,18);bounds(knobs[11],185,895,120,100);bounds(labels[11],185,995,120,18);bounds(preLow,345,954,200,30);bounds(preHigh,565,954,200,30);bounds(source,345,906,200,30);bounds(seed,565,906,100,30);bounds(seedButton,680,906,90,30);bounds(importButton,785,906,145,30);bounds(motion,960,904,250,26);bounds(ablate,960,943,310,26);bounds(detail,345,995,800,45);
}
void MicroEditor::drawFrame(juce::Graphics& g){float h=expanded?1060.f:880.f;g.drawImage(texture,0,0,1320,300,0,0,texture.getWidth(),int(texture.getHeight()*320.f/760));g.drawImage(texture,0,300,1320,320,0,int(texture.getHeight()*320.f/760),texture.getWidth(),int(texture.getHeight()*128.f/760));g.drawImage(texture,0,620,1320,260,0,int(texture.getHeight()*448.f/760),texture.getWidth(),texture.getHeight()-int(texture.getHeight()*448.f/760));if(expanded){g.setColour(juce::Colour(0xff17191b));g.fillRect(25.f,880.f,1270.f,h-890);g.setColour(cream.withAlpha(.3f));g.drawRect(25.f,880.f,1270.f,h-890);}
 text(g,"M E G A T U B U L A S",{260,25,800,47},38);text(g,"P I X E L   R E C O R D S   /   S T A T E F U L   S A T U R A T I O N",{280,74,760,15},11,cream.withAlpha(.65f));
 text(g,juce::String::fromUTF8("Size is a matter of perspective. Drive isn’t."),{340,91,640,15},10,cream.withAlpha(.7f));
 for(int side=0;side<2;++side)for(int i=0;i<50;++i){float x=side?1240-i*4.f:80+i*4.f,y=45+std::sin(i*.38f)*18;g.setColour(amber.withAlpha(.22f));g.drawEllipse(x,y,2.4f,2.4f,.7f);}
 text(g,"ARTISTIC STATE VISUALIZATION / NOT BIOLOGICAL MEASUREMENT",{370,614,580,16},9,cream.withAlpha(.42f));
 text(g,"INPUT",{84,768,55,20},10);text(g,"OUTPUT",{84,793,55,20},10);
 for(int meter=0;meter<2;++meter){float level=meter?processor.outMeter.load():processor.inMeter.load();double db=level>0?20*std::log10(level):-120;int lit=int(std::clamp((db+48)/54.,0.,1.)*34);for(int i=0;i<34;++i){g.setColour(i<lit?(i>30?juce::Colour(0xffff6464):amber):juce::Colour(0xff353332));g.fillRoundedRectangle(145+i*7.f,773+meter*24.f,4,10,1);}text(g,juce::String(db,1)+" dBFS",{340,766+meter*24.f,88,20},10);}
 if(processor.outMeter.load()>1)text(g,"OVER 0 dBFS / NO LIMITER",{835,744,380,15},10,juce::Colour(0xffff6464));
}
void MicroEditor::paint(juce::Graphics& g){g.addTransform(juce::AffineTransform::scale(getWidth()/1320.f,getHeight()/(expanded?1060.f:880.f)));drawFrame(g);}
void MicroEditor::exportLayers(const juce::File& folder){folder.createDirectory();auto save=[&](juce::String name,const juce::Image& im){juce::FileOutputStream stream(folder.getChildFile(name));stream.setPosition(0);stream.truncate();juce::PNGImageFormat png;png.writeImageToStream(im,stream);};
 juce::Image frame(juce::Image::ARGB,getWidth(),getHeight(),true);{juce::Graphics g(frame);g.addTransform(juce::AffineTransform::scale(getWidth()/1320.f,getHeight()/(expanded?1060.f:880.f)));drawFrame(g);}save("01-frame-background.png",frame);
 for(int t=0;t<4;++t){juce::Image im(juce::Image::ARGB,getWidth(),getHeight(),true);juce::Graphics g(im);g.setOrigin(lattice.getPosition());if(t<3){int left=t*lattice.getWidth()/3,right=(t+1)*lattice.getWidth()/3;g.reduceClipRegion(juce::Rectangle<int>(left,0,right-left,lattice.getHeight()));lattice.drawArtwork(g);}else lattice.drawLayer(g,0,true);save(t==3?"05-coupling-strands.png":"0"+juce::String(t+2)+"-lattice-"+juce::String(t+1)+".png",im);}
 juce::Image controls(juce::Image::ARGB,getWidth(),getHeight(),true);{juce::Graphics g(controls);for(auto* child:getChildren())if(child!=&lattice&&child->isVisible()){auto im=child->createComponentSnapshot(child->getLocalBounds(),true,1);g.drawImageAt(im,child->getX(),child->getY());}}save("06-native-controls.png",controls);save("00-interface-preview.png",createComponentSnapshot(getLocalBounds(),true,1));
}
