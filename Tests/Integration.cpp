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
