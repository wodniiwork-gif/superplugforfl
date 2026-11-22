#pragma once
#include <JuceHeader.h>

class WodniiDirtyVoiceAudioProcessor : public juce::AudioProcessor
{
public:
    WodniiDirtyVoiceAudioProcessor();
    ~WodniiDirtyVoiceAudioProcessor() override = default;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Super Krutoy Mega Plagin"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.2; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    static constexpr int numDoubles = 4;
    double sr = 44100.0;
    std::vector<float> analysis;
    int analysisPos = 0;
    float currentRatio = 1.0f, smoothedRatio = 1.0f;

    struct Shifter
    {
        std::vector<float> buf; int write = 0; float phase = 0.0f; int size = 1;
        void prepare (int n) { size = n; buf.assign ((size_t) n, 0.0f); write = 0; phase = 0.0f; }
        float process (float in, float ratio);
    };
    Shifter tuneShifter;
    Shifter doubleShifters[numDoubles];
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> doubleDelays[numDoubles];
    juce::dsp::IIR::Filter<float> lowCut[2], highCut[2], midBoost[2];

    float detectPitch();
    float snapToScale (float freq, int key, int scale);
};
