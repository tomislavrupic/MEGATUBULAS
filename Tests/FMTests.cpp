#include "PitchTracker.h"
#include "HarmonicFM.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <limits>
namespace {
int checks=0,failures=0;
void check(bool v,const char* label){++checks;if(!v){++failures;std::cerr<<"FAIL: "<<label<<'\n';}}
double cents(double actual,double wanted){return actual>0?std::abs(1200*std::log2(actual/wanted)):1e9;}
double bass(double phase,int kind){return kind==0?std::sin(phase):.06*std::sin(phase)+.25*std::sin(2*phase)+.16*std::sin(3*phase)+.08*std::sin(4*phase);}
void detectorTests(){
 for(double fs:{44100.,48000.,96000.})for(double hz:{25.,30.8677,41.2034,55.,82.4069,110.,220.,400.})for(int kind:{0,1}){
  micro::PitchTracker t;t.prepare(fs,2);micro::PitchEstimate e{};int locked=0,correct=0;double first=-1;
  for(int i=0;i<int(fs*.5);++i){double x=.6*bass(2*micro::pi*hz*i/fs,kind);e=t.process(x,-x);if(e.locked&&first<0)first=i/fs;if(i>int(fs*.25)){locked+=e.locked;correct+=e.locked&&cents(e.hz,hz)<20;}}
  std::cout<<"pitch,"<<fs<<','<<hz<<','<<kind<<','<<first<<','<<cents(e.hz,hz)<<','<<correct<<'\n';
  if(correct<int(fs*.20))std::cerr<<"pitch fixture fs="<<fs<<" hz="<<hz<<" kind="<<kind<<" got="<<e.hz<<" locked="<<locked<<" first="<<first<<'\n';
  check(correct>int(fs*.20),"periodic/weak fundamental/anti-phase bass tracks within 20 cents");check(first>=0&&first<.15,"periodic note acquisition under 150 ms");
 }
 {micro::PitchTracker t;t.prepare(48000,1);int falseLocks=0;uint32_t seed=12345;
  for(int i=0;i<48000;++i){seed=1664525*seed+1013904223;double n=(double(seed)/4294967296.-.5)*.4;falseLocks+=t.process(n,n).locked;}
  check(falseLocks==0,"white noise does not assert a stable pitch");
  for(int i=0;i<48000;++i)t.process(0,0);auto e=t.process(std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity());
  check(!e.locked&&std::isfinite(e.hz)&&e.envelope[0]<1e-4,"silence and invalid samples release tracking");
 }
 {micro::PitchTracker t;t.prepare(48000,2);micro::PitchEstimate e{};double phase=0;int bad=0,good=0;
  for(int i=0;i<72000;++i){double hz=i<24000?55.:i<48000?110.:110.+20.*(i-48000)/24000.;phase+=2*micro::pi*hz/48000;double x=.3*bass(phase,1);e=t.process(i<36000?x:x*.1,i<36000?x*.1:-x);if(i>30000&&i<35000){bad+=e.locked&&cents(e.hz,110)>20;good+=e.locked;}if(i>60000)bad+=e.locked&&cents(e.hz,hz)>80;}
  check(good>4000&&bad==0,"octave change, channel switch and bend avoid wrong stable locks");check(e.locked&&cents(e.hz,130)<80,"continuous bend is followed without semitone quantization");
  t.reset();check(!t.process(0,0).locked,"reset clears note lock");
 }
}
double component(const std::vector<double>& v,double hz,double fs){double a=0,b=0;for(size_t i=0;i<v.size();++i){double p=2*micro::pi*hz*i/fs;a+=v[i]*std::cos(p);b+=v[i]*std::sin(p);}return 2*std::hypot(a,b)/v.size();}
void voiceTests(){
 const double fs=192000;micro::FMControl c{55,1,{.5,.5}};
 for(int ratio:{1,2,3}){micro::HarmonicFM voice;voice.prepare(fs);voice.set(80,70,ratio);std::vector<double> samples;double peak=0,mean=0;
  for(int i=0;i<int(fs*1.5);++i){auto y=voice.sample(c);peak=std::max(peak,std::abs(y[0]));if(!std::isfinite(y[0])||y[0]!=y[1]){check(false,"FM finite linked stereo output");break;}if(i>=int(fs*.5)){samples.push_back(y[0]);mean+=y[0];}}
  double harmonic=0;for(int n=2;n<=9;++n)harmonic+=std::pow(component(samples,55*n,fs),2);
  check(harmonic>1e-4,"FM adds harmonics at integer multiples");check(component(samples,137,fs)<.001,"FM does not add stationary inharmonic tone");check(peak<.4&&std::abs(mean/samples.size())<1e-4,"FM contribution bounded and DC rejected");
  voice.reset();voice.set(80,70,ratio);std::vector<double> a;for(int i=0;i<2000;++i)a.push_back(voice.sample(c)[0]);voice.reset();voice.set(80,70,ratio);bool same=true;for(double x:a)same&=x==voice.sample(c)[0];check(same,"FM reset deterministically replays");
  voice.set(80,70,ratio==3?1:ratio+1);double previous=0,maxStep=0;for(int i=0;i<int(fs*.2);++i){double y=voice.sample(c)[0];if(i)maxStep=std::max(maxStep,std::abs(y-previous));previous=y;}check(maxStep<.02,"ratio transition remains click free on 55 Hz fixture");
  c.gate=0;double tail=0;for(int i=0;i<int(fs*1.5);++i){double y=voice.sample(c)[0];if(i>int(fs))tail=std::max(tail,std::abs(y));}check(tail<1e-7,"lost lock fades oscillator and filter tails to silence");c.gate=1;
 }
 {micro::HarmonicFM a,b;a.prepare(192000);b.prepare(1536000);a.set(100,100,3);b.set(100,100,3);micro::FMControl ctl{400,1,{.5,.5}};double e=0,energy=0;int n=0;for(int i=0;i<96000;++i){double x=a.sample(ctl)[0],y=0;for(int j=0;j<8;++j)y=b.sample(ctl)[0];if(i>48000){e+=(x-y)*(x-y);energy+=y*y;++n;}}std::cout<<"FM max-pitch/depth high-rate reference residual "<<10*std::log10(e/energy)<<" dB relative\n";check(std::sqrt(e/n)<.003,"maximum FM settings remain close to higher-rate reference");}
 for(int kind:{0,1,2}){micro::HarmonicFM v;v.prepare(fs);v.set(kind==0?0:80,kind==1?0:70,1);auto ctl=c;if(kind==2)ctl.envelope={0,0};double peak=0;for(int i=0;i<10000;++i)peak=std::max(peak,std::abs(v.sample(ctl)[0]));check(peak==0,"Amount zero, Depth zero and silent input generate no sine carrier");}
}
}
int main(){detectorTests();voiceTests();std::cout<<checks<<" FM checks, "<<failures<<" failures\n";return failures?1:0;}
