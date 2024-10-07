/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class OmniSmartClipAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    OmniSmartClipAudioProcessor();
    ~OmniSmartClipAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    //odfjsifjosdjfoijfosijdf
    
    float remap (float value, float start1, float end1, float start2, float end2);
    float analogClip(float input);
    
    // VALUE TREE STATE
    using APVTS = juce::AudioProcessorValueTreeState;
    static APVTS::ParameterLayout createParameterLayout();
    
    APVTS apvts{ *this, nullptr, "Parameters", createParameterLayout() };

private:
    
    juce::dsp::Compressor<float> compressor;
    
    using Filter = juce::dsp::LinkwitzRileyFilter<float>;
    Filter LP, HP;
    
    juce::AudioParameterFloat* drive { nullptr };
    juce::AudioParameterFloat* preserve { nullptr };
    
    std::array<juce::AudioBuffer<float>, 2> filterBuffers;
    
    juce::dsp::Gain<float> inputGain, compressorInputGain, compressorOutputGain;
    
    template<typename T, typename U>
    void applyGain(T& buffer, U& gain)
    {
        auto block = juce::dsp::AudioBlock<float>(buffer);
        auto ctx = juce::dsp::ProcessContextReplacing<float>(block);
        gain.process(ctx);
    }
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OmniSmartClipAudioProcessor)
};
