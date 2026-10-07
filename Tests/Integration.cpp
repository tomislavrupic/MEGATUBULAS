#include "../Source/Processor.h"
#include <iostream>
#include <thread>
#include <cstdlib>
thread_local bool audioGuard=false;std::atomic<int> allocationCount{0};
void* operator new(std::size_t n){if(audioGuard)++allocationCount;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}void operator delete(void* p) noexcept{std::free(p);}void operator delete[](void* p) noexcept{std::free(p);}
class Integration:public juce::JUCEApplication {
 int failed=0,checks=0;
 void check(bool ok,const char* label){++checks;if(!ok){std::cerr<<"FAIL "<<label<<"\n";++failed;}}
public:
 const juce::String getApplicationName() override{return "MicroIntegration";}const juce::String getApplicationVersion() override{return "0.1";}
 void initialise(const juce::String&) override {
  MicroProcessor a,b;check(a.getName()=="MEGATUBULAS","renamed product identity");a.prepareToPlay(48000,127);b.prepareToPlay(48000,127);juce::AudioBuffer<float> buffer(2,1024);juce::MidiBuffer midi;
  auto process=[&](MicroProcessor& p){for(int c=0;c<2;++c)for(int i=0;i<buffer.getNumSamples();++i)buffer.setSample(c,i,float(.4*std::sin(i*.1)));audioGuard=true;p.processBlock(buffer,midi);audioGuard=false;};
  bool hasFM=true;for(auto id:{"fmAmount","fmDepth","fmRatio"}){bool exists=a.state.getParameter(id)!=nullptr;check(exists,"experimental FM host parameter exists");hasFM&=exists;}
  if(hasFM){
   auto set=[](MicroProcessor& p,const char* id,float value){auto* parameter=p.state.getParameter(id);parameter->setValueNotifyingHost(parameter->convertTo0to1(value));};
   check(a.state.getRawParameterValue("fmAmount")->load()==0,"FM defaults off");
   check(a.state.getRawParameterValue("fmDepth")->load()==66&&a.state.getParameter("fmDepth")->convertFrom0to1(a.state.getParameter("fmDepth")->getDefaultValue())==66,"FM Depth defaults and double-click reset to 66 percent");
   set(a,"fmAmount",80);set(a,"fmDepth",65);set(a,"fmRatio",2);process(a);juce::MemoryBlock fmState;a.getStateInformation(fmState);b.setStateInformation(fmState.getData(),int(fmState.getSize()));process(b);
   check(b.state.getRawParameterValue("fmAmount")->load()==80&&b.state.getRawParameterValue("fmDepth")->load()==65&&b.state.getRawParameterValue("fmRatio")->load()==2,"FM values survive session round trip");
   auto xml=juce::AudioProcessor::getXmlFromBinary(fmState.getData(),int(fmState.getSize()));auto old=juce::ValueTree::fromXml(*xml);
   for(int i=old.getNumChildren()-1;i>=0;--i)if(old.getChild(i)["id"].toString().startsWith("fm"))old.removeChild(i,nullptr);
   juce::MemoryBlock oldState;juce::AudioProcessor::copyXmlToBinary(*old.createXml(),oldState);b.setStateInformation(oldState.getData(),int(oldState.getSize()));process(b);
   check(b.state.getRawParameterValue("fmAmount")->load()==0&&b.state.getRawParameterValue("fmDepth")->load()==66&&b.state.getRawParameterValue("fmRatio")->load()==0,"loading legacy state clears previously enabled FM and restores defaults");
   for(int preset=0;preset<5;++preset){set(a,"fmAmount",100);set(a,"fmDepth",100);set(a,"fmRatio",2);a.setCurrentProgram(preset);process(a);check(a.state.getRawParameterValue("fmAmount")->load()==0&&a.state.getRawParameterValue("fmDepth")->load()==66&&a.state.getRawParameterValue("fmRatio")->load()==0,"existing presets explicitly reset all FM parameters");}
   for(int n=0;n<30;++n){set(a,"fmAmount",n%2?100:40);set(a,"fmRatio",float(n%3));set(a,"fmDepth",float((n*17)%101));process(a);}check(allocationCount==0,"FM automation causes no guarded callback allocation");
   // Lifecycle replay with a continuous single-note fixture; no device is opened.
   set(a,"fmAmount",90);set(a,"fmDepth",70);set(a,"fmRatio",1);set(a,"freeze",1);
   auto replay=[&](bool prepare,bool offline){if(prepare)a.prepareToPlay(48000,127);else a.reset();a.setNonRealtime(offline);std::vector<float> result;result.reserve(48000);int initial=-1;
    for(int block=0;block<400;++block){buffer.setSize(2,127,false,false,true);double hz=block<200?55:82.4069;for(int i=0;i<127;++i){float v=float(.4*std::sin(2*micro::pi*hz*(block*127+i)/48000));buffer.setSample(0,i,v);buffer.setSample(1,i,-v);}audioGuard=true;a.processBlock(buffer,midi);audioGuard=false;for(int i=0;i<127;++i)result.push_back(buffer.getSample(0,i));if(block==100)initial=a.selection;}
    check(a.fmTrackingState==2&&std::abs(a.fmHz.load()-82.4069)<1,"Freeze retains pitch following after note change");check(a.selection==initial,"Freeze retains selector while FM follows");return result;};
   auto first=replay(true,false),reset=replay(false,false),prepared=replay(true,false),offline=replay(false,true);
   check(first==reset&&first==prepared&&first==offline,"enabled FM reset, reprepare and offline entry reproduce audio");
   set(a,"fmAmount",0);set(a,"freeze",0);a.setNonRealtime(false);buffer.setSize(2,1024,false,false,true);
   auto bad=juce::ValueTree::fromXml(*xml);for(int i=0;i<bad.getNumChildren();++i)if(bad.getChild(i)["id"].toString()=="fmDepth")bad.getChild(i).setProperty("value",std::numeric_limits<double>::infinity(),nullptr);
   juce::MemoryBlock invalid;juce::AudioProcessor::copyXmlToBinary(*bad.createXml(),invalid);int errors=b.loadErrors;b.setStateInformation(invalid.getData(),int(invalid.getSize()));check(b.loadErrors==errors+1,"non-finite FM state rejected");
  }
  for(int preset=0;preset<5;++preset){a.setCurrentProgram(preset);process(a);juce::MemoryBlock saved;a.getStateInformation(saved);b.setStateInformation(saved.getData(),int(saved.getSize()));process(b);for(auto* parameter:a.getParameters())if(auto* p=dynamic_cast<juce::RangedAudioParameter*>(parameter))check(std::abs(p->getValue()-b.state.getParameter(p->paramID)->getValue())<1e-6,"preset parameter round trip");check(b.getCurrentProgram()==preset,"preset index round trip");}
  juce::String error;check(!a.importEntropy("{\"kind\":\"quantum-derived\",\"values\":[-1]}",error),"negative import rejected");check(!a.importEntropy("{\"kind\":\"quantum-derived\",\"values\":[1.2]}",error),"fractional import rejected");check(!a.importEntropy("[]",error),"non-object rejected");
  check(a.importEntropy("{\"kind\":\"saved-sequence\",\"values\":[0,4294967295,123],\"source\":\"test fixture\",\"sequenceId\":\"fixture-1\",\"importedAt\":\"2026-10-07\"}",error),"valid sequence accepted");process(a);juce::MemoryBlock saved;a.getStateInformation(saved);b.setStateInformation(saved.getData(),int(saved.getSize()));process(b);auto c=b.entropyConfig();check(c.count==3&&c.values[1]==4294967295u&&c.kind==micro::EntropyConfig::replay,"sequence configuration round trip");
  int before=b.loadErrors.load();b.setStateInformation("garbage",7);check(b.loadErrors.load()==before+1,"malformed state rejected");
  a.state.getParameter("quality")->setValueNotifyingHost(1);int latency=a.getLatencySamples();process(a);check(a.activeQuality==4&&a.getLatencySamples()==latency,"quality deferred while processing");a.prepareToPlay(48000,127);check(a.activeQuality==8,"quality applies during prepare");
  a.matchRequested=true;for(int i=0;i<100;++i)process(a);check(a.matchState==2,"RMS match completes and holds");float db=a.matchDb;for(int i=0;i<5;++i)process(a);check(a.matchDb==db,"match gain stays held");
  juce::AudioBuffer<float> zero(2,0);audioGuard=true;a.processBlock(zero,midi);audioGuard=false;check(allocationCount==0,"no C++ heap allocation in guarded callbacks, including state consumption");
  // Load configuration on a different thread while processing, with bounded queue/rejection behaviour.
  std::atomic<bool> done=false;std::thread writer([&]{for(int i=0;i<100;++i)b.setStateInformation(saved.getData(),int(saved.getSize()));done=true;});while(!done)process(b);writer.join();check(std::isfinite(buffer.getSample(0,0)),"active state loading remains finite");
  std::cout<<checks<<" integration checks, "<<failed<<" failures, callback allocations "<<allocationCount<<"\n";setApplicationReturnValue(failed?1:0);quit();
 }
 void shutdown() override{}
};START_JUCE_APPLICATION(Integration)
