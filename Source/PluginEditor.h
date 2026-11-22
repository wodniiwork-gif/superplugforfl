#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class MegaLook : public juce::LookAndFeel_V4
{
public:
    MegaLook();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
};

class MegaEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit MegaEditor (WodniiDirtyVoiceAudioProcessor&);
    ~MegaEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override { phase += 0.008f; if (phase > 1.0f) phase -= 1.0f; repaint(); }
    void knob (juce::Slider&, const juce::String& id);
    void combo (juce::ComboBox&, const juce::String& id);
    static void drawSkull (juce::Graphics&, juce::Rectangle<float>, bool horns);
    void panel (juce::Graphics&, juce::Rectangle<int>, const juce::String& title);

    WodniiDirtyVoiceAudioProcessor& proc;
    MegaLook look;
    float phase = 0.0f;
    juce::Slider speed, dirt, dark, mix;
    juce::ComboBox key, scale;
    juce::Slider dblDelay[4], dblPan[4], dblLevel[4];
    juce::ComboBox dblPitch[4];
    juce::ToggleButton master;
    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::vector<std::unique_ptr<SA>> sliderAtt;
    std::vector<std::unique_ptr<CA>> comboAtt;
    std::unique_ptr<BA> masterAtt;
    juce::Rectangle<int> rTune, rDirt, rDark, rDbl, rMix;
};
