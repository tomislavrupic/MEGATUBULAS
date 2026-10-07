#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
namespace micro {
constexpr double pi=3.14159265358979323846;
inline double finite(double x) noexcept { return std::isfinite(x)?std::clamp(x,-32.0,32.0):0.0; }
inline double gain(double db) noexcept { return std::pow(10.,db/20.); }
struct Parameters {
 double drive=30, memory=40, coupling=30, blend=100, output=0, input=0;
 double wetLevel=0;int preLow=1,preHigh=1;
 std::array<double,4> eq{}; double variation=0; int mode=0;
 bool freeze=false, bypass=false, ablate=false;
};
struct EntropyConfig {
 enum Kind { seeded, imported, replay }; int kind=seeded;
 uint32_t seed=0x4d494352; uint32_t count=0,start=0; int frozenIndex=0;
 std::array<uint32_t,1024> values{};
 double matchOffset=0;bool hasFrozen=false;std::array<double,6> frozenVoice{1,.05,.08,1,0,1};
 std::array<char,128> source{},identifier{},importedAt{};
 bool valid() const noexcept { if(!std::isfinite(matchOffset)||std::abs(matchOffset)>12)return false;for(double v:frozenVoice)if(!std::isfinite(v)||std::abs(v)>4)return false;return kind>=0&&kind<=2&&count<=values.size()&& (kind==seeded || (count>0&&start<count))&&frozenIndex>=0&&frozenIndex<8; }
};
// All storage fixed. No operating-system or network entropy enters this class.
class EntropySource {
 EntropyConfig config; uint32_t state=1,position=0;
public:
 void configure(const EntropyConfig& c) noexcept { config=c; reset(); }
 void reset() noexcept { state=config.seed?config.seed:1; position=config.start; }
 bool draw(double& out) noexcept {
  uint32_t n;
  if(config.kind==EntropyConfig::seeded){ state^=state<<13;state^=state>>17;state^=state<<5;n=state; }
  else { if(position>=config.count)return false; n=config.values[position++]; }
  out=(double(n)+.5)/4294967296.; return true;
 }
 uint32_t consumed() const noexcept { return position; }
};
struct Voice { double drive=1,asym=.05,bias=.08,recovery=1,emphasis=0,coupling=1; };
inline Voice candidate(int mode,int index) noexcept {
 const double u=(index-3.5)/3.5;
 Voice v; v.drive=(mode==1?1.28:mode==2?.82:1.)*(1+.15*u);
 v.asym=(mode==1?.13:mode==2?.025:.065)+.025*u;
 v.bias=(mode==1?.16:mode==2?.04:.09)*(1+.2*u);
 v.recovery=1+.3*u;v.emphasis=.025*u+(mode==1?-.055:mode==2?-.015:-.085);
 v.coupling=(mode==1?1.35:mode==2?.65:1.)*(1+.2*u); return v;
}
struct Stage { double env=0,bias=0,low=0,dcX=0,dcY=0; };
class Engine {
 double fs=192000,envA=0,envAttackA=0,biasA=0,lowA=0,dcA=0,smoothA=0,analysisA=0,fastA=0,splitA=0;
 std::array<std::array<Stage,3>,2> stages{};
 double level=0,fast=0,low=0,high=0;std::array<double,2> split{};
 Voice voice,target; Parameters p,smoothed;
 EntropySource entropy; EntropyConfig config;
 uint64_t clock=0,lastDecision=0; uint32_t control=3840,dwell=30720;
 int selected=3,lastMode=0; bool wasFrozen=false,exhausted=false,force=false;
public:
 uint64_t events=0;
 void prepare(double sampleRate) noexcept {
  fs=sampleRate;control=uint32_t(std::max(1.,std::round(.02*fs)));dwell=uint32_t(std::round(.16*fs));
  lowA=std::exp(-2*pi*700/fs);dcA=std::exp(-2*pi*12/fs);smoothA=std::exp(-1/(.05*fs));
  analysisA=std::exp(-1/(.06*fs));fastA=envAttackA=std::exp(-1/(.003*fs));splitA=std::exp(-2*pi*900/fs);reset();
 }
 void configure(const EntropyConfig& c) noexcept { if(c.valid())config=c;entropy.configure(config);reset(); }
 void reset() noexcept {
  stages={};level=fast=low=high=0;split={};clock=lastDecision=events=0;selected=config.frozenIndex;
  lastMode=std::clamp(p.mode,0,2);voice=target=candidate(lastMode,selected);smoothed=p;
  if(config.hasFrozen&&p.freeze)voice=target={config.frozenVoice[0],config.frozenVoice[1],config.frozenVoice[2],config.frozenVoice[3],config.frozenVoice[4],config.frozenVoice[5]};
  entropy.reset();exhausted=false;wasFrozen=p.freeze;force=false;updateTime();
 }
 void set(const Parameters& value) noexcept { p=value;p.mode=std::clamp(p.mode,0,2); }
 void explore() noexcept {force=true;}
 void updateTime() noexcept {
  double tau=.012*std::pow(200.,std::clamp(smoothed.memory,0.,100.)/100.)*voice.recovery;
  envA=std::exp(-1/(tau*fs));biasA=std::exp(-1/(tau*.6*fs));
 }
 void decide() noexcept {
  if(p.freeze){wasFrozen=true;return;} wasFrozen=false;
  if(!force && clock-lastDecision<dwell && p.mode==lastMode)return;
  lastMode=p.mode;lastDecision=clock;force=false;
  const double variation=std::clamp(p.variation/100.,0.,1.);
  std::array<double,8> score{},prob{};double maxScore=-1e30;
  const double bright=std::clamp(high/(low+high+1e-9),0.,1.);
  const double transient=std::clamp((fast-level)/(level+.01),0.,1.);
  for(int i=0;i<8;++i){ double u=i/7.;score[i]=-.9*std::abs(u-(.2+.4*bright+.2*transient))-.18*std::abs(i-selected);maxScore=std::max(maxScore,score[i]); }
  int choice=int(std::max_element(score.begin(),score.end())-score.begin());
  if(variation>0){double draw=0;if(!entropy.draw(draw)){exhausted=true;return;}
   double sum=0;for(int i=0;i<8;++i){prob[i]=std::exp((score[i]-maxScore)/(.06+.65*variation));sum+=prob[i];}
   double threshold=draw*sum;choice=7;for(int i=0;i<8;++i)if((threshold-=prob[i])<=0){choice=i;break;}
  }
  selected=choice;Voice c=candidate(p.mode,choice),base=candidate(p.mode,3);
  auto lerp=[variation](double a,double b){return a+variation*(b-a);};
  target={lerp(base.drive,c.drive),lerp(base.asym,c.asym),lerp(base.bias,c.bias),lerp(base.recovery,c.recovery),lerp(base.emphasis,c.emphasis),lerp(base.coupling,c.coupling)};
  ++events;
 }
 void sample(double* x,int channels) noexcept {
  double linked=0;for(int c=0;c<channels;++c){x[c]=finite(x[c]);linked+=std::abs(x[c]);}linked/=channels;
  level=analysisA*level+(1-analysisA)*linked;fast=fastA*fast+(1-fastA)*linked;
  double lo=0,hi=0;for(int c=0;c<channels;++c){split[c]=splitA*split[c]+(1-splitA)*x[c];lo+=std::abs(split[c]);hi+=std::abs(x[c]-split[c]);}
  low=analysisA*low+(1-analysisA)*lo/channels;high=analysisA*high+(1-analysisA)*hi/channels;
  if(clock%control==0){decide();updateTime();}++clock;
  auto smooth=[this](double& a,double b){a=smoothA*a+(1-smoothA)*b;};
  smooth(smoothed.drive,p.drive);smooth(smoothed.memory,p.memory);smooth(smoothed.coupling,p.coupling);
  smooth(voice.drive,target.drive);smooth(voice.asym,target.asym);smooth(voice.bias,target.bias);smooth(voice.recovery,target.recovery);smooth(voice.emphasis,target.emphasis);smooth(voice.coupling,target.coupling);
  const double amount=std::clamp(smoothed.drive/100.,0.,1.),driveCurve=std::pow(10.,1.6*amount)-1;
  const double memory=p.ablate?0:std::clamp(smoothed.memory/100.,0.,1.);
  const double couplingDepth=p.ablate?0:std::clamp(smoothed.coupling/100.,0.,1.)*voice.coupling;
  for(int c=0;c<channels;++c){double y=x[c]; const auto previous=stages[c];
   for(int k=0;k<3;++k){auto& s=stages[c][k];s.low=lowA*s.low+(1-lowA)*y;
    double conditioned=y+amount*voice.emphasis*(y-s.low);
    double prior=k?previous[k-1].env:0;
    const double coupling=couplingDepth*std::tanh(4*prior);
    // Only upstream, previous-sample state is read: no feedback/resonant solver.
    // A bounded 700 Hz feed-forward path brings weight and changes harmonic texture
    // instead of sending more high-frequency edge into the later nonlinear stages.
    if(k)conditioned+=amount*.35*couplingDepth*std::tanh(3*previous[k-1].low);
    if(p.ablate){s.env=s.bias=0;}else{
     const double envelopeInput=std::abs(conditioned),a=envelopeInput>s.env?envAttackA:envA;
     s.env=std::clamp(a*s.env+(1-a)*envelopeInput,0.,8.);
     double biasTarget=voice.bias*std::tanh(s.env+2*coupling)*(1+2*memory*memory);
     s.bias=std::clamp(biasA*s.bias+(1-biasA)*biasTarget,-.3,.3);
    }
    // Fast charging and Memory-dependent discharge create audible, signal-driven sag.
    // Depth also rises with Memory: changing only a time constant was almost inert
    // on sustained, already-compressed material. Drive zero remains linear.
    const double charge=std::tanh(4*s.env),sag=1/(1+1.8*memory*memory*charge);
    double drive=1+driveCurve*voice.drive*(.8+.18*k)*(1+1.4*coupling+.2*std::tanh(s.env))*sag;
    double b=voice.asym+s.bias;
    double shaped=(std::tanh(drive*conditioned+b)-std::tanh(b))/(std::pow(drive,1-.5*amount)*(1-std::pow(std::tanh(b),2)));
    // Drive zero is calibrated. Higher Drive retains deliberate gain and harmonic density;
    // square-root compensation at full Drive restrains level without cancelling the drive.
    y=(conditioned+amount*(shaped-conditioned))/(1+.5*amount*memory*memory*charge);
    if(amount>0){double dc=y-s.dcX+dcA*s.dcY;s.dcX=y;s.dcY=finite(dc);y=s.dcY;}
   } x[c]=finite(y);
  }
 }
 std::array<double,6> selectedVoice() const noexcept{return {target.drive,target.asym,target.bias,target.recovery,target.emphasis,target.coupling};}
 int index() const noexcept{return selected;} bool isExhausted() const noexcept{return exhausted;}
 uint32_t consumed() const noexcept{return entropy.consumed();}
 double memoryState(int stage) const noexcept{return (stages[0][stage].env+stages[1][stage].env)*.5;}
 bool bounded() const noexcept {for(auto& ch:stages)for(auto& s:ch)if(!std::isfinite(s.env)||s.env>8||std::abs(s.bias)>.3||!std::isfinite(s.dcY))return false;return true;}
};
struct Biquad {
 std::array<double,5> c{1,0,0,0,0};double z1=0,z2=0;
 void reset() noexcept {z1=z2=0;}
 double process(double x) noexcept {double y=c[0]*x+z1;z1=c[1]*x-c[3]*y+z2;z2=c[2]*x-c[4]*y;return y;}
 static std::array<double,5> coefficients(int band,double db,double fs) noexcept {
  const double f=std::min(std::array<double,4>{100,500,1500,5000}[band],fs*.4),w=2*pi*f/fs,cs=std::cos(w),sn=std::sin(w),A=std::pow(10.,db/40.);
  double b0,b1,b2,a0,a1,a2;
  if(band==1||band==2){double alpha=sn/(2*.7071067811865476);b0=1+alpha*A;b1=-2*cs;b2=1-alpha*A;a0=1+alpha/A;a1=-2*cs;a2=1-alpha/A;}
  else {double alpha=sn/std::sqrt(2.),beta=2*std::sqrt(A)*alpha;
   if(band==0){b0=A*((A+1)-(A-1)*cs+beta);b1=2*A*((A-1)-(A+1)*cs);b2=A*((A+1)-(A-1)*cs-beta);a0=(A+1)+(A-1)*cs+beta;a1=-2*((A-1)+(A+1)*cs);a2=(A+1)+(A-1)*cs-beta;}
   else{b0=A*((A+1)+(A-1)*cs+beta);b1=-2*A*((A-1)+(A+1)*cs);b2=A*((A+1)+(A-1)*cs-beta);a0=(A+1)-(A-1)*cs+beta;a1=2*((A-1)-(A+1)*cs);a2=(A+1)-(A-1)*cs-beta;}
  }return {b0/a0,b1/a0,b2/a0,a1/a0,a2/a0};
 }
};
}
