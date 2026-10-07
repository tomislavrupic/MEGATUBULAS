#include "PitchTracker.h"
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
}
int main(){detectorTests();std::cout<<checks<<" FM checks, "<<failures<<" failures\n";return failures?1:0;}
