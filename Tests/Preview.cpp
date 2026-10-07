#include "../Source/Editor.h"
#include <fstream>
#include <iostream>
class PreviewApp:public juce::JUCEApplication,private juce::Timer {
 std::unique_ptr<MicroProcessor> processor;std::unique_ptr<MicroEditor> editor;
 juce::File folder;bool audit=false;int frame=0;double phase=0;std::vector<double> brightness;
 void signal(float amplitude,int count){juce::AudioBuffer<float> b(2,count);juce::MidiBuffer midi;for(int i=0;i<count;++i){float v=amplitude*float(std::sin(phase));phase+=2*micro::pi*110/48000;b.setSample(0,i,v);b.setSample(1,i,v);}processor->processBlock(b,midi);}
 void saveImage(const juce::Image& im,const juce::String& name){juce::FileOutputStream stream(folder.getChildFile(name));stream.setPosition(0);stream.truncate();juce::PNGImageFormat png;png.writeImageToStream(im,stream);}
 void timerCallback() override {
  signal(frame>=30&&frame<120?.5f:0.f,1600);
  auto im=editor->captureLattice();double sum=0;int count=0;
  for(int y=0;y<im.getHeight();y+=6)for(int x=0;x<im.getWidth();x+=6){auto c=im.getPixelAt(x,y);sum+=c.getFloatAlpha()*(.2126*c.getFloatRed()+.7152*c.getFloatGreen()+.0722*c.getFloatBlue());++count;}
  brightness.push_back(sum/count);if(frame==15||frame==90||frame==165)saveImage(im,"motion-"+juce::String(frame)+".png");
  if(++frame>=180){stopTimer();double jump=0;for(size_t i=70;i<115;++i)jump=std::max(jump,std::abs(brightness[i]-brightness[i-1])/std::max(brightness[i-1],1.e-12));
   double quiet=0,loud=0;for(size_t i=10;i<25;++i)quiet+=brightness[i];for(size_t i=80;i<95;++i)loud+=brightness[i];
   std::ofstream out(folder.getChildFile("motion-photometry.csv").getFullPathName().toStdString());out<<"frame,mean_premultiplied_luminance\n";for(size_t i=0;i<brightness.size();++i)out<<i<<','<<brightness[i]<<'\n';
   const bool passed=jump<.035&&loud>quiet*1.15;std::cout<<"Native motion audit: maximum adjacent-frame brightness jump in constant sound="<<jump<<", loud/quiet ratio="<<loud/quiet<<", result="<<(passed?"PASS":"FAIL")<<"\n";setApplicationReturnValue(passed?0:1);quit();
  }
 }
public:
 const juce::String getApplicationName() override{return "MicroPreview";}const juce::String getApplicationVersion() override{return "0.2";}
 void initialise(const juce::String& args) override {
  audit=args.startsWith("--audit ");const auto path=(audit?args.substring(8):args).unquoted();folder=path.isEmpty()?juce::File::getCurrentWorkingDirectory().getChildFile("Artwork/Layers"):juce::File(path);folder.createDirectory();
  processor=std::make_unique<MicroProcessor>();processor->prepareToPlay(48000,127);if(!audit)for(int n=0;n<500;++n)signal(.5f,127);
  editor=std::make_unique<MicroEditor>(*processor);
  if(audit){brightness.reserve(180);startTimerHz(30);}else juce::Timer::callAfterDelay(500,[this]{editor->exportLayers(folder);quit();});
 }
 void shutdown() override{stopTimer();editor.reset();processor.reset();}
};START_JUCE_APPLICATION(PreviewApp)
