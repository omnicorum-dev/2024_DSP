/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
_427AudioProcessor::_427AudioProcessor()
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
    
    exponentiation = dynamic_cast<juce::AudioParameterInt*>(apvts.getParameter("Exponentiation"));
}

_427AudioProcessor::~_427AudioProcessor()
{
}

//==============================================================================
const juce::String _427AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool _427AudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool _427AudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool _427AudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double _427AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int _427AudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int _427AudioProcessor::getCurrentProgram()
{
    return 0;
}

void _427AudioProcessor::setCurrentProgram (int index)
{
}

const juce::String _427AudioProcessor::getProgramName (int index)
{
    return {};
}

void _427AudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void _427AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    spec.sampleRate = sampleRate;
    
    inputDrive.prepare(spec);
    
    inputDrive.setRampDurationSeconds(0.05);
}

void _427AudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool _427AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void _427AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    // MY SHIT pasihdfosihdfosidhfsoihsodifh
    
    auto numSamples = buffer.getNumSamples();
    
    float driveParam = drive->get();
    int exponentiationParam = exponentiation->get();
    
    inputDrive.setGainDecibels(driveParam);
    applyGain(buffer, inputDrive);
    
    double n = 8.0 * ((exponentiationParam + 13) / 100.0);

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        for (int sample = 0; sample < numSamples; ++sample) {
            double tmp = 0.0;
            tmp = channelData[sample];
            
            if (tmp > (n / (n - 1))) {
                tmp = 1;
            }
            else if (tmp < -(n / (n - 1))) {
                tmp = -1;
            }
            else if (tmp >= 0 && tmp <= (n / (n - 1))) {
                tmp = (tmp - ((pow(n - 1, n - 1)) / pow(n, n)) * pow(tmp, n));
            }
            else if (tmp <= 0 && tmp >= -(n / (n - 1))) {
                tmp = (tmp + ((pow(n - 1, n - 1)) / pow(n, n)) * pow(-tmp, n));
            }
            else {
                tmp = 0;
            }
            
            buffer.setSample(channel, sample, tmp);
        }
    }
}

//==============================================================================
bool _427AudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* _427AudioProcessor::createEditor()
{
    //return new _427AudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void _427AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void _427AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

juce::AudioProcessorValueTreeState::ParameterLayout _427AudioProcessor::createParameterLayout() {
    APVTS::ParameterLayout layout;
    
    using namespace juce;
    
    layout.add(std::make_unique<AudioParameterFloat>("Drive",
                                                    "Drive",
                                                    NormalisableRange<float>(0, 24, 0.01f, 1),
                                                    0));
    
    layout.add(std::make_unique<AudioParameterInt>("Exponentiation",
                                                   "Exponentiation",
                                                   0,
                                                   100,
                                                   50));
    
    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new _427AudioProcessor();
}
