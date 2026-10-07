#pragma once
#include "Processor.h"
class MetalLook final:public juce::LookAndFeel_V4 {
public:MetalLook();void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
};
class Lattice final:public juce::Component {
 struct Bead{float x,y,z;int tube;};std::vector<Bead> beads;juce::Path branches;
 MicroProcessor& processor;juce::Image artwork;std::shared_ptr<const std::vector<juce::Image>> frames;float phase=0,afterglow=0,pulse=0,animationPosition=0,activity=0;std::array<float,3> priorParameters{};double lastTick=0,loopSeconds=0;float drivePosition=0;int lastEvent=0;
 juce::Image blendedFrame;float playbackSpeed=.36f,audioLight=0,noiseTime=0;
 static constexpr int noiseWidth=49,noiseHeight=19;
 std::array<float,noiseWidth*noiseHeight> noiseField{};
 unsigned renderRevision=1,lastRenderRevision=0;
public:
 explicit Lattice(MicroProcessor&);void tick();void paint(juce::Graphics&) override;
 void drawArtwork(juce::Graphics&);
 void drawLayer(juce::Graphics&,int tube,bool connections=false);
};
class MicroEditor final:public juce::AudioProcessorEditor,private juce::Timer {
 MicroProcessor& processor;MetalLook look;Lattice lattice;
 std::array<juce::Slider,14> knobs;std::array<juce::Label,14> labels;
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> sliders;
 juce::ComboBox mode,quality,preset,source,preLow,preHigh,fmRatio;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeLink,qualityLink,preLowLink,preHighLink,fmRatioLink;
 juce::TextButton freeze{"FREEZE"},bypass{"BYPASS"},match{"MATCH 2s"},explore{"EXPLORE"},resetButton{"RESET"},advanced{"ENTROPY / ADVANCED"},importButton{"IMPORT JSON"},seedButton{"APPLY SEED"};
 juce::ToggleButton motion{"Animate lattice"},ablate{"Ablation: disable memory + coupling"};
 std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> buttons;
 juce::TextEditor seed;juce::Label status,detail,fmStatus,fmHelp;juce::TooltipWindow tips{this,600};
 std::unique_ptr<juce::FileChooser> chooser;bool expanded=false;juce::Image texture;float ventLight=0,ventTime=0;double lastVisualTick=0;
 juce::AffineTransform contentTransform() const;void timerCallback() override;void drawFrame(juce::Graphics&);void doImport();
public:
 explicit MicroEditor(MicroProcessor&);~MicroEditor() override;
 void paint(juce::Graphics&) override;void resized() override;
 void exportLayers(const juce::File&);
 juce::Image captureLattice(){return lattice.createComponentSnapshot(lattice.getLocalBounds(),true,1);}
};
