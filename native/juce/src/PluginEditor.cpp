#include "PluginEditor.h"

namespace {
constexpr auto paramIdDrive = "drive";
constexpr auto paramIdBias = "bias";
constexpr auto paramIdEnvDrive = "env_to_drive";
constexpr auto paramIdCurve = "curve_index";
constexpr auto paramIdChaos = "chaos_level";
constexpr auto paramIdGateComp = "gate_comp";
constexpr auto paramIdPreTilt = "pre_tilt_db_per_oct";
constexpr auto paramIdPostAir = "post_air_gain_db";
constexpr auto paramIdDirt = "dirt_amount";
constexpr auto paramIdMix = "mix";
constexpr auto paramIdCeiling = "limiter_ceiling_db";
constexpr auto paramIdOutput = "out_trim_db";
}

DustPressAudioProcessorEditor::DustPressAudioProcessorEditor(DustPressAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p) {
  addKnob(drive, "Drive", paramIdDrive);
  addKnob(bias, "Bias", paramIdBias, true);
  addKnob(envToDrive, "Env→Drive", paramIdEnvDrive, true);

  addChoice(curve, "Curve", paramIdCurve, {"Tanh", "Cubic", "Diode", "Fold"});
  addChoice(chaos, "Chaos", paramIdChaos, {"0", "1", "2", "3", "4", "5", "6", "7"});

  addKnob(gateComp, "Gate/Comp", paramIdGateComp);
  addKnob(preTilt, "Pre Tilt", paramIdPreTilt, true);
  addKnob(postAir, "Post Air", paramIdPostAir, true);

  addKnob(dirt, "Dirt", paramIdDirt);
  addKnob(mix, "Mix", paramIdMix);
  addKnob(ceiling, "Ceiling", paramIdCeiling);
  addKnob(output, "Output", paramIdOutput);

  setSize(760, 400);
}

void DustPressAudioProcessorEditor::paint(juce::Graphics& g) {
  g.fillAll(juce::Colours::black);
  g.setColour(juce::Colours::darkorange);
  g.setFont(juce::Font(18.0f, juce::Font::bold));
  g.drawFittedText("Dust Press — control map mirror", getLocalBounds().reduced(10, 6).removeFromTop(30),
                   juce::Justification::centred, 1);
  g.setColour(juce::Colours::darkgrey);
  g.drawRect(getLocalBounds().reduced(5));
}

void DustPressAudioProcessorEditor::resized() {
  auto area = getLocalBounds().reduced(12);
  area.removeFromTop(36);

  auto rowHeight = 120;
  auto row1 = area.removeFromTop(rowHeight);
  layoutRow(row1, {&drive, &bias, &envToDrive});

  auto row2 = area.removeFromTop(rowHeight);
  layoutRow(row2, {&curve, &chaos, &gateComp, &preTilt, &postAir});

  auto row3 = area.removeFromTop(rowHeight);
  layoutRow(row3, {&dirt, &mix, &ceiling, &output});
}

void DustPressAudioProcessorEditor::addKnob(Knob& target, const juce::String& name, const juce::String& paramId,
                                           bool bipolar) {
  target.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  target.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 68, 20);
  target.slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::darkorange);
  target.slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::darkgrey);
  if (auto* param = processor.parameters.getParameter(paramId))
    target.slider.setDoubleClickReturnValue(true, param->convertFrom0to1(param->getDefaultValue()));
  if (bipolar) target.slider.setSkewFactorFromMidPoint(0.0f);

  target.label.setText(name, juce::dontSendNotification);
  target.label.setJustificationType(juce::Justification::centred);
  target.label.setColour(juce::Label::textColourId, juce::Colours::white);

  target.usesCombo = false;
  addAndMakeVisible(target.slider);
  addAndMakeVisible(target.label);

  target.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.parameters, paramId,
                                                                                             target.slider);
}

void DustPressAudioProcessorEditor::addChoice(Knob& target, const juce::String& name, const juce::String& paramId,
                                             const juce::StringArray& options) {
  target.combo.addItemList(options, 1);
  target.combo.setJustificationType(juce::Justification::centred);
  target.combo.setColour(juce::ComboBox::backgroundColourId, juce::Colours::black.withAlpha(0.6f));
  target.combo.setColour(juce::ComboBox::outlineColourId, juce::Colours::darkgrey);
  target.combo.setColour(juce::ComboBox::textColourId, juce::Colours::white);

  target.label.setText(name, juce::dontSendNotification);
  target.label.setJustificationType(juce::Justification::centred);
  target.label.setColour(juce::Label::textColourId, juce::Colours::white);

  target.usesCombo = true;
  addAndMakeVisible(target.combo);
  addAndMakeVisible(target.label);

  target.comboAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(processor.parameters,
                                                                                                    paramId, target.combo);
}

void DustPressAudioProcessorEditor::layoutRow(juce::Rectangle<int> area, std::initializer_list<Knob*> knobs) {
  auto knobWidth = area.getWidth() / static_cast<int>(knobs.size());
  for (auto* knob : knobs) {
    auto knobArea = area.removeFromLeft(knobWidth).reduced(8);
    auto labelArea = knobArea.removeFromTop(20);
    knob->label.setBounds(labelArea);
    knob->component().setBounds(knobArea);
  }
}
