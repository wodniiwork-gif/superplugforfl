#include "PluginEditor.h"

static juce::String U (const char* s) { return juce::String::fromUTF8 (s); }
static const juce::Colour purple (0xffb84dff), pink (0xffff5fae), bgDark (0xff0d0812), panelCol (0xcc17101f);

MegaLook::MegaLook()
{
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff1c1424));
    setColour (juce::ComboBox::outlineColourId, purple.withAlpha (0.6f));
    setColour (juce::ComboBox::textColourId, juce::Colours::white);
    setColour (juce::ComboBox::arrowColourId, juce::Colours::white);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1c1424));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, purple.withAlpha (0.5f));
    setColour (juce::Slider::textBoxTextColourId, juce::Colours::white);
    setColour (juce::Slider::textBoxOutlineColourId, purple.withAlpha (0.5f));
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff120c18));
}

void MegaLook::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider&)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.0f);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float angle = start + pos * (end - start);
    juce::PathStrokeType thin (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    juce::Path track; track.addCentredArc (c.x, c.y, r, r, 0.0f, start, end, true);
    g.setColour (juce::Colour (0xff2a2232)); g.strokePath (track, thin);
    juce::Path val; val.addCentredArc (c.x, c.y, r, r, 0.0f, start, angle, true);
    g.setColour (purple.withAlpha (0.35f)); g.strokePath (val, juce::PathStrokeType (10.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (purple); g.strokePath (val, thin);
    const float inner = r * 0.78f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3442), c.x, c.y - inner, juce::Colour (0xff141018), c.x, c.y + inner, false));
    g.fillEllipse (c.x - inner, c.y - inner, inner * 2, inner * 2);
    g.setColour (juce::Colours::white);
    g.drawLine (juce::Line<float> (c.getPointOnCircumference (inner * 0.3f, angle), c.getPointOnCircumference (inner * 0.85f, angle)), 3.0f);
}

void MegaLook::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (4.0f);
    const bool on = b.getToggleState();
    g.setColour (on ? purple : juce::Colour (0xff2a2232)); g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (purple.brighter()); g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 2.0f);
    const float d = r.getHeight() - 8.0f;
    g.setColour (juce::Colours::white);
    g.fillEllipse (on ? r.getRight() - d - 4.0f : r.getX() + 4.0f, r.getY() + 4.0f, d, d);
}

MegaEditor::MegaEditor (WodniiDirtyVoiceAudioProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&look);
    combo (key, "key"); combo (scale, "scale");
    knob (speed, "speed"); knob (dirt, "dirt"); knob (dark, "dark"); knob (mix, "mix");
    for (int i = 0; i < 4; ++i)
    {
        auto n = juce::String (i + 1);
        combo (dblPitch[i], "dbl" + n + "pitch");
        knob (dblDelay[i], "dbl" + n + "delay");
        knob (dblPan[i], "dbl" + n + "pan");
        knob (dblLevel[i], "dbl" + n + "level");
    }
    addAndMakeVisible (master);
    masterAtt = std::make_unique<BA> (proc.apvts, "master", master);
    setSize (1000, 640);
    startTimerHz (30);
}

MegaEditor::~MegaEditor() { stopTimer(); setLookAndFeel (nullptr); }

void MegaEditor::knob (juce::Slider& s, const juce::String& id)
{
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 20);
    s.setNumDecimalPlacesToDisplay (0);
    addAndMakeVisible (s);
    sliderAtt.push_back (std::make_unique<SA> (proc.apvts, id, s));
}

void MegaEditor::combo (juce::ComboBox& c, const juce::String& id)
{
    if (auto* ch = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (id)))
        c.addItemList (ch->choices, 1);
    addAndMakeVisible (c);
    comboAtt.push_back (std::make_unique<CA> (proc.apvts, id, c));
}

void MegaEditor::drawSkull (juce::Graphics& g, juce::Rectangle<float> r, bool horns)
{
    const float w = r.getWidth(), h = r.getHeight(), x = r.getX(), y = r.getY();
    if (horns)
    {
        juce::Path hl, hr;
        hl.addTriangle (x + w * 0.15f, y + h * 0.35f, x + w * 0.32f, y + h * 0.2f, x + w * 0.05f, y);
        hr.addTriangle (r.getRight() - w * 0.15f, y + h * 0.35f, r.getRight() - w * 0.32f, y + h * 0.2f, r.getRight() - w * 0.05f, y);
        g.setColour (purple); g.fillPath (hl); g.fillPath (hr);
    }
    g.setColour (pink);
    g.fillEllipse (x + w * 0.1f, y + h * 0.12f, w * 0.8f, h * 0.68f);
    g.fillRoundedRectangle (x + w * 0.3f, y + h * 0.62f, w * 0.4f, h * 0.3f, w * 0.06f);
    g.setColour (bgDark);
    g.fillEllipse (x + w * 0.24f, y + h * 0.38f, w * 0.2f, h * 0.2f);
    g.fillEllipse (x + w * 0.56f, y + h * 0.38f, w * 0.2f, h * 0.2f);
    juce::Path nose; nose.addTriangle (r.getCentreX(), y + h * 0.6f, r.getCentreX() - w * 0.05f, y + h * 0.7f, r.getCentreX() + w * 0.05f, y + h * 0.7f);
    g.fillPath (nose);
    for (int t = 0; t < 3; ++t) g.fillRect (x + w * (0.38f + (float) t * 0.09f), y + h * 0.8f, w * 0.03f, h * 0.1f);
}

void MegaEditor::panel (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title)
{
    auto f = r.toFloat();
    g.setColour (panelCol); g.fillRoundedRectangle (f, 10.0f);
    g.setColour (purple.withAlpha (0.55f)); g.drawRoundedRectangle (f, 10.0f, 1.5f);
    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (juce::FontOptions (20.0f, juce::Font::bold)));
    g.drawText (title, r.removeFromTop (40), juce::Justification::centred);
    g.setColour (purple); g.fillRect (f.getX() + 12.0f, f.getY() + 40.0f, f.getWidth() - 24.0f, 2.0f);
}

void MegaEditor::paint (juce::Graphics& g)
{
    const float a = phase * juce::MathConstants<float>::twoPi;
    const float cx = (float) getWidth() * 0.5f, cy = (float) getHeight() * 0.5f;
    juce::ColourGradient grad (juce::Colour (0xff3b0f5c), cx + std::cos (a) * cx, cy + std::sin (a) * cy,
                               bgDark, cx - std::cos (a) * cx, cy - std::sin (a) * cy, false);
    grad.addColour (0.5 + 0.15 * std::sin (a * 2.0f), juce::Colour (0xff1a0b26));
    g.setGradientFill (grad); g.fillAll();
    g.setColour (pink.withAlpha (0.08f + 0.06f * std::sin (a * 3.0f)));
    g.fillEllipse (cx - 400.0f, (float) getHeight() - 80.0f, 800.0f, 200.0f);

    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (juce::FontOptions (40.0f, juce::Font::bold)));
    g.drawText (U ("СУПЕР КРУТОЙ МЕГА ПЛАГИН"), 0, 10, getWidth(), 56, juce::Justification::centred);
    drawSkull (g, { 40.0f, 18.0f, 44.0f, 44.0f }, false);
    drawSkull (g, { (float) getWidth() - 90.0f, 10.0f, 56.0f, 56.0f }, true);

    panel (g, rTune, U ("АВТОТЮН")); panel (g, rDirt, U ("ГРЯЗЬ")); panel (g, rDark, U ("ТЕМНОТА"));
    panel (g, rDbl, U ("ДАБЛЫ")); panel (g, rMix, U ("МИКС"));

    g.setFont (juce::Font (juce::FontOptions (14.0f)));
    g.setColour (juce::Colours::white.withAlpha (0.8f));
    auto label = [&] (juce::Component& c, const char* t) { g.drawText (U (t), c.getBounds().translated (0, -16).withHeight (16), juce::Justification::centred); };
    for (int i = 0; i < 4; ++i)
    {
        g.drawText (U ("Дабл ") + juce::String (i + 1), dblPitch[i].getBounds().translated (0, -22).withHeight (20), juce::Justification::centred);
        label (dblDelay[i], "Задержка"); label (dblPan[i], "Панорама"); label (dblLevel[i], "Громкость");
    }
    label (speed, "Скорость"); label (dirt, "Дисторшн"); label (dark, "Фильтр");
    g.drawText (U ("МАСТЕР"), master.getBounds().translated (0, -24).withHeight (20), juce::Justification::centred);
}

void MegaEditor::resized()
{
    auto area = getLocalBounds().reduced (10).withTrimmedTop (70);
    rTune = area.removeFromLeft (190); area.removeFromLeft (8);
    rDirt = area.removeFromLeft (140); area.removeFromLeft (8);
    rDark = area.removeFromLeft (140); area.removeFromLeft (8);
    rMix  = area.removeFromRight (130); area.removeFromRight (8);
    rDbl  = area;

    auto t = rTune.reduced (14).withTrimmedTop (44);
    key.setBounds (t.removeFromTop (30)); t.removeFromTop (10);
    scale.setBounds (t.removeFromTop (30)); t.removeFromTop (30);
    speed.setBounds (t.removeFromTop (150));
    dirt.setBounds (rDirt.reduced (10).withTrimmedTop (120).withHeight (150));
    dark.setBounds (rDark.reduced (10).withTrimmedTop (120).withHeight (150));

    auto d = rDbl.reduced (8).withTrimmedTop (70);
    const int colW = d.getWidth() / 4;
    for (int i = 0; i < 4; ++i)
    {
        auto c = d.removeFromLeft (colW).reduced (4, 0);
        dblPitch[i].setBounds (c.removeFromTop (28)); c.removeFromTop (22);
        dblDelay[i].setBounds (c.removeFromTop (110)); c.removeFromTop (18);
        dblPan[i].setBounds (c.removeFromTop (110)); c.removeFromTop (18);
        dblLevel[i].setBounds (c.removeFromTop (110));
    }
    auto m = rMix.reduced (10).withTrimmedTop (60);
    mix.setBounds (m.removeFromTop (160)); m.removeFromTop (80);
    master.setBounds (m.removeFromTop (44).withSizeKeepingCentre (90, 40));
}
