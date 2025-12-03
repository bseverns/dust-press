#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "PluginProcessor.h"

class DustPressAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
  explicit DustPressAudioProcessorEditor(DustPressAudioProcessor& p);
  ~DustPressAudioProcessorEditor() override = default;

  void paint(juce::Graphics& g) override;
  void resized() override;

private:
  struct Knob {
    juce::Slider slider;
    juce::ComboBox combo;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> comboAttachment;
    bool usesCombo = false;

    juce::Component& component() { return usesCombo ? static_cast<juce::Component&>(combo)
                                                    : static_cast<juce::Component&>(slider); }
  };

  void addKnob(Knob& target, const juce::String& name, const juce::String& paramId, bool bipolar = false);
  void addChoice(Knob& target, const juce::String& name, const juce::String& paramId,
                 const juce::StringArray& options);
  void layoutRow(juce::Rectangle<int> area, std::initializer_list<Knob*> knobs);

  DustPressAudioProcessor& processor;

  Knob drive{};
  Knob bias{};
  Knob envToDrive{};
  Knob curve{};
  Knob chaos{};
  Knob gateComp{};
  Knob preTilt{};
  Knob postAir{};
  Knob dirt{};
  Knob mix{};
  Knob ceiling{};
  Knob output{};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DustPressAudioProcessorEditor)
};
