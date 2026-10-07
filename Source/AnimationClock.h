#pragma once
#include <algorithm>
#include <cmath>
namespace micro {
// Source-time per wall-clock second. Higher Memory slows movement; Variation adds motion.
inline float animationSpeed(float memory,float variation) noexcept {
 return .50f-.35f*std::clamp(memory,0.f,1.f)+.25f*std::clamp(variation,0.f,1.f);
}
// 31 cached samples represent the supplied 121-frame clip at nominal 30 fps.
inline float animationFrame(float drive,double seconds,bool moving=true) noexcept {
 const float span=7.5f,last=30.f;double t=std::fmod(std::max(0.,seconds),2.);float triangle=float(t<=1?t:2-t);
 return std::clamp(drive,0.f,1.f)*(last-span)+(moving?triangle*span:0);
}
}
