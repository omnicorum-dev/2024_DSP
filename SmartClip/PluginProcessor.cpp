/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
OmniSmartClipAudioProcessor::OmniSmartClipAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    drive = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("Drive"));
    
    preserve = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("Preserve"));
    
    LP.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    HP.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
}

OmniSmartClipAudioProcessor::~OmniSmartClipAudioProcessor()
{
}

//==============================================================================
const juce::String OmniSmartClipAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool OmniSmartClipAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool OmniSmartClipAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool OmniSmartClipAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double OmniSmartClipAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int OmniSmartClipAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int OmniSmartClipAudioProcessor::getCurrentProgram()
{
    return 0;
}

void OmniSmartClipAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String OmniSmartClipAudioProcessor::getProgramName (int index)
{
    return {};
}

void OmniSmartClipAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void OmniSmartClipAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    spec.sampleRate = sampleRate;
    
    compressor.prepare(spec);
    
    LP.prepare(spec);
    HP.prepare(spec);
    
    inputGain.prepare(spec);
    compressorInputGain.prepare(spec);
    compressorOutputGain.prepare(spec);
    
    inputGain.setRampDurationSeconds(0.05);
    compressorInputGain.setRampDurationSeconds(0.05);
    compressorOutputGain.setRampDurationSeconds(0.05);
    
    for (auto& buffer : filterBuffers)
    {
        buffer.setSize(spec.numChannels, samplesPerBlock);
    }
}

void OmniSmartClipAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool OmniSmartClipAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void OmniSmartClipAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    // CUSTOM CODE

        // gets values from the parameters
        float preserveParam = preserve->get();
        float driveParam = drive->get();

        // sets the compressor parameters
        compressor.setRelease(30);
        compressor.setAttack(0);
        compressor.setRatio(100);
        compressor.setThreshold(remap(preserveParam, 0, 127, 0.00, -4.00));

        // sets all gain settings
        inputGain.setGainDecibels(driveParam);
        compressorInputGain.setGainDecibels(remap(preserveParam, 0, 127, -20.0, 0));
        compressorOutputGain.setGainDecibels(remap(preserveParam, 0, 127, 19.5, 0));

        applyGain(buffer, inputGain);

        // creates audio block and context of input signal
        auto block = juce::dsp::AudioBlock<float>(buffer);
        auto context = juce::dsp::ProcessContextReplacing<float>(block);

        // prepares the array of buffers for the filters
        for (auto& fb : filterBuffers)
        {
            fb = buffer;
        }

        // sets filter cutoff
        auto cutoff = 140;
        LP.setCutoffFrequency(cutoff);
        HP.setCutoffFrequency(cutoff);

        // sets up blocks and context for each filter
        auto fb0Block = juce::dsp::AudioBlock<float>(filterBuffers[0]);
        auto fb1Block = juce::dsp::AudioBlock<float>(filterBuffers[1]);

        auto fb0Ctx = juce::dsp::ProcessContextReplacing<float>(fb0Block);
        auto fb1Ctx = juce::dsp::ProcessContextReplacing<float>(fb1Block);

        // processes each context for high and low pass
        LP.process(fb0Ctx);
        HP.process(fb1Ctx);

        // applies low band compressor input gain
        applyGain(filterBuffers[0], compressorInputGain);

        /* fb0Block = juce::dsp::AudioBlock<float>(filterBuffers[0]);
        fb1Block = juce::dsp::AudioBlock<float>(filterBuffers[1]);

        auto fb0Ctx = juce::dsp::ProcessContextReplacing<float>(fb0Block);
        auto fb1Ctx = juce::dsp::ProcessContextReplacing<float>(fb1Block); */

        // compresses the low band
        compressor.process(fb0Ctx);

        // applies low band output gain
        applyGain(filterBuffers[0], compressorOutputGain);

        // sets variables for sample number and channel numbers
        auto numSamples = buffer.getNumSamples();
        auto numChannels = buffer.getNumChannels();

        buffer.clear();

        // sums the two filters back together
        auto addFilterBand = [nc = numChannels, ns = numSamples](auto& inputBuffer, const auto& source)
        {
            for (auto i = 0; i < nc; ++i)
            {
                inputBuffer.addFrom(i, 0, source, i, 0, ns);
            }
        };

        addFilterBand(buffer, filterBuffers[0]);
        addFilterBand(buffer, filterBuffers[1]);


        // anologue cliper (3rd power)
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);

            for (int sample = 0; sample < numSamples; ++sample)
            {
                // creates temporary calculation variable
                double tmp = 0.0f;
                tmp = channelData[sample];

                float thresh = 3.0f / 2.0f;

                // the function itself
                if (tmp > thresh) {
                    tmp = 1;
                }
                else if (tmp < -thresh) {
                    tmp = -1;
                }
                else if (tmp >= -thresh && tmp <= thresh) {
                    tmp = tmp - (4.0 / 27.0) * pow(tmp, 3);
                }
                else {
                    tmp = 0;
                }

                buffer.setSample(channel, sample, tmp);

            }

        }
}

//==============================================================================
bool OmniSmartClipAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* OmniSmartClipAudioProcessor::createEditor()
{
    //return new OmniSmartClipAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void OmniSmartClipAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void OmniSmartClipAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

float OmniSmartClipAudioProcessor::remap(float value, float start1, float end1, float start2, float end2) {
    float outgoing = start2 + (end2 - start2) * ((value - start1) / (end1 - start1));
    return outgoing;
}

float OmniSmartClipAudioProcessor::analogClip(float input) {
    if (input > 1)
        return 1;
    if (input < -1)
        return -1;
    return (((3 * input) - (input * input * input)) / 2);
}

juce::AudioProcessorValueTreeState::ParameterLayout OmniSmartClipAudioProcessor::createParameterLayout() {
    APVTS::ParameterLayout layout;
    
    using namespace juce;
    
    layout.add(std::make_unique<AudioParameterFloat>("Drive",
                                                    "Drive",
                                                    NormalisableRange<float>(0, 16, 0.01f, 1),
                                                    0));
    
    layout.add(std::make_unique<AudioParameterFloat>("Preserve",
                                                     "Preserve",
                                                     NormalisableRange<float>(0, 127, 1, 1),
                                                     0));
    
    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OmniSmartClipAudioProcessor();
}
