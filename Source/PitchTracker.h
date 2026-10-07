#pragma once
#include "Core.h"
namespace micro {
struct PitchEstimate {double hz=0,confidence=0;std::array<double,2> envelope{};bool locked=false;};
// Fixed storage, causal mono-note analysis. The audio path never passes through these filters.
class PitchTracker {
 struct Lowpass {
  double b0=0,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
  void prepare(double fs,double q) noexcept {double w=2*pi*800/fs,c=std::cos(w),a=std::sin(w)/(2*q),d=1+a;b0=(1-c)*.5/d;b1=(1-c)/d;b2=b0;a1=-2*c/d;a2=(1-a)/d;z1=z2=0;}
  double sample(double x) noexcept {double y=b0*x+z1;z1=b1*x-a1*y+z2;z2=b2*x-a2*y;return y;}
 };
 std::array<std::array<Lowpass,3>,2> filters{};
 std::array<std::array<double,512>,2> ring{};
 std::array<double,258> difference{};
 std::array<double,512> analysisWindow{};double differenceSum=0;int jobLag=0,minLag=15,maxLag=240,lagsPerSample=1;
 std::array<double,2> energy{};
 PitchEstimate estimate{};
 double rate=48000,analysisRate=6000,attack=0,release=0,energyA=0,candidateHz=0;
 int decimation=8,decimationClock=0,write=0,filled=0,hopClock=0,hop=48,selected=0,switchClock=0,streak=0,channels=2;
 bool selectedInitially=false;
 void analyze() noexcept {
  if(!selectedInitially){selected=energy[1]>energy[0]?1:0;selectedInitially=true;}
  int lag=0;const double threshold=estimate.locked?.25:.15;
  for(int t=minLag;t<=maxLag;++t)if(difference[size_t(t)]<threshold){while(t<maxLag&&difference[size_t(t+1)]<difference[size_t(t)])++t;lag=t;break;}
  estimate.confidence=lag?std::clamp(1-difference[size_t(lag)],0.,1.):0;
  if(!lag||energy[size_t(selected)]<1e-6){estimate.locked=false;streak=0;return;}
  const double a=difference[size_t(lag-1)],b=difference[size_t(lag)],c=difference[size_t(lag+1)],den=a-2*b+c;
  double refined=lag+(std::abs(den)>1e-12?std::clamp(.5*(a-c)/den,-.5,.5):0.);
  double hz=analysisRate/refined;
  // Allow interpolation at the range edges, then clamp display/control to the supported range.
  if(hz<24.5||hz>410){estimate.locked=false;streak=0;return;}hz=std::clamp(hz,25.,400.);
  if(candidateHz>0&&std::abs(1200*std::log2(hz/candidateHz))<=50)++streak;else{streak=1;estimate.locked=false;}
  candidateHz=hz;
  if(streak>=3){estimate.hz=hz;estimate.locked=true;}
 }
 // Snapshot each 8 ms; spread the fixed difference job across native samples.
 // No worker thread, lookahead or host-block clock is involved.
 void beginAnalysis() noexcept {
  for(int j=0;j<512;++j)analysisWindow[size_t(j)]=ring[size_t(selected)][size_t((write+j)&511)];
  difference[0]=1;differenceSum=0;jobLag=1;
 }
 void advanceAnalysis() noexcept {
  for(int work=0;work<lagsPerSample&&jobLag>0;++work){int tau=jobLag;double d=0;
   for(int j=0;j<256;++j){double v=analysisWindow[size_t(j)]-analysisWindow[size_t(j+tau)];d+=v*v;}
   differenceSum+=d;difference[size_t(tau)]=differenceSum>1e-20?d*tau/differenceSum:1;
   if(++jobLag>maxLag+1){jobLag=0;analyze();}
  }
 }
public:
 void prepare(double fs,int ch) noexcept {
  rate=fs;channels=ch;decimation=std::max(1,int(std::ceil(fs/6000)));analysisRate=fs/decimation;hop=std::max(1,int(std::lround(.008*analysisRate)));
  minLag=std::max(2,int(std::floor(analysisRate/400)));maxLag=std::min(255,int(std::ceil(analysisRate/25)));lagsPerSample=std::max(1,int(std::ceil(double(maxLag+1)/(hop*decimation-1))));
  attack=std::exp(-1/(.003*rate));release=std::exp(-1/(.08*rate));energyA=std::exp(-1/(.05*rate));
  for(auto& channel:filters){channel[0].prepare(rate,.5176380902050415);channel[1].prepare(rate,.7071067811865476);channel[2].prepare(rate,1.9318516525781366);}reset();
 }
 void reset() noexcept {for(auto& channel:filters)for(auto& f:channel)f.z1=f.z2=0;ring={};energy={};estimate={};candidateHz=0;decimationClock=write=filled=hopClock=selected=switchClock=streak=0;selectedInitially=false;jobLag=0;differenceSum=0;}
 PitchEstimate process(double left,double right) noexcept {
  std::array<double,2> x{finite(left),finite(channels==1?left:right)};
  for(size_t c=0;c<2;++c){double amplitude=std::abs(x[c]),a=amplitude>estimate.envelope[c]?attack:release;estimate.envelope[c]=a*estimate.envelope[c]+(1-a)*amplitude;for(auto& f:filters[c])x[c]=f.sample(x[c]);energy[c]=energyA*energy[c]+(1-energyA)*x[c]*x[c];}
  if(selectedInitially&&channels==2){int other=1-selected;if(energy[size_t(other)]>4*energy[size_t(selected)]){if(++switchClock>=int(.1*rate)){selected=other;jobLag=0;estimate.locked=false;estimate.confidence=0;streak=0;candidateHz=0;switchClock=0;}}else switchClock=0;}
  // Do not retain a stale "locked" state through silence while waiting for the next analysis hop.
  if(energy[size_t(selected)]<1e-6){estimate.locked=false;estimate.confidence=0;streak=0;}
  if(++decimationClock>=decimation){decimationClock=0;for(size_t c=0;c<2;++c)ring[c][size_t(write)]=x[c];write=(write+1)&511;filled=std::min(512,filled+1);if(++hopClock>=hop){hopClock=0;if(filled==512){if(!selectedInitially){selected=energy[1]>energy[0]?1:0;selectedInitially=true;}beginAnalysis();}}}
  advanceAnalysis();
  return estimate;
 }
};
}
