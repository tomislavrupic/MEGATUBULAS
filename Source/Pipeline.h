#pragma once
#include "Core.h"
#include "PitchTracker.h"
#include "HarmonicFM.h"
#include <juce_dsp/juce_dsp.h>
namespace micro {
class Pipeline {
 std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
 juce::AudioBuffer<float> wet, dryBuffer;
 std::array<std::array<float,8192>,2> delay{};
 std::array<std::array<Biquad,4>,2> filters{};
 std::array<std::array<Biquad,2>,2> characterFilters{};std::array<double,2> characterDb{};
 std::array<std::array<Biquad,2>,2> preFilters{};double preLow=0,preHigh=0,wetGain=1;uint64_t preClock=0;
 std::array<double,4> eq{};
 double savedMatchOffset=0;double fs=48000,smoothing=0,inGain=1,outGain=1,blend=1,bypass=0,matchGain=1,matchTarget=1;
 int channels=2,capacity=1024,latency=0,write=0,quality=4;
 uint64_t eqClock=0,matchSamples=0;double dryEnergy=0,wetEnergy=0;bool matching=false;
 Parameters p;
 PitchTracker tracker;HarmonicFM fm;std::vector<FMControl> fmTimeline;
 PitchEstimate lastPitch{};FMControl previousFM{};bool tracking=false;
public:
 Engine engine;
 float inputPeak=0,outputPeak=0;double heldMatchDb=0;int matchStatus=0;
 void prepare(double rate,int maximum,int ch,int factor) {
  fs=rate;channels=ch;capacity=std::max(1,maximum);quality=factor;
  oversampler=std::make_unique<juce::dsp::Oversampling<float>>(size_t(ch),factor==8?3:2,juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,true,true);
  oversampler->initProcessing(size_t(capacity));latency=int(std::lround(oversampler->getLatencyInSamples()));
  jassert(latency<int(delay[0].size()));wet.setSize(ch,capacity);dryBuffer.setSize(ch,capacity);smoothing=std::exp(-1/(.02*rate));
  fmTimeline.resize(size_t(capacity));tracker.prepare(rate,ch);fm.prepare(rate*factor);
  engine.prepare(rate*factor);reset();
 }
 void configure(const EntropyConfig& c) noexcept {savedMatchOffset=c.matchOffset;engine.configure(c);reset();}
 void set(const Parameters& value) noexcept {p=value;engine.set(p);fm.set(p.fmAmount,p.fmDepth,std::clamp(p.fmRatio,0,2)+1);}
 void reset() noexcept {
  if(oversampler)oversampler->reset();engine.reset();delay={};write=0;eqClock=0;eq=p.eq;
  tracker.reset();fm.reset();lastPitch={};previousFM={};tracking=false;
  for(auto& ch:filters)for(int b=0;b<4;++b){ch[b].reset();ch[b].c=Biquad::coefficients(b,eq[b],fs);}
  characterDb=characterTone();for(auto& ch:characterFilters)for(int b=0;b<2;++b){ch[b].reset();ch[b].c=Biquad::coefficients(b==0?0:3,characterDb[size_t(b)],fs);}
  inGain=gain(p.input);outGain=gain(p.output);blend=p.blend/100.;bypass=p.bypass?1:0;inputPeak=outputPeak=0;
  preLow=(p.preLow-1)*4.;preHigh=(p.preHigh-1)*4.;wetGain=gain(p.wetLevel);preClock=0;for(auto& ch:preFilters)for(int b=0;b<2;++b){ch[b].reset();ch[b].c=Biquad::coefficients(b==0?0:3,b==0?preLow:preHigh,fs);}
  matchSamples=0;matching=false;matchGain=matchTarget=gain(savedMatchOffset);heldMatchDb=savedMatchOffset;matchStatus=0;
 }
 int getLatency() const noexcept {return latency;}int getQuality() const noexcept {return quality;}
 PitchEstimate pitchEstimate() const noexcept {return lastPitch;}
 double fmGate() const noexcept {return fm.gate();}
 bool fmEnabled() const noexcept {return p.fmAmount>0&&p.fmDepth>0;}
 std::array<double,2> characterTone() const noexcept {
  const auto mode=size_t(std::clamp(p.mode,0,2));const double amount=std::clamp(p.drive/100.,0.,1.);
  return {std::array{2.,1.5,.75}[mode]*amount,std::array{-10.,-8.,-6.}[mode]*amount};
 }
 void startMatch() noexcept {matching=true;matchSamples=0;dryEnergy=wetEnergy=0;matchStatus=1;}
 // Processes arbitrary block sizes via bounded preallocated slices. Never resizes.
 void process(float* const* data,int count) noexcept {
  if(!oversampler)return;
  inputPeak=outputPeak=0;
  for(int offset=0;offset<count;offset+=capacity){int n=std::min(capacity,count-offset);
   const bool wantsFM=fmEnabled();
   if(wantsFM&&!tracking){tracker.reset();fm.reset();lastPitch={};previousFM={};tracking=true;}
   else if(!wantsFM&&tracking){tracker.reset();tracking=false;lastPitch.locked=false;lastPitch.confidence=0;}
   for(int i=0;i<n;++i){preLow=smoothing*preLow+(1-smoothing)*(p.preLow-1)*4.;preHigh=smoothing*preHigh+(1-smoothing)*(p.preHigh-1)*4.;
    if(preClock++%32==0)for(auto& ch:preFilters)for(int b=0;b<2;++b)ch[b].c=Biquad::coefficients(b==0?0:3,b==0?preLow:preHigh,fs);
    inGain=smoothing*inGain+(1-smoothing)*gain(p.input);
    for(int c=0;c<channels;++c){float v=float(finite(data[c][offset+i])*inGain);dryBuffer.setSample(c,i,v);double shaped=v;for(auto& f:preFilters[c])shaped=f.process(shaped);wet.setSample(c,i,float(shaped));inputPeak=std::max(inputPeak,std::abs(v));}
    if(tracking)lastPitch=tracker.process(dryBuffer.getSample(0,i),dryBuffer.getSample(channels==1?0:1,i));
    fmTimeline[size_t(i)]={lastPitch.hz,lastPitch.locked?1.:0.,lastPitch.envelope};
   }
   auto block=juce::dsp::AudioBlock<float>(wet).getSubBlock(0,size_t(n));auto up=oversampler->processSamplesUp(block);
   for(size_t i=0;i<up.getNumSamples();++i){double frame[2]{};for(int c=0;c<channels;++c)frame[c]=up.getSample(size_t(c),i);
    const size_t native=i/size_t(quality);const auto& control=fmTimeline[native];
    if(fm.active()){
     const double fraction=double(i%size_t(quality)+1)/quality;FMControl interpolated=control;
     // Both endpoints are available at this native sample; never use a future timeline entry.
     for(size_t c=0;c<2;++c)interpolated.envelope[c]=previousFM.envelope[c]+fraction*(control.envelope[c]-previousFM.envelope[c]);
     const auto addition=fm.sample(interpolated);for(int c=0;c<channels;++c)frame[c]+=addition[size_t(c)];
    }
    if(i%size_t(quality)==size_t(quality-1))previousFM=control;
    engine.sample(frame,channels);for(int c=0;c<channels;++c)up.setSample(size_t(c),i,float(frame[c]));}
   oversampler->processSamplesDown(block);
   const auto toneTarget=characterTone();
   for(int i=0;i<n;++i){
    for(int b=0;b<4;++b)eq[b]=smoothing*eq[b]+(1-smoothing)*p.eq[b];
    for(size_t b=0;b<2;++b)characterDb[b]=smoothing*characterDb[b]+(1-smoothing)*toneTarget[b];
    if((eqClock++%32)==0){for(int c=0;c<channels;++c)for(int b=0;b<4;++b)filters[c][b].c=Biquad::coefficients(b,eq[b],fs);
     for(int c=0;c<channels;++c)for(int b=0;b<2;++b)characterFilters[c][b].c=Biquad::coefficients(b==0?0:3,characterDb[size_t(b)],fs);
    }
    wetGain=smoothing*wetGain+(1-smoothing)*gain(p.wetLevel);
    matchGain=smoothing*matchGain+(1-smoothing)*matchTarget;
    blend=smoothing*blend+(1-smoothing)*std::clamp(p.blend/100.,0.,1.);outGain=smoothing*outGain+(1-smoothing)*gain(p.output);
    bypass=smoothing*bypass+(1-smoothing)*(p.bypass?1:0);
    for(int c=0;c<channels;++c){
     // Input trim on dry uses the identical per-sample gain stored during the upsampling copy.
     // Store trimmed dry before it is overwritten; its gain is recomputed from a parallel smoothing state below.
     double dry=dryBuffer.getSample(c,i);
     delay[c][write]=float(dry);int read=(write-latency+int(delay[c].size()))%int(delay[c].size());dry=delay[c][read];
     double y=wet.getSample(c,i);for(auto& f:characterFilters[c])y=f.process(y);for(auto& f:filters[c])y=f.process(y);y=std::isfinite(y)?y:0;
     if(matching){dryEnergy+=dry*dry;wetEnergy+=y*y;}
     double result=((1-blend)*dry+blend*y*matchGain*wetGain)*outGain;
     data[c][offset+i]=float(std::isfinite(result)?(1-bypass)*result+bypass*dry:0);outputPeak=std::max(outputPeak,std::abs(data[c][offset+i]));
    }
    write=(write+1)%int(delay[0].size());
    if(matching && ++matchSamples>=uint64_t(2*fs)){
     if(dryEnergy/(double(matchSamples)*channels)>1e-8 && wetEnergy>1e-12){heldMatchDb=std::clamp(10*std::log10(dryEnergy/wetEnergy),-12.,12.);savedMatchOffset=heldMatchDb;matchTarget=gain(heldMatchDb);matchStatus=2;}
     else matchStatus=3;matching=false;
    }
   }
  }
 }
};
}
