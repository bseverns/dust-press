#include "DustPressAudioProcessorEditor.h"

namespace {
constexpr auto kMargin = 14;
constexpr auto kRowHeight = 120;
constexpr auto kHeaderHeight = 46;
constexpr auto kMaxColumns = 4;
constexpr auto kColumnWidth = 242;
constexpr auto kTotalWidth = 2 * kMargin + kMaxColumns * kColumnWidth;
constexpr auto kTotalHeight = kHeaderHeight + 4 * kRowHeight + 2 * kMargin;
}

DustPressAudioProcessorEditor::DustPressAudioProcessorEditor(DustPressAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p), state(p.getValueTreeState()) {
  addSlider(drive, "Drive (dB)", DustPressParamIDs::driveDb, " dB");
  addSlider(bias, "Bias", DustPressParamIDs::bias, {}, true);
  addSlider(envToDrive, "Env→Drive (dB)", DustPressParamIDs::envToDrive, " dB", true);

  addChoice(curve, "Curve", DustPressParamIDs::curve, {"tanh", "cubic", "diode", "fold"});
  addChoice(chaos, "Chaos", DustPressParamIDs::chaos,
            {"0 - clean", "1", "2", "3", "4", "5", "6", "7"});

  addSlider(gateComp, "Gate/Comp", DustPressParamIDs::gateComp);
  addSlider(preTilt, "Tilt (dB/oct)", DustPressParamIDs::preTilt, " dB/oct", true);
  addSlider(postAir, "Air (dB)", DustPressParamIDs::postAir, " dB", true);

  addSlider(dirt, "Dirt", DustPressParamIDs::dirt);
  addSlider(mix, "Mix (%)", DustPressParamIDs::mixPercent, " %");
  addSlider(ceiling, "Ceiling (dBFS)", DustPressParamIDs::ceiling, " dBFS");
  addSlider(output, "Output (dB)", DustPressParamIDs::outputTrim, " dB");

  setSize(kTotalWidth, kTotalHeight);
  setResizable(true, true);
  setResizeLimits(kTotalWidth, kTotalHeight, kTotalWidth * 2, kTotalHeight * 2);
}

void DustPressAudioProcessorEditor::paint(juce::Graphics& g) {
  g.fillAll(juce::Colours::black);

  g.setColour(juce::Colours::darkgrey);
  g.drawRect(getLocalBounds().reduced(6));

  auto titleArea = getLocalBounds().reduced(kMargin).removeFromTop(kHeaderHeight);
  g.setColour(juce::Colours::darkorange);
  g.setFont(juce::Font(20.0f, juce::Font::bold));
  g.drawFittedText("Dust Press — hardware map, host automation-ready", titleArea, juce::Justification::centred, 1);
}

void DustPressAudioProcessorEditor::resized() {
  auto area = getLocalBounds().reduced(kMargin);
  area.removeFromTop(kHeaderHeight);

  auto rowHeight = area.getHeight() / 4;

  layoutRow(area.removeFromTop(rowHeight), {&drive, &bias, &envToDrive});
  layoutRow(area.removeFromTop(rowHeight), {&curve, &chaos});
  layoutRow(area.removeFromTop(rowHeight), {&gateComp, &preTilt, &postAir});
  layoutRow(area.removeFromTop(rowHeight), {&dirt, &mix, &ceiling, &output});
}

void DustPressAudioProcessorEditor::addSlider(Control& control, const juce::String& name,
                                              const juce::String& paramId, const juce::String& suffix,
                                              bool centerOnZero) {
  control.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  control.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 74, 22);
  control.slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::darkorange);
  control.slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::darkgrey);
  control.slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
  control.slider.setTextValueSuffix(suffix);
  if (auto* param = state.getParameter(paramId)) {
    control.slider.setDoubleClickReturnValue(true, param->convertFrom0to1(param->getDefaultValue()));
  }
  if (centerOnZero) control.slider.setSkewFactorFromMidPoint(0.0f);

  control.label.setText(name, juce::dontSendNotification);
  control.label.setJustificationType(juce::Justification::centred);
  control.label.setColour(juce::Label::textColourId, juce::Colours::white);

  control.usesCombo = false;
  addAndMakeVisible(control.slider);
  addAndMakeVisible(control.label);

  control.sliderAttachment = std::make_unique<SliderAttachment>(state, paramId, control.slider);
}

void DustPressAudioProcessorEditor::addChoice(Control& control, const juce::String& name,
                                              const juce::String& paramId, const juce::StringArray& options) {
  control.combo.addItemList(options, 1);
  control.combo.setJustificationType(juce::Justification::centred);
  control.combo.setColour(juce::ComboBox::backgroundColourId, juce::Colours::black.withAlpha(0.7f));
  control.combo.setColour(juce::ComboBox::outlineColourId, juce::Colours::darkgrey);
  control.combo.setColour(juce::ComboBox::textColourId, juce::Colours::white);

  control.label.setText(name, juce::dontSendNotification);
  control.label.setJustificationType(juce::Justification::centred);
  control.label.setColour(juce::Label::textColourId, juce::Colours::white);

  control.usesCombo = true;
  addAndMakeVisible(control.combo);
  addAndMakeVisible(control.label);

  control.comboAttachment = std::make_unique<ComboAttachment>(state, paramId, control.combo);
}

void DustPressAudioProcessorEditor::layoutRow(juce::Rectangle<int> area,
                                              std::initializer_list<Control*> controls) {
  auto columnWidth = area.getWidth() / static_cast<int>(controls.size());
  for (auto* control : controls) {
    auto column = area.removeFromLeft(columnWidth).reduced(6);
    auto labelArea = column.removeFromTop(22);
    control->label.setBounds(labelArea);
    control->component().setBounds(column);
  }
}
