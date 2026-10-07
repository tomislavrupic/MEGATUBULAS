#include "Pipeline.h"
#include <chrono>
#include <iostream>
#include <algorithm>
int main(){
 std::cout<<"factor,block,fm,mean_us,p99_us,max_us,deadline_us\n";
 for(int factor:{4,8})for(int block:{1,17,127,512,4096})for(int enabled:{0,1}){
  micro::Pipeline pipeline;micro::Parameters p;p.fmAmount=enabled?100:0;p.fmDepth=100;p.fmRatio=2;pipeline.set(p);pipeline.prepare(48000,block,2,factor);
  std::vector<float> left(size_t(block),0.f),right(size_t(block),0.f);float* data[]{left.data(),right.data()};std::vector<double> times;times.reserve(size_t(48000/block+1));
  int sample=0;for(int pass=0;pass<2;++pass)for(int n=0;n<48000;n+=block){for(int i=0;i<block;++i){left[size_t(i)]=float(.4*std::sin(2*micro::pi*55*sample++/48000));right[size_t(i)]=-left[size_t(i)];}auto start=std::chrono::steady_clock::now();pipeline.process(data,block);auto end=std::chrono::steady_clock::now();if(pass)times.push_back(std::chrono::duration<double,std::micro>(end-start).count());}
  double total=0;for(auto t:times)total+=t;std::sort(times.begin(),times.end());std::cout<<factor<<','<<block<<','<<enabled<<','<<total/times.size()<<','<<times[size_t(.99*(times.size()-1))]<<','<<times.back()<<','<<block*1.e6/48000<<'\n';
 }
}
