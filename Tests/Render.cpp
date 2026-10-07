#include "Pipeline.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cerrno>
int main(int argc,char** argv){
 if(argc<4){std::cerr<<"MegaRender input.wav output.wav dry|drive [mode 0..2] [match -20 dBFS RMS: 0|1] [memory 0..100] [coupling 0..100] [FM amount 0..100] [FM depth 0..100] [FM ratio index 0..2] [pitch.csv]\n";return 1;}
 double amount=0,depth=25,ratio=0;
 auto number=[&](int index,double maximum,double& value,bool integer=false){if(argc<=index)return true;char* end=nullptr;errno=0;double parsed=std::strtod(argv[index],&end);if(errno||end==argv[index]||*end||!std::isfinite(parsed)||parsed<0||parsed>maximum||(integer&&parsed!=std::floor(parsed)))return false;value=parsed;return true;};
 if(argc>12||!number(8,100,amount)||!number(9,100,depth)||!number(10,2,ratio,true)){std::cerr<<"Invalid FM arguments\n";return 1;}
 const juce::File input(juce::String::fromUTF8(argv[1])),output(juce::String::fromUTF8(argv[2]));
 if(output.exists()){std::cerr<<"Refusing to overwrite output\n";return 1;}
 juce::AudioFormatManager formats;formats.registerBasicFormats();std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(input));
 if(!reader||reader->lengthInSamples<128||reader->numChannels<1||reader->numChannels>2||reader->lengthInSamples>reader->sampleRate*600){std::cerr<<"Expected mono/stereo audio, at most 10 minutes\n";return 1;}
 const int n=int(reader->lengthInSamples),channels=int(reader->numChannels);const double fs=reader->sampleRate;
 juce::AudioBuffer<float> audio(channels,n),dry(channels,n);if(!reader->read(&audio,0,n,0,true,true)){std::cerr<<"Read failed\n";return 1;}
 micro::Parameters p;p.bypass=juce::String(argv[3])=="dry";p.drive=p.bypass?0:std::clamp(std::atof(argv[3]),0.,100.);p.mode=argc>4?std::clamp(std::atoi(argv[4]),0,2):0;
 if(argc>6)p.memory=std::clamp(std::atof(argv[6]),0.,100.);if(argc>7)p.coupling=std::clamp(std::atof(argv[7]),0.,100.);
 p.fmAmount=amount;p.fmDepth=depth;p.fmRatio=int(ratio);
 std::ofstream pitchLog;if(argc>11){if(juce::File(argv[11]).exists()){std::cerr<<"Refusing to overwrite pitch log\n";return 1;}pitchLog.open(argv[11]);if(!pitchLog){std::cerr<<"Cannot write pitch log\n";return 1;}pitchLog<<"end_sample,hz,confidence,locked,gate\n";}
 micro::Pipeline pipeline;pipeline.set(p);pipeline.prepare(fs,256,channels,8);
 dry.clear();for(int c=0;c<channels;++c)dry.copyFrom(c,pipeline.getLatency(),audio,c,0,n-pipeline.getLatency());
 for(int offset=0;offset<n;offset+=127){float* frame[2]{};for(int c=0;c<channels;++c)frame[c]=audio.getWritePointer(c,offset);pipeline.process(frame,std::min(127,n-offset));if(pitchLog){auto estimate=pipeline.pitchEstimate();pitchLog<<offset+std::min(127,n-offset)<<','<<estimate.hz<<','<<estimate.confidence<<','<<estimate.locked<<','<<pipeline.fmGate()<<'\n';}}
 double sourceEnergy=0,wetEnergy=0,peak=0;for(int c=0;c<channels;++c)for(int i=0;i<n;++i){double d=dry.getSample(c,i),w=audio.getSample(c,i);sourceEnergy+=d*d;wetEnergy+=w*w;peak=std::max(peak,std::abs(w));}
 const double rms=std::sqrt(wetEnergy/(n*channels)),match=std::sqrt(sourceEnergy/std::max(1.e-30,wetEnergy));double difference=0;
 for(int c=0;c<channels;++c)for(int i=0;i<n;++i){double e=dry.getSample(c,i)-audio.getSample(c,i)*match;difference+=e*e;}
 const bool normalize=argc>5&&std::atoi(argv[5])!=0;const double trim=normalize?std::min(.1/std::max(rms,1.e-20),micro::gain(-1)/std::max(peak,1.e-20)):1.;
 audio.applyGain(float(trim));output.getParentDirectory().createDirectory();juce::WavAudioFormat wav;std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(new juce::FileOutputStream(output),fs,unsigned(channels),24,{},0));
 if(!writer||!writer->writeFromAudioSampleBuffer(audio,0,n)){std::cerr<<"Write failed\n";return 1;}
 std::cout<<std::setprecision(8)<<"drive="<<p.drive<<" fm="<<p.fmAmount<<" depth="<<p.fmDepth<<" ratio="<<p.fmRatio+1<<" mode="<<p.mode<<" dry="<<p.bypass<<" source_rms_dbfs="<<10*std::log10(sourceEnergy/(n*channels))<<" wet_rms_dbfs="<<20*std::log10(rms)<<" matched_difference_db="<<10*std::log10(difference/std::max(sourceEnergy,1.e-30))<<" audition_trim_db="<<20*std::log10(trim)<<" final_peak_dbfs="<<20*std::log10(peak*trim)<<"\n";
 return 0;
}
