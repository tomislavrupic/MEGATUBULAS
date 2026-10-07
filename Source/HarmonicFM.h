#pragma once
#include "Core.h"
namespace micro {
struct FMControl {double hz=0,gate=0;std::array<double,2> envelope{};};
// A pitch-controlled synthesis layer, not modulation of the recorded waveform.
class HarmonicFM {
 double fs=192000,paramA=0,pitchA=0,gateAttack=0,gateRelease=0,dcA=0;
 double targetAmount=0,targetDepth=.25,amount=0,depth=0,phase=0,hz=0,lockGain=0;
 int ratio=1,remaining=0,rampLength=1;
 std::array<double,3> weights{1,0,0},weightStep{};
 std::array<double,2> dcX{},dcY{},z1{},z2{};
 double b0=0,b1=0,b2=0,a1=0,a2=0;
 static double bounded(double v,double lo,double hi) noexcept {return std::isfinite(v)?std::clamp(v,lo,hi):lo;}
public:
 void prepare(double sampleRate) noexcept {
  fs=sampleRate;paramA=std::exp(-1/(.02*fs));pitchA=std::exp(-1/(.01*fs));gateAttack=std::exp(-1/(.01*fs));gateRelease=std::exp(-1/(.04*fs));dcA=std::exp(-2*pi*12/fs);rampLength=std::max(1,int(std::lround(.02*fs)));
  double w=2*pi*std::min(3000.,fs*.4)/fs,c=std::cos(w),alpha=std::sin(w)/std::sqrt(2.),d=1+alpha;
  b0=(1-c)*.5/d;b1=(1-c)/d;b2=b0;a1=-2*c/d;a2=(1-alpha)/d;reset();
 }
 void reset() noexcept {amount=depth=phase=hz=lockGain=0;remaining=0;weights={0,0,0};weights[size_t(ratio-1)]=1;weightStep={};dcX=dcY=z1=z2={};}
 void set(double a,double d,int r) noexcept {
  targetAmount=bounded(a,0,100)/100;targetDepth=bounded(d,0,100)/100;r=std::clamp(r,1,3);
  if(r!=ratio){ratio=r;remaining=rampLength;for(size_t i=0;i<3;++i)weightStep[i]=((int(i)==ratio-1?1.:0.)-weights[i])/rampLength;}
 }
 std::array<double,2> sample(const FMControl& control) noexcept {
  amount=paramA*amount+(1-paramA)*targetAmount;depth=paramA*depth+(1-paramA)*targetDepth;
  if(targetAmount==0&&amount<1e-9)amount=0;if(targetDepth==0&&depth<1e-9)depth=0;
  const bool validPitch=std::isfinite(control.hz)&&control.hz>=25&&control.hz<=400;
  double targetGate=validPitch?bounded(control.gate,0,1):0,a=targetGate>lockGain?gateAttack:gateRelease;
  lockGain=a*lockGain+(1-a)*targetGate;if(targetGate==0&&lockGain<1e-9)lockGain=0;
  if(validPitch)hz=hz>0?pitchA*hz+(1-pitchA)*control.hz:control.hz;
  if(remaining>0){for(size_t i=0;i<3;++i)weights[i]+=weightStep[i];if(--remaining==0){weights={0,0,0};weights[size_t(ratio-1)]=1;}}
  if(hz>0){phase+=2*pi*hz/fs;if(phase>=2*pi)phase-=2*pi;}
  std::array<double,2> envelope{bounded(control.envelope[0],0,32),bounded(control.envelope[1],0,32)},out{};
  double index=3*depth*(.25+.75*std::tanh(4*std::max(envelope[0],envelope[1]))),wave=0;
  // No sine carrier at zero Depth. During disable, a bounded 20 ms parameter fade remains.
  const double depthGate=std::min(1.,depth*1000.);
  if(amount>0&&depth>0&&lockGain>0)for(size_t r=0;r<3;++r)if(weights[r]>1e-12)wave+=weights[r]*std::sin(phase+index*std::sin((r+1)*phase));
  for(size_t c=0;c<2;++c){double v=.5*envelope[c]*amount*lockGain*depthGate*wave;double dc=v-dcX[c]+dcA*dcY[c];dcX[c]=v;dcY[c]=dc;
   double y=b0*dc+z1[c];z1[c]=b1*dc-a1*y+z2[c];z2[c]=b2*dc-a2*y;out[c]=std::isfinite(y)?y:0;
  }return out;
 }
 double gate() const noexcept {return lockGain;}
 bool active() const noexcept {
  if(targetAmount>0&&targetDepth>0||amount>1e-9&&depth>1e-9)return true;
  for(size_t c=0;c<2;++c)if(std::abs(dcX[c])+std::abs(dcY[c])+std::abs(z1[c])+std::abs(z2[c])>1e-12)return true;
  return false;
 }
};
}
