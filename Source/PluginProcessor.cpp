#include "PluginProcessor.h"
#include "PluginEditor.h"

static juce::String U (const char* s) { return juce::String::fromUTF8 (s); }

static juce::StringArray pitchNames()
{
    juce::StringArray a;
    for (auto* s : { "2 тона вниз", "1,5 тона вниз", "тон вниз", "полтона вниз", "без сдвига", "полтона вверх", "тон вверх", "1,5 тона вверх", "2 тона вверх" }) a.add (U (s));
    return a;
}

WodniiDirtyVoiceAudioProcessor::WodniiDirtyVoiceAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout()) {}

juce::AudioProcessorValueTreeState::ParameterLayout WodniiDirtyVoiceAudioProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<AudioParameterChoice> (ParameterID{"key",1}, U("Тональность"), StringArray{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}, 9));
    l.add (std::make_unique<AudioParameterChoice> (ParameterID{"scale",1}, U("Лад"), StringArray{U("Минор"),U("Мажор"),U("Все ноты")}, 0));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID{"speed",1}, U("Скорость автотюна"), 0.0f, 100.0f, 100.0f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID{"dirt",1}, U("Грязь"), 0.0f, 100.0f, 40.0f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID{"dark",1}, U("Темнота"), 0.0f, 100.0f, 50.0f));
    const int defPitch[4] { 2, 3, 5, 6 };
    const float defPan[4] { -0.75f, -0.35f, 0.35f, 0.75f };
    for (int i = 0; i < 4; ++i)
    {
        auto n = String (i + 1);
        l.add (std::make_unique<AudioParameterChoice> (ParameterID{"dbl" + n + "pitch",1}, U("Дабл ") + n + U(" питч"), pitchNames(), defPitch[i]));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID{"dbl" + n + "delay",1}, U("Дабл ") + n + U(" задержка мс"), 0.0f, 60.0f, 10.0f + (float) i * 8.0f));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID{"dbl" + n + "pan",1}, U("Дабл ") + n + U(" панорама"), -1.0f, 1.0f, defPan[i]));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID{"dbl" + n + "level",1}, U("Дабл ") + n + U(" громкость"), 0.0f, 100.0f, 50.0f));
    }
    l.add (std::make_unique<AudioParameterFloat> (ParameterID{"mix",1}, U("Микс"), 0.0f, 100.0f, 100.0f));
    l.add (std::make_unique<AudioParameterBool> (ParameterID{"master",1}, U("Мастер"), false));
    return l;
}

bool WodniiDirtyVoiceAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && l.getMainInputChannelSet() == l.getMainOutputChannelSet();
}

void WodniiDirtyVoiceAudioProcessor::prepareToPlay (double sampleRate, int blockSize)
{
    sr = sampleRate;
    analysis.assign (2048, 0.0f); analysisPos = 0;
    const int shiftSize = (int) (sr * 0.04);
    tuneShifter.prepare (shiftSize);
    juce::dsp::ProcessSpec spec { sr, (juce::uint32) blockSize, 1 };
    for (int i = 0; i < numDoubles; ++i)
    {
        doubleShifters[i].prepare (shiftSize + i * 37);
        doubleDelays[i].setMaximumDelayInSamples ((int) (sr * 0.1));
        doubleDelays[i].prepare (spec);
    }
    for (int c = 0; c < 2; ++c)
    {
        lowCut[c].coefficients  = juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, 120.0f);
        midBoost[c].coefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter (sr, 1600.0f, 0.8f, 2.0f);
        highCut[c].coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, 12000.0f);
        lowCut[c].reset(); midBoost[c].reset(); highCut[c].reset();
    }
}

float WodniiDirtyVoiceAudioProcessor::Shifter::process (float in, float ratio)
{
    buf[(size_t) write] = in;
    phase += 1.0f - ratio;
    while (phase < 0.0f) phase += (float) size;
    while (phase >= (float) size) phase -= (float) size;
    auto tap = [this] (float d)
    {
        float pos = (float) write - d; while (pos < 0.0f) pos += (float) size;
        int i0 = (int) pos % size; int i1 = (i0 + 1) % size; float fr = pos - std::floor (pos);
        return buf[(size_t) i0] * (1.0f - fr) + buf[(size_t) i1] * fr;
    };
    float d1 = phase, d2 = std::fmod (phase + (float) size * 0.5f, (float) size);
    float g1 = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * d1 / (float) size);
    float g2 = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * d2 / (float) size);
    write = (write + 1) % size;
    return tap (d1) * g1 + tap (d2) * g2;
}

float WodniiDirtyVoiceAudioProcessor::detectPitch()
{
    const int n = (int) analysis.size();
    const int minLag = (int) (sr / 1000.0), maxLag = juce::jmin (n / 2, (int) (sr / 70.0));
    float energy = 0.0f;
    for (auto s : analysis) energy += s * s;
    if (energy < 0.01f) return 0.0f;
    int bestLag = 0; float best = 0.0f;
    for (int lag = minLag; lag < maxLag; ++lag)
    {
        float sum = 0.0f;
        for (int i = 0; i < n - lag; ++i)
            sum += analysis[(size_t) ((analysisPos + i) % n)] * analysis[(size_t) ((analysisPos + i + lag) % n)];
        if (sum > best) { best = sum; bestLag = lag; }
    }
    if (bestLag == 0 || best < energy * 0.3f) return 0.0f;
    return (float) (sr / bestLag);
}

float WodniiDirtyVoiceAudioProcessor::snapToScale (float freq, int key, int scale)
{
    static const int minorS[7] { 0,2,3,5,7,8,10 }, majorS[7] { 0,2,4,5,7,9,11 };
    float midi = 69.0f + 12.0f * std::log2 (freq / 440.0f);
    float bestNote = std::round (midi), bestDist = 100.0f;
    if (scale < 2)
    {
        const int* s = scale == 0 ? minorS : majorS;
        for (int oct = -1; oct <= 1; ++oct)
            for (int k = 0; k < 7; ++k)
            {
                float note = (float) (12 * ((int) std::floor (midi / 12.0f) + oct) + key + s[k]);
                if (std::abs (note - midi) < bestDist) { bestDist = std::abs (note - midi); bestNote = note; }
            }
    }
    return 440.0f * std::pow (2.0f, (bestNote - 69.0f) / 12.0f);
}

void WodniiDirtyVoiceAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int num = buffer.getNumSamples();
    auto p = [this] (const juce::String& id) { return apvts.getRawParameterValue (id)->load(); };

    const bool master = p ("master") > 0.5f;
    const float dirt = p ("dirt") / 100.0f * (master ? 0.3f : 1.0f);
    const float mix = p ("mix") / 100.0f;
    const float cutoff = juce::jmap (p ("dark") / 100.0f, 18000.0f, 2500.0f);
    for (int c = 0; c < 2; ++c)
        *highCut[c].coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass (sr, juce::jmin (cutoff, (float) sr * 0.45f));

    auto* L = buffer.getWritePointer (0);
    auto* R = buffer.getWritePointer (1);
    const float drive = 1.0f + dirt * 20.0f;
    auto dirty = [&] (float x, int c)
    {
        float y = std::tanh (x * drive) / std::tanh (drive);
        y = lowCut[c].processSample (y);
        y = midBoost[c].processSample (y);
        return highCut[c].processSample (y);
    };

    if (master)
    {
        for (int i = 0; i < num; ++i)
        {
            L[i] = L[i] * (1.0f - mix) + dirty (L[i], 0) * mix;
            R[i] = R[i] * (1.0f - mix) + dirty (R[i], 1) * mix;
        }
        return;
    }

    for (int i = 0; i < num; ++i)
    {
        analysis[(size_t) analysisPos] = 0.5f * (L[i] + R[i]);
        analysisPos = (analysisPos + 1) % (int) analysis.size();
    }
    const float f = detectPitch();
    if (f > 0.0f) currentRatio = snapToScale (f, (int) p ("key"), (int) p ("scale")) / f;
    const float coef = juce::jmap (p ("speed") / 100.0f, 0.002f, 1.0f);

    float dblRatio[numDoubles], gL[numDoubles], gR[numDoubles];
    for (int d = 0; d < numDoubles; ++d)
    {
        auto n = juce::String (d + 1);
        int st = (int) p ("dbl" + n + "pitch") - 4;
        dblRatio[d] = std::pow (2.0f, (float) st / 12.0f);
        float pan = p ("dbl" + n + "pan");
        float lvl = p ("dbl" + n + "level") / 100.0f;
        gL[d] = lvl * std::cos ((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
        gR[d] = lvl * std::sin ((pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f);
        doubleDelays[d].setDelay ((float) (p ("dbl" + n + "delay") * 0.001 * sr));
    }

    for (int i = 0; i < num; ++i)
    {
        const float dryL = L[i], dryR = R[i];
        smoothedRatio += (currentRatio - smoothedRatio) * coef;
        float tuned = tuneShifter.process (0.5f * (dryL + dryR), smoothedRatio);
        float lead = dirty (tuned, 0);
        float outL = lead, outR = lead;
        for (int d = 0; d < numDoubles; ++d)
        {
            doubleDelays[d].pushSample (0, tuned);
            float v = doubleShifters[d].process (doubleDelays[d].popSample (0), dblRatio[d]);
            outL += v * gL[d]; outR += v * gR[d];
        }
        L[i] = dryL * (1.0f - mix) + outL * 0.6f * mix;
        R[i] = dryR * (1.0f - mix) + outR * 0.6f * mix;
    }
}

void WodniiDirtyVoiceAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}
void WodniiDirtyVoiceAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size)) apvts.replaceState (juce::ValueTree::fromXml (*xml));
}
juce::AudioProcessorEditor* WodniiDirtyVoiceAudioProcessor::createEditor() { return new MegaEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new WodniiDirtyVoiceAudioProcessor(); }
