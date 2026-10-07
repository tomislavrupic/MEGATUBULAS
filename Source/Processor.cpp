#include "Processor.h"
#include "Editor.h"
namespace {
constexpr const char* ids[]={"drive","memory","coupling","blend","output","input","low","lowMid","highMid","high","variation","mode","freeze","bypass","quality","ablate","motion","wetLevel","preLow","preHigh","fmAmount","fmDepth","fmRatio"};
}
juce::AudioProcessorValueTreeState::ParameterLayout MicroProcessor::layout(){
 juce::AudioProcessorValueTreeState::ParameterLayout l;
 auto add=[&](const char* id,const char* name,float lo,float hi,float def,const char* unit){
  l.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},name,juce::NormalisableRange<float>(lo,hi,.01f),def,juce::AudioParameterFloatAttributes().withLabel(unit)));
 };
 add("drive","Drive",0,100,30,"%");add("memory","Memory",0,100,40,"%");add("coupling","Coupling",0,100,30,"%");add("blend","Blend",0,100,100,"%");add("output","Output",-24,12,0,"dB");add("input","Input trim",-24,12,0,"dB");
 add("low","Low 100 Hz",-12,12,0,"dB");add("lowMid","Low mid 500 Hz",-12,12,0,"dB");add("highMid","High mid 1.5 kHz",-12,12,0,"dB");add("high","High 5 kHz",-12,12,0,"dB");add("variation","Variation",0,100,0,"%");
 l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"mode",1},"Character",juce::StringArray{"Warm","Tense","Open"},0));
 l.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"freeze",1},"Freeze selection",false));
 l.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"bypass",1},"Bypass",false));
 l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"quality",1},"Oversampling",juce::StringArray{"4x","8x"},0));
 l.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"ablate",1},"Disable memory and coupling",false));
 l.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"motion",1},"Animate lattice",true));
 add("wetLevel","Wet level",-24,12,0,"dB");
 l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"preLow",1},"Bass into saturation",juce::StringArray{"Tight (-4 dB)","Flat","Full (+4 dB)"},1));
 l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"preHigh",1},"Treble into saturation",juce::StringArray{"Soft (-4 dB)","Flat","Bright (+4 dB)"},1));
 add("fmAmount","Experimental FM amount",0,100,0,"%");add("fmDepth","Experimental FM depth",0,100,25,"%");
 l.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"fmRatio",1},"Experimental FM ratio",juce::StringArray{"1x","2x","3x"},0));
 // Source is configuration, deliberately not an automatable parameter; reserved ID omitted.
 return l;
}
MicroProcessor::MicroProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),state(*this,nullptr,"MICROTUBULAS",layout()){
 for(size_t i=0;i<values.size();++i)values[i]=state.getRawParameterValue(ids[i]);
 auto initial=micro::candidate(0,3);std::array<double,6> vector{initial.drive,initial.asym,initial.bias,initial.recovery,initial.emphasis,initial.coupling};for(size_t i=0;i<6;++i)selectedVector[i]=float(vector[i]);
 std::strncpy(savedConfig.source.data(),"Seeded xorshift32 (not quantum)",127);
}
micro::Parameters MicroProcessor::parameters() const {
 micro::Parameters p;p.drive=values[0]->load();p.memory=values[1]->load();p.coupling=values[2]->load();p.blend=values[3]->load();p.output=values[4]->load();p.input=values[5]->load();for(int b=0;b<4;++b)p.eq[b]=values[size_t(b+6)]->load();
 p.variation=values[10]->load();p.mode=int(values[11]->load());p.freeze=values[12]->load()>.5;p.bypass=values[13]->load()>.5;p.ablate=values[15]->load()>.5;p.wetLevel=values[17]->load();p.preLow=int(values[18]->load());p.preHigh=int(values[19]->load());p.fmAmount=values[20]->load();p.fmDepth=values[21]->load();p.fmRatio=int(values[22]->load());return p;
}
bool MicroProcessor::isBusesLayoutSupported(const BusesLayout& l) const {auto out=l.getMainOutputChannelSet();return (out==juce::AudioChannelSet::mono()||out==juce::AudioChannelSet::stereo())&&out==l.getMainInputChannelSet();}
void MicroProcessor::prepareToPlay(double fs,int block){pipeline.set(parameters());pipeline.prepare(fs,std::clamp(block,1,4096),getTotalNumOutputChannels(),values[14]->load()>.5?8:4);pipeline.configure(entropyConfig());activeQuality=pipeline.getQuality();setLatencySamples(pipeline.getLatency());wasOffline=isNonRealtime();}
bool MicroProcessor::enqueue(const micro::EntropyConfig& c){auto w=producer.load(std::memory_order_relaxed);if(w-consumer.load(std::memory_order_acquire)>=queue.size())return false;auto& slot=queue[w%queue.size()];slot.config=c;producer.store(w+1,std::memory_order_release);return true;}
bool MicroProcessor::setEntropy(const micro::EntropyConfig& c){if(!c.valid())return false;std::lock_guard lock(configMutex);if(!enqueue(c))return false;savedConfig=c;return true;}
micro::EntropyConfig MicroProcessor::entropyConfig() const {std::lock_guard lock(configMutex);auto c=savedConfig;c.matchOffset=matchDb.load();c.frozenIndex=selection.load();c.hasFrozen=values[12]->load()>.5;for(size_t i=0;i<6;++i)c.frozenVoice[i]=selectedVector[i].load();return c;}
void MicroProcessor::consume() noexcept {auto r=consumer.load(std::memory_order_relaxed),w=producer.load(std::memory_order_acquire);for(unsigned i=0;i<4&&r<w;++i,++r)pipeline.configure(queue[r%queue.size()].config);consumer.store(r,std::memory_order_release);}
void MicroProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer& midi){
 juce::ScopedNoDenormals noDenormals;midi.clear();consume();pipeline.set(parameters());
 bool offline=isNonRealtime();if(offline&&!wasOffline)pipeline.reset();wasOffline=offline;
 if(resetRequested.exchange(false))pipeline.reset();if(exploreRequested.exchange(false))pipeline.engine.explore();if(matchRequested.exchange(false))pipeline.startMatch();
 for(int c=getTotalNumInputChannels();c<b.getNumChannels();++c)b.clear(c,0,b.getNumSamples());
 if(b.getNumSamples()>0)pipeline.process(b.getArrayOfWritePointers(),b.getNumSamples());
 auto vector=pipeline.engine.selectedVoice();for(size_t i=0;i<6;++i)selectedVector[i]=float(vector[i]);
 inMeter=pipeline.inputPeak;outMeter=pipeline.outputPeak;memoryMeter=float(pipeline.engine.memoryState(1));matchDb=float(pipeline.heldMatchDb);selection=pipeline.engine.index();eventCount=int(pipeline.engine.events);entropyPosition=int(pipeline.engine.consumed());entropyExhausted=pipeline.engine.isExhausted()?1:0;matchState=pipeline.matchStatus;
 auto pitch=pipeline.pitchEstimate();fmHz=float(pitch.hz);fmConfidence=float(pitch.confidence);fmGate=float(pipeline.fmGate());fmTrackingState=pitch.locked?2:pipeline.fmGate()>1.e-6?3:pipeline.fmEnabled()?1:0;
}
const juce::String MicroProcessor::getProgramName(int i){return juce::StringArray{"Bass Foundation","Synth Amber","Drum Tension","Gentle Mix Colour","Fractal Motion"}[std::clamp(i,0,4)];}
void MicroProcessor::setCurrentProgram(int i){i=std::clamp(i,0,4);program=i;const float presets[5][12]={{35,65,25,80,0,0,1,0,0,-1,0,0},{50,70,40,100,0,0,0,1,0,0,15,0},{60,25,65,65,0,0,0,-1,1,0,5,1},{15,35,15,45,0,0,0,0,0,0,0,2},{45,85,70,85,0,0,0,0,0,0,75,1}};
 for(int j=0;j<12;++j){auto* param=state.getParameter(ids[j]);param->beginChangeGesture();param->setValueNotifyingHost(param->convertTo0to1(presets[i][j]));param->endChangeGesture();}
 for(int j=20;j<23;++j){auto* param=state.getParameter(ids[j]);param->beginChangeGesture();param->setValueNotifyingHost(param->getDefaultValue());param->endChangeGesture();}resetRequested=true;
}
bool MicroProcessor::importEntropy(const juce::String& text,juce::String& error){
 if(text.getNumBytesAsUTF8()>65536){error="Import exceeds 64 KiB";return false;}
 juce::var json;auto result=juce::JSON::parse(text,json);if(result.failed()||!json.isObject()){error="Expected a JSON object";return false;}
 auto* a=json["values"].getArray();auto kind=json["kind"].toString();
 if((kind!="quantum-derived"&&kind!="saved-sequence")||!a||a->isEmpty()||a->size()>1024){error="Expected kind quantum-derived/saved-sequence and 1–1024 uint32 values";return false;}
 micro::EntropyConfig c;c.kind=kind=="quantum-derived"?micro::EntropyConfig::imported:micro::EntropyConfig::replay;c.count=uint32_t(a->size());
 for(int i=0;i<a->size();++i){const auto& v=(*a)[i];double n=double(v);if((!v.isInt()&&!v.isInt64()&&!v.isDouble())||!std::isfinite(n)||n<0||n>4294967295.||n!=std::floor(n)){error="Invalid uint32 value";return false;}c.values[size_t(i)]=uint32_t(n);}
 for(const char* key:{"source","sequenceId","importedAt"})if(json[key].toString().isEmpty()||json[key].toString().getNumBytesAsUTF8()>127){error="source, sequenceId and importedAt required, maximum 127 UTF-8 bytes each";return false;}
 json["source"].toString().copyToUTF8(c.source.data(),128);json["sequenceId"].toString().copyToUTF8(c.identifier.data(),128);json["importedAt"].toString().copyToUTF8(c.importedAt.data(),128);
 if(json.hasProperty("start")){auto start=json["start"];double v=double(start);if((!start.isInt()&&!start.isInt64())||v<0||v>=c.count){error="start must be an integer within the sequence";return false;}c.start=uint32_t(v);}
 if(!setEntropy(c)){error="Configuration queue busy; retry after processing resumes";return false;}return true;
}
void MicroProcessor::getStateInformation(juce::MemoryBlock& dest){auto tree=state.copyState();tree.setProperty("schema",1,nullptr);tree.setProperty("program",program.load(),nullptr);auto c=entropyConfig();juce::ValueTree e("Entropy");e.setProperty("kind",c.kind,nullptr);e.setProperty("seed",juce::String(c.seed),nullptr);e.setProperty("start",int(c.start),nullptr);e.setProperty("frozenIndex",c.frozenIndex,nullptr);e.setProperty("source",c.source.data(),nullptr);e.setProperty("sequenceId",c.identifier.data(),nullptr);e.setProperty("importedAt",c.importedAt.data(),nullptr);
 juce::String sequence;for(uint32_t i=0;i<c.count;++i)sequence+=juce::String(c.values[i])+" ";e.setProperty("values",sequence,nullptr);e.setProperty("matchOffset",c.matchOffset,nullptr);e.setProperty("hasFrozen",c.hasFrozen,nullptr);for(size_t i=0;i<6;++i)e.setProperty("voice"+juce::String(i),c.frozenVoice[i],nullptr);tree.addChild(e,-1,nullptr);auto xml=tree.createXml();copyXmlToBinary(*xml,dest);
}
void MicroProcessor::setStateInformation(const void* data,int size){
 if(size<=0||size>131072){++loadErrors;return;}auto xml=getXmlFromBinary(data,size);if(!xml){++loadErrors;return;}auto tree=juce::ValueTree::fromXml(*xml);
 if(!tree.hasType("MICROTUBULAS")||int(tree["schema"])!=1){++loadErrors;return;}
 for(int i=0;i<tree.getNumChildren();++i){auto ch=tree.getChild(i);if(ch.hasType("PARAM")){auto* param=state.getParameter(ch["id"].toString());double v=double(ch["value"]);if(!param||!std::isfinite(v)){++loadErrors;return;}auto range=param->getNormalisableRange();ch.setProperty("value",juce::jlimit(double(range.start),double(range.end),v),nullptr);}}
 auto e=tree.getChildWithName("Entropy");if(!e.isValid()){++loadErrors;return;}micro::EntropyConfig c;c.kind=int(e["kind"]);auto seed=e["seed"].toString().getLargeIntValue();if(seed<0||seed>4294967295LL){++loadErrors;return;}c.seed=uint32_t(seed);c.start=uint32_t(int(e["start"]));c.frozenIndex=int(e["frozenIndex"]);
 c.matchOffset=double(e["matchOffset"]);c.hasFrozen=bool(e["hasFrozen"]);for(size_t i=0;i<6;++i)c.frozenVoice[i]=double(e["voice"+juce::String(i)]);
 auto tokens=juce::StringArray::fromTokens(e["values"].toString().trim(),false);if(tokens.size()>1024){++loadErrors;return;}for(auto& t:tokens){auto v=t.getLargeIntValue();if(v<0||v>4294967295LL||!t.containsOnly("0123456789")){++loadErrors;return;}c.values[c.count++]=uint32_t(v);}
 for(auto pair:{std::pair{"source",c.source.data()},std::pair{"sequenceId",c.identifier.data()},std::pair{"importedAt",c.importedAt.data()}}){auto t=e[pair.first].toString();if(t.getNumBytesAsUTF8()>127){++loadErrors;return;}t.copyToUTF8(pair.second,128);}
 if(!c.valid()||!setEntropy(c)){++loadErrors;return;}for(int j=20;j<23;++j){bool found=false;for(int i=0;i<tree.getNumChildren();++i)if(tree.getChild(i).hasType("PARAM")&&tree.getChild(i)["id"].toString()==ids[j])found=true;if(!found){juce::ValueTree node("PARAM");node.setProperty("id",ids[j],nullptr);auto* param=state.getParameter(ids[j]);node.setProperty("value",param->convertFrom0to1(param->getDefaultValue()),nullptr);tree.addChild(node,-1,nullptr);}}
 state.replaceState(tree);program=std::clamp(int(tree["program"]),0,4);resetRequested=true;
}
juce::AudioProcessorEditor* MicroProcessor::createEditor(){return new MicroEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new MicroProcessor;}
