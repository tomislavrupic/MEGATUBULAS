#include "VisualResponse.h"
#include "Pipeline.h"
#include "AnimationClock.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <complex>
#include <cstdlib>
#include <memory>
int failures=0,checks=0;
void check(bool condition,const char* name){++checks;if(!condition){std::cerr<<"FAIL: "<<name<<"\n";++failures;}}
std::vector<float> render(double fs,int block,int factor,micro::Parameters p,const std::vector<float>& input,micro::EntropyConfig config={}){
 // Keep large fixtures off the default 1 MiB Windows thread stack, before processing.
 auto pipeStorage=std::make_unique<micro::Pipeline>();auto& pipe=*pipeStorage;pipe.set(p);pipe.prepare(fs,256,2,factor);pipe.configure(config);std::vector<float> a=input,b=input;
 for(size_t i=0;i<a.size();i+=size_t(block)){float* ptr[]={a.data()+i,b.data()+i};pipe.process(ptr,int(std::min(size_t(block),a.size()-i)));}
 check(a==b,"linked stereo consistency");check(pipe.engine.bounded(),"bounded state");return a;
}
double rmsDiff(const std::vector<float>& a,const std::vector<float>& b,size_t start=0){double sum=0;for(size_t i=start;i<a.size();++i)sum+=std::pow(double(a[i])-b[i],2);return std::sqrt(sum/(a.size()-start));}
// Remove the best-fitting constant gain: a level-only change cannot pass this check.
double shapeDifference(const std::vector<float>& a,const std::vector<float>& b,size_t start){
 double aa=0,bb=0,ab=0;for(size_t i=start;i<a.size();++i){aa+=double(a[i])*a[i];bb+=double(b[i])*b[i];ab+=double(a[i])*b[i];}
 const double fit=ab/std::max(bb,1.e-30);double residual=0;for(size_t i=start;i<a.size();++i)residual+=std::pow(a[i]-fit*b[i],2);
 return std::sqrt(residual/std::max(aa,1.e-30));
}
int main(int argc,char** argv){
 std::cout<<std::unitbuf;
 // FM controls follow sample time, including a note change inside arbitrary host blocks.
 for(double fs:{44100.,48000.,96000.})for(int factor:{4,8}){
  std::vector<float> input(size_t(fs*.65));double phase=0;
  for(size_t i=0;i<input.size();++i){double hz=i<size_t(fs*.313)?55.:82.4069;phase+=2*micro::pi*hz/fs;input[i]=float(.25*(std::sin(phase)+.2*std::sin(2*phase)));}
  micro::Parameters p;p.drive=20;p.fmAmount=100;p.fmDepth=70;p.fmRatio=1;
  auto fm=render(fs,127,factor,p,input);p.fmAmount=0;auto legacy=render(fs,127,factor,p,input);
  check(shapeDifference(legacy,fm,size_t(fs*.15))>.005,"enabled FM adds harmonic texture beyond a constant gain");
  p.fmAmount=100;p.fmDepth=0;auto noDepth=render(fs,17,factor,p,input);check(rmsDiff(legacy,noDepth)<1e-7,"zero FM Depth retains original pipeline audio");
  p.fmDepth=70;for(int block:{1,17,512,4096}){auto partition=render(fs,block,factor,p,input);check(rmsDiff(fm,partition)<2e-6,"FM pitch decisions and audio are causal across host partitions");}
  auto reset=render(fs,127,factor,p,input);check(fm==reset,"FM reset/replay deterministic");
  std::vector<float> silence(size_t(fs*.3));auto quiet=render(fs,127,factor,p,silence);check(std::all_of(quiet.begin(),quiet.end(),[](float v){return std::abs(v)<1e-8f;}),"FM enabled silence never generates a sine tone");
  p.blend=0;auto dryFM=render(fs,127,factor,p,input);p.fmAmount=0;auto dryLegacy=render(fs,127,factor,p,input);check(dryFM==dryLegacy,"FM leaves Blend-zero dry path unchanged");
 }
 {micro::Parameters p;p.fmAmount=100;p.fmDepth=100;p.drive=50;auto pipeStorage=std::make_unique<micro::Pipeline>();auto& pipe=*pipeStorage;pipe.set(p);pipe.prepare(48000,127,2,4);std::array<float,127> l{},r{};double phase=0;bool finite=true;for(int block=0;block<700;++block){for(int i=0;i<127;++i){phase+=2*micro::pi*55/48000;double x=block<250?.3*std::sin(phase):0;l[size_t(i)]=float(x);r[size_t(i)]=float(-x);}float* ptr[]={l.data(),r.data()};pipe.process(ptr,127);for(float x:l)finite&=std::isfinite(x);if(block==200)check(pipe.pitchEstimate().locked&&std::abs(pipe.pitchEstimate().hz-55)<.5,"FM pipeline detects anti-phase stereo bass");}double tail=0;for(float x:l)tail=std::max(tail,std::abs(double(x)));check(finite&&tail<1e-6&&!pipe.pitchEstimate().locked,"FM pipeline settles after note release without autonomous tail");}
 // Rapid zero crossings must release/reacquire without truncating an audible FM voice.
 for(bool depthToggle:{false,true})for(double hz:{25.,55.,110.}){
  micro::Parameters p;p.drive=0;p.fmAmount=100;p.fmDepth=70;
  auto pipeStorage=std::make_unique<micro::Pipeline>();auto& pipe=*pipeStorage;pipe.set(p);pipe.prepare(48000,1,1,4);
  float sample=0;float* data[]={&sample};double previous=0,maxStep=0;bool finite=true;
  for(int i=0;i<48000;++i){
   // Multiple phases of the settled voice; each off period is just 1 ms.
   if(i>=16000&&i<40000&&(i-16000)%6000==0){if(depthToggle)p.fmDepth=0;else p.fmAmount=0;pipe.set(p);}
   if(i>=16048&&i<40048&&(i-16048)%6000==0){p.fmAmount=100;p.fmDepth=70;pipe.set(p);}
   sample=float(.5*std::sin(2*micro::pi*hz*i/48000));pipe.process(data,1);
   if(i>15000){maxStep=std::max(maxStep,std::abs(double(sample)-previous));finite&=std::isfinite(sample);}
   previous=sample;
  }
  std::cout<<"rapid FM toggle,"<<depthToggle<<','<<hz<<','<<maxStep<<'\n';
  check(finite&&maxStep<.02,"rapid Amount/Depth zero crossing preserves smooth FM tail");
 }
 // Anti-phase bass switches the strongest channel smoothly; inspect the full handoff.
 // A transient may move within the specified 50-cent continuity window; settled error stays <20.
 {micro::Parameters p;p.drive=50;p.fmAmount=100;p.fmDepth=80;
  auto pipeStorage=std::make_unique<micro::Pipeline>();auto& pipe=*pipeStorage;pipe.set(p);pipe.prepare(48000,1,2,4);
  float l=0,r=0;float* data[]={&l,&r};double previousL=0,previousR=0,maxStep=0,peak=0;int badLocks=0,goodLocks=0;bool finite=true;
  for(int i=0;i<48000;++i){double cross=std::clamp((i-21600)/960.,0.,1.),x=std::sin(2*micro::pi*55*i/48000);
   l=float((.3-.27*cross)*x);r=float(-(.03+.27*cross)*x);pipe.process(data,1);
   if(i>=21000&&i<39000){auto e=pipe.pitchEstimate();badLocks+=e.locked&&std::abs(1200*std::log2(e.hz/55))>50;goodLocks+=e.locked;
    peak=std::max({peak,std::abs(double(l)),std::abs(double(r))});maxStep=std::max({maxStep,std::abs(double(l)-previousL),std::abs(double(r)-previousR)});finite&=std::isfinite(l)&&std::isfinite(r);}
   previousL=l;previousR=r;
  }
  std::cout<<"FM channel handoff,"<<goodLocks<<','<<badLocks<<','<<peak<<','<<maxStep<<'\n';
  check(pipe.pitchEstimate().locked&&std::abs(1200*std::log2(pipe.pitchEstimate().hz/55))<20,"handoff returns to settled pitch within 20 cents");
  check(goodLocks>12000&&badLocks==0,"stronger-channel handoff avoids wrong pitch throughout switch/recovery");
  check(finite&&peak<.8&&maxStep<.03,"stronger anti-phase channel handoff produces no audio burst");
 }
 check(micro::animationFrame(0,0)==0&&micro::animationFrame(0,1)==7.5f&&micro::animationFrame(0,2)==0,"one source-second forward / back animation window");
 check(micro::animationFrame(1,0)==22.5f&&micro::animationFrame(1,1)==30&&micro::animationFrame(.5f,1,false)==11.25f,"Drive chooses animation start / reduced motion");

 check(std::abs(micro::animationSpeed(0,0)-.5f)<1e-6f&&std::abs(micro::animationSpeed(1,0)-.15f)<1e-6f&&micro::animationSpeed(.5f,1)>micro::animationSpeed(.5f,0),"Memory slows animation; Variation adds speed");
 {bool steady=true;for(unsigned w=0;w<=256;++w)for(unsigned value:{0u,30u,128u,255u})steady=steady&&micro::mixVisualByte(value,value,w)==value;check(steady,"constant-brightness frame blending has no midpoint alpha dip");}
 {bool bounded=true;float biggestStep=0;for(int i=0;i<1000;++i){float t=float(i)*.01f;float n=micro::organicVisualNoise(t,t*.3f,t*.1f);bounded=bounded&&n>=0&&n<=1;biggestStep=std::max(biggestStep,std::abs(n-micro::organicVisualNoise(t+.001f,t*.3f,t*.1f)));}check(bounded&&biggestStep<.01f,"organic noise is bounded and spatially continuous");}
 for(double fs:{44100.,48000.,88200.,96000.}){
  std::vector<float> input(12000);for(size_t i=0;i<input.size();++i)input[i]=float(.6*std::sin(2*micro::pi*997*i/fs)+.2*std::sin(2*micro::pi*59*i/fs));
  for(int factor:{4,8}){micro::Parameters p;p.variation=75;p.drive=90;p.coupling=100;
   auto ref=render(fs,1,factor,p,input);for(int block:{32,64,127,256,1024}){auto out=render(fs,block,factor,p,input);check(rmsDiff(ref,out)<2e-6,"sample-clock block partition consistency");}
   auto replay=render(fs,127,factor,p,input);check(rmsDiff(ref,replay)<2e-6,"seeded deterministic reset/replay");
   std::vector<float> silence(12000,0);auto zero=render(fs,127,factor,p,silence);double max=0;for(float v:zero)max=std::max(max,std::abs(double(v)));check(max<1e-8,"silence generates no noise");
   p.drive=0;p.ablate=true;auto linear=render(fs,127,factor,p,input);auto probeStorage=std::make_unique<micro::Pipeline>();auto& probe=*probeStorage;probe.set(p);probe.prepare(fs,127,1,factor);int latency=probe.getLatency();double delta=0;for(size_t i=2048;i<input.size();++i)delta+=std::pow(linear[i]-input[i-size_t(latency)],2);check(std::sqrt(delta/(input.size()-2048))<.002,"linear gain calibration / aligned conversion");
  }
 }
 {micro::Engine a,b;micro::Parameters p;p.drive=80;p.memory=90;p.coupling=80;a.set(p);b.set(p);a.prepare(192000);b.prepare(192000);for(int i=0;i<192000;++i){double x[]={.8*std::sin(2*micro::pi*220*i/192000)},y[]={.01*std::sin(2*micro::pi*220*i/192000)};a.sample(x,1);b.sample(y,1);}double difference=0;for(int i=0;i<9600;++i){double x[]={.3*std::sin(2*micro::pi*440*i/192000)},y[]={x[0]};a.sample(x,1);b.sample(y,1);difference+=std::abs(x[0]-y[0]);}check(difference/9600>1e-4,"same probe differs after different histories");}
 {micro::Parameters p;p.drive=85;p.memory=80;p.coupling=100;std::vector<float> in(48000);for(size_t i=0;i<in.size();++i)in[i]=float(.7*std::sin(2*micro::pi*110*i/48000));auto full=render(48000,127,4,p,in);p.ablate=true;auto ablated=render(48000,127,4,p,in);check(rmsDiff(full,ablated,10000)>1e-4,"memory/coupling ablation changes response");}
 // Regression for nearly inert controls: a dynamic bass phrase, not only steady sine/gain.
 {std::vector<float> phrase(144000);for(size_t i=0;i<phrase.size();++i){double t=double(i)/48000,beat=std::fmod(t,.375);double amplitude=(int(t/.375)%2?.08:.45)*(.15+.85*std::exp(-beat/.075));phrase[i]=float(amplitude*(std::sin(2*micro::pi*80*t)+.2*std::sin(2*micro::pi*160*t)));}
  for(int mode=0;mode<3;++mode)for(int factor:{4,8}){micro::Parameters p;p.drive=75;p.mode=mode;p.coupling=0;p.memory=0;auto fast=render(48000,127,factor,p,phrase);p.memory=100;auto slow=render(48000,127,factor,p,phrase);double memoryDelta=shapeDifference(fast,slow,24000);std::cout<<"Mode "<<mode<<" / "<<factor<<"x Memory shape difference "<<memoryDelta*100<<"%\n";check(memoryDelta>.05,"Memory extremes reshape dynamic bass after constant-gain removal");
   p.memory=40;auto independent=render(48000,127,factor,p,phrase);p.coupling=100;auto coupled=render(48000,127,factor,p,phrase);double couplingDelta=shapeDifference(independent,coupled,24000);std::cout<<"Mode "<<mode<<" / "<<factor<<"x Coupling shape difference "<<couplingDelta*100<<"%\n";check(couplingDelta>.05,"Coupling extremes reshape dynamic bass after constant-gain removal");
  }
  micro::Parameters p;p.drive=0;p.memory=0;p.coupling=0;auto neutral=render(48000,127,4,p,phrase);p.memory=100;p.coupling=100;auto extreme=render(48000,127,4,p,phrase);check(rmsDiff(neutral,extreme)<1.e-7,"Memory and Coupling retain the linear Drive-zero calibration path");
 }
 // A loud note must leave a clearly longer recovery on the following quiet note.
 {std::vector<float> probe(192000);for(size_t i=0;i<probe.size();++i)probe[i]=float((i<48000?.7:.035)*std::sin(2*micro::pi*100*i/48000));
  auto windowRms=[](const auto& audio,size_t begin,size_t end){double e=0;for(size_t i=begin;i<end;++i)e+=double(audio[i])*audio[i];return std::sqrt(e/(end-begin));};
  micro::Parameters p;p.drive=75;p.memory=0;p.coupling=0;auto fast=render(48000,127,8,p,probe);p.memory=100;auto held=render(48000,127,8,p,probe);
  double shortRecovery=20*std::log10(windowRms(fast,52800,57600)/windowRms(fast,168000,172800));double longRecovery=20*std::log10(windowRms(held,52800,57600)/windowRms(held,168000,172800));
  std::cout<<"Quiet note recovery: Memory 0 "<<shortRecovery<<" dB; Memory 100 "<<longRecovery<<" dB (early vs late)\n";
  check(std::abs(shortRecovery)<.25,"low Memory recovers promptly after a loud note");check(longRecovery< -2,"high Memory leaves audible sag that recovers through a quiet note");
 }
 {micro::EntropyConfig c;c.kind=micro::EntropyConfig::replay;c.count=2;c.values[0]=123;c.values[1]=4000000000u;micro::EntropySource s;s.configure(c);double x=0,y=0;check(s.draw(x)&&s.draw(y)&&!s.draw(y),"bounded entropy exhaustion");s.reset();double z=0;s.draw(z);check(x==z,"saved entropy replay reset");c.count=1025;check(!c.valid(),"oversized entropy rejected");}
 {micro::Engine engine;micro::Parameters p;p.drive=100;p.memory=100;p.coupling=100;p.variation=100;engine.set(p);engine.prepare(384000);for(int i=0;i<384000;++i){if(i%1000==0){p.mode=(i/1000)%3;p.drive=(i/1000)%2?100:0;engine.set(p);}double x[]={i%997==0?std::numeric_limits<double>::quiet_NaN():32*std::sin(i*.3),std::numeric_limits<double>::infinity()};engine.sample(x,2);if(!std::isfinite(x[0])||!std::isfinite(x[1])||!engine.bounded()){check(false,"extreme/automation/nonfinite stress");break;}}check(engine.bounded(),"sustained extreme boundedness");}
 {micro::Parameters p;p.drive=95;p.coupling=100;std::vector<float> dc(144000,.5);auto out=render(48000,127,4,p,dc);double mean=0;for(size_t i=96000;i<out.size();++i)mean+=out[i];check(std::abs(mean/48000)<.0001,"DC removed after settling");}
 {micro::Parameters p;p.drive=70;p.variation=85;std::vector<float> input(48000);for(size_t i=0;i<input.size();++i)input[i]=float(.5*std::sin(i*.03));micro::EntropyConfig prng,sequence;sequence.kind=micro::EntropyConfig::replay;sequence.count=32;micro::EntropySource source;source.configure(prng);for(uint32_t i=0;i<sequence.count;++i){double draw;source.draw(draw);sequence.values[i]=uint32_t(draw*4294967296.);}auto a=render(48000,127,4,p,input,prng),b=render(48000,127,4,p,input,sequence);check(rmsDiff(a,b)<1e-7,"same draws yield same selector/audio regardless of provenance");}
 {for(int factor:{4,8}){auto pipeStorage=std::make_unique<micro::Pipeline>();auto& pipe=*pipeStorage;micro::Parameters p;p.drive=0;p.blend=100;pipe.set(p);pipe.prepare(48000,127,1,factor);std::vector<float> impulse(4096);impulse[0]=.01f;float* ptr[]={impulse.data()};pipe.process(ptr,4096);auto peak=int(std::max_element(impulse.begin(),impulse.end())-impulse.begin());check(peak==pipe.getLatency(),"measured impulse latency equals reported latency");std::cout<<factor<<"x measured latency "<<peak<<" samples\n";}}
 // Bass-first voicing: settled small-signal response, before the user EQ.
 {auto response=[&](double frequency){std::vector<float> in(96000);for(size_t i=0;i<in.size();++i)in[i]=float(1.e-4*std::sin(2*micro::pi*frequency*i/48000));micro::Parameters p;p.drive=80;auto voiced=render(48000,127,4,p,in);p.drive=0;auto linear=render(48000,127,4,p,in);double a=0,b=0;for(size_t i=48000;i<in.size();++i){a+=double(voiced[i])*voiced[i];b+=double(linear[i])*linear[i];}return 10*std::log10(a/b);};
  const double bass=response(70),mid=response(1000),top=response(12000);std::cout<<"Warm / Drive 80 small-signal: 70 Hz "<<bass<<" dB, 12 kHz "<<top<<" dB relative to Drive 0\n";
  check(bass-mid>.4&&bass-mid<3,"bass body reinforced relative to mids");check(top-mid< -4&&top-mid> -14,"upper-frequency energy restrained relative to mids in Warm");
 }
 {micro::Parameters p;p.drive=80;p.memory=60;std::vector<float> in(96000);for(size_t i=0;i<in.size();++i)in[i]=float(.5*std::sin(2*micro::pi*100*i/48000));auto out=render(48000,127,4,p,in);
  auto harmonic=[&](int multiple){std::complex<double> sum{};for(size_t i=48000;i<out.size();++i)sum+=double(out[i])*std::polar(1.,-2*micro::pi*100*multiple*double(i)/48000);return 2*std::abs(sum)/48000;};
  const double body=std::hypot(harmonic(2),harmonic(3));double upper=0;for(int h=60;h<=150;++h)upper+=std::pow(harmonic(h),2);upper=std::sqrt(upper);
  std::cout<<"100 Hz bass probe: H2+H3 "<<20*std::log10(body)<<" dBFS, 6-15 kHz harmonic sum "<<20*std::log10(upper+1e-20)<<" dBFS\n";
  check(body>.001&&body>upper*20,"bass probe produces low-order harmonics above upper harmonic tail");
 }
 {micro::Parameters p;p.drive=65;std::vector<float> in(96000);for(size_t i=0;i<in.size();++i)in[i]=float(.0316227766*std::sin(2*micro::pi*100*i/48000));auto out=render(48000,127,8,p,in);auto amplitude=[&](int h){std::complex<double> sum{};for(size_t i=48000;i<out.size();++i)sum+=double(out[i])*std::polar(1.,-2*micro::pi*100*h*double(i)/48000);return 2*std::abs(sum)/48000;};const double h3=20*std::log10(amplitude(3)/amplitude(1));std::cout<<"Quiet bass / Drive 65: third harmonic "<<h3<<" dBc\n";check(h3> -32&&h3< -6,"Drive 65 produces substantial low-order saturation even on -30 dBFS bass");}
 {auto pipeStorage=std::make_unique<micro::Pipeline>();auto& pipe=*pipeStorage;micro::Parameters p;pipe.set(p);pipe.prepare(48000,127,2,8);std::array<float,127> l{},r{};bool ok=true;
  for(int block=0;block<600;++block){p.drive=block%2?100:0;p.mode=block%3;p.memory=100;p.coupling=100;p.variation=100;p.input=block%2?12:-24;p.output=block%3?12:-24;p.blend=block%2?100:0;p.preLow=block%3;p.preHigh=(block+1)%3;for(int b=0;b<4;++b)p.eq[size_t(b)]=(block+b)%2?12:-12;pipe.set(p);for(int i=0;i<127;++i)l[size_t(i)]=r[size_t(i)]=float(2*std::sin((block*127+i)*.17));float* data[]={l.data(),r.data()};pipe.process(data,127);for(auto v:l)ok=ok&&std::isfinite(v)&&std::abs(v)<4096;}
  check(ok&&pipe.engine.bounded(),"full-pipeline rapid gain/EQ/voicing automation remains finite");
 }
 // Independent analytic center/DC/Nyquist identities for RBJ bell/shelves.
 for(int band=0;band<4;++band)for(double db:{-12.,-3.,0.,6.,12.}){auto c=micro::Biquad::coefficients(band,db,48000);double f=std::array{100.,500.,1500.,5000.}[size_t(band)];std::complex<double> z=std::polar(1.,-2*micro::pi*(band==0?0:band==3?24000:f)/48000);double response=std::abs((c[0]+c[1]*z+c[2]*z*z)/(1.+c[3]*z+c[4]*z*z));check(std::abs(20*std::log10(response)-db)<1e-7,"EQ independent gain identity");}
 if(argc>1){juce::File dir(juce::String::fromUTF8(argv[1]));dir.createDirectory();std::ofstream report(dir.getChildFile("measurements.csv").getFullPathName().toStdString());report<<"sample,source,wet4,wet8,reference32,blend50,burst\n";std::vector<float> signal(65536),burst(65536);for(size_t i=0;i<signal.size();++i){signal[i]=float(.5*std::sin(2*micro::pi*7001*i/48000)+.1*std::sin(2*micro::pi*701*i/48000));burst[i]=i<24000?float(.7*std::sin(2*micro::pi*110*i/48000)):float(.1*std::sin(2*micro::pi*110*i/48000));}micro::Parameters p;p.drive=80;p.memory=80;p.coupling=80;auto t=std::chrono::steady_clock::now();auto a=render(48000,127,4,p,signal);auto b=render(48000,127,8,p,signal);auto recovery=render(48000,127,4,p,burst);
 // Higher-rate independent engine render: JUCE 32x FIR conversion, same nonlinear core.
 juce::dsp::Oversampling<float> os(1,5,juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,true,true);os.initProcessing(signal.size());juce::AudioBuffer<float> rb(1,int(signal.size()));rb.copyFrom(0,0,signal.data(),int(signal.size()));auto block=juce::dsp::AudioBlock<float>(rb);auto up=os.processSamplesUp(block);micro::Engine high;high.set(p);high.prepare(48000*32);for(size_t i=0;i<up.getNumSamples();++i){double x[]={up.getSample(0,i)};high.sample(x,1);up.setSample(0,i,float(x[0]));}os.processSamplesDown(block);int latency=int(std::round(os.getLatencyInSamples()));
 // Apply the same documented post-saturation voicing to the higher-rate reference.
 auto voicingStorage=std::make_unique<micro::Pipeline>();auto& voicing=*voicingStorage;voicing.set(p);const auto tone=voicing.characterTone();std::array<micro::Biquad,2> referenceTone;for(int band=0;band<2;++band)referenceTone[size_t(band)].c=micro::Biquad::coefficients(band==0?0:3,tone[size_t(band)],48000);
 for(int i=0;i<rb.getNumSamples();++i){double y=rb.getSample(0,i);for(auto& filter:referenceTone)y=filter.process(y);rb.setSample(0,i,float(y));}
 p.blend=50;auto blend=render(48000,127,4,p,signal);
 for(size_t i=0;i<signal.size();++i)report<<i<<','<<signal[i]<<','<<a[i]<<','<<b[i]<<','<<rb.getSample(0,int(i))<<','<<blend[i]<<','<<recovery[i]<<'\n';
 std::ofstream harmonics(dir.getChildFile("harmonics.csv").getFullPathName().toStdString());harmonics<<"sample,low4,medium4,high4,low8,medium8,high8\n";std::array<std::vector<float>,6> tones;
 for(int q=0;q<2;++q)for(int level=0;level<3;++level){std::vector<float> in(32768);double amplitude=std::array{.0316227766,.177827941,.707945784}[size_t(level)];for(size_t i=0;i<in.size();++i)in[i]=float(amplitude*std::sin(2*micro::pi*997*i/48000));p.blend=100;tones[size_t(q*3+level)]=render(48000,127,q?8:4,p,in);}
 for(size_t i=0;i<32768;++i){harmonics<<i;for(auto& tone:tones)harmonics<<','<<tone[i];harmonics<<'\n';}
 std::ofstream cpu(dir.getChildFile("cpu.csv").getFullPathName().toStdString());cpu<<"quality,seconds_per_audio_second\n";
 for(int quality:{4,8}){auto benchmarkStorage=std::make_unique<micro::Pipeline>();auto& benchmark=*benchmarkStorage;benchmark.set(p);benchmark.prepare(48000,256,2,quality);std::array<float,256> l{},r{};double elapsed=0;auto begin=std::chrono::steady_clock::now();for(int blockIndex=0;blockIndex<188;++blockIndex){for(int i=0;i<256;++i)l[size_t(i)]=r[size_t(i)]=float(.3*std::sin((blockIndex*256+i)*.03));float* channels[]={l.data(),r.data()};benchmark.process(channels,256);}elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();cpu<<quality<<','<<elapsed/(188*256/48000.)<<'\n';}
 std::ofstream meta(dir.getChildFile("measurement-meta.txt").getFullPathName().toStdString());meta<<"Reference 32x latency: "<<latency<<" samples\nRender/analysis total wall seconds: "<<std::chrono::duration<double>(std::chrono::steady_clock::now()-t).count()<<"\n";
 }
 std::cout<<checks<<" checks, "<<failures<<" failures\n";return failures?EXIT_FAILURE:EXIT_SUCCESS;
}
