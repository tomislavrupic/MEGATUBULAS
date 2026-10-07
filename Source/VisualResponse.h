#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace micro {
inline std::uint8_t mixVisualByte(unsigned a,unsigned b,unsigned weight) noexcept {
 return std::uint8_t((a*(256-weight)+b*weight+128)>>8);
}
inline float visualHash(int x,int y) noexcept {
 std::uint32_t h=std::uint32_t(x)*0x8da6b343u^std::uint32_t(y)*0xd8163841u;
 h^=h>>16;h*=0x7feb352du;h^=h>>15;h*=0x846ca68bu;h^=h>>16;
 return float(h>>8)/16777215.f;
}
inline float visualNoise(float x,float y) noexcept {
 const int ix=int(std::floor(x)),iy=int(std::floor(y));float u=x-float(ix),v=y-float(iy);
 u=u*u*(3-2*u);v=v*v*(3-2*v);
 const float a=visualHash(ix,iy),b=visualHash(ix+1,iy),c=visualHash(ix,iy+1),d=visualHash(ix+1,iy+1);
 return (a+(b-a)*u)*(1-v)+(c+(d-c)*u)*v;
}
inline float organicVisualNoise(float x,float y,float time) noexcept {
 return .57f*visualNoise(x+time*.17f,y-time*.11f)
       +.29f*visualNoise(x*2.03f-time*.13f+17,y*2.03f+time*.09f+31)
       +.14f*visualNoise(x*4.07f+time*.08f+53,y*4.07f-time*.07f+11);
}
}
