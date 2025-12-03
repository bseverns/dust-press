#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "DustPressAudioProcessor.h"

class DustPressAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
  explicit DustPressAudioProcessorEditor(DustPressAudioProcessor& processor);
  ~DustPressAudioProcessorEditor() override = default;

  void paint(juce::Graphics& g) override;
  void resized() override;

private:
  using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
  using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

  struct Control {
    juce::Slider slider;
    juce::ComboBox combo;
    juce::Label label;
    std::unique_ptr<SliderAttachment> sliderAttachment;
    std::unique_ptr<ComboAttachment> comboAttachment;
    bool usesCombo = false;

    juce::Component& component() { return usesCombo ? static_cast<juce::Component&>(combo)
                                                    : static_cast<juce::Component&>(slider); }
  };

  void addSlider(Control& control, const juce::String& name, const juce::String& paramId,
                 const juce::String& suffix = {}, bool centerOnZero = false);
  void addChoice(Control& control, const juce::String& name, const juce::String& paramId,
                 const juce::StringArray& options);
  void layoutRow(juce::Rectangle<int> area, std::initializer_list<Control*> controls);

  DustPressAudioProcessor& processor;
  juce::AudioProcessorValueTreeState& state;

  Control drive{};
  Control bias{};
  Control envToDrive{};

  Control curve{};
  Control chaos{};

  Control gateComp{};
  Control preTilt{};
  Control postAir{};

  Control dirt{};
  Control mix{};
  Control ceiling{};
  Control output{};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DustPressAudioProcessorEditor)
};
