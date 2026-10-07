#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Pipeline.h"
#include <mutex>
static_assert(std::atomic<float>::is_always_lock_free && std::atomic<unsigned>::is_always_lock_free);
class MicroProcessor final : public juce::AudioProcessor {
public:
 MicroProcessor();
 juce::AudioProcessorValueTreeState state;
 micro::Pipeline pipeline;
 std::atomic<float> inMeter{0},outMeter{0},memoryMeter{0},matchDb{0},fmHz{0},fmConfidence{0},fmGate{0};
 std::atomic<int> fmTrackingState{0};
 std::atomic<int> selection{3},eventCount{0},entropyPosition{0},entropyExhausted{0},matchState{0},activeQuality{4};
 std::atomic<bool> resetRequested{false},matchRequested{false},exploreRequested{false};
 std::array<std::atomic<float>,6> selectedVector{};
 std::atomic<int> loadErrors{0};
 const juce::String getName() const override{return "MEGATUBULAS";}
 void prepareToPlay(double,int) override;void releaseResources() override{}
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 bool isBusesLayoutSupported(const BusesLayout&) const override;
 bool hasEditor() const override{return true;}juce::AudioProcessorEditor* createEditor() override;
 bool acceptsMidi() const override{return false;}bool producesMidi() const override{return false;}bool isMidiEffect() const override{return false;}
 double getTailLengthSeconds() const override{return 3.;}
 int getNumPrograms() override{return 5;}int getCurrentProgram() override{return program.load();}
 void setCurrentProgram(int) override;const juce::String getProgramName(int) override;void changeProgramName(int,const juce::String&) override{}
 void getStateInformation(juce::MemoryBlock&) override;void setStateInformation(const void*,int) override;
 juce::AudioProcessorParameter* getBypassParameter() const override{return state.getParameter("bypass");}
 void reset() override {resetRequested=true;}
 bool importEntropy(const juce::String&,juce::String&);bool setEntropy(const micro::EntropyConfig&);micro::EntropyConfig entropyConfig() const;
 static juce::AudioProcessorValueTreeState::ParameterLayout layout();
 micro::Parameters parameters() const;
private:
 struct Slot {micro::EntropyConfig config;std::atomic<int> status{0};};std::array<Slot,4> queue;
 std::atomic<unsigned> producer{0},consumer{0};mutable std::mutex configMutex;micro::EntropyConfig savedConfig;
 std::atomic<int> program{0};bool wasOffline=false;
 std::array<std::atomic<float>*,23> values{};
 bool enqueue(const micro::EntropyConfig&);void consume() noexcept;
};
