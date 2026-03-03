#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

#include "PluginProcessor.h"

class MiVerbAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer {
 public:
  explicit MiVerbAudioProcessorEditor(MiVerbAudioProcessor&);
  ~MiVerbAudioProcessorEditor() override = default;

  void paint(juce::Graphics&) override;
  void resized() override;
  void timerCallback() override;

 private:
  using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
  using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
  using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

  struct Knob {
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<SliderAttachment> attachment;
  };

  struct DepthRow {
    juce::Label targetLabel;
    juce::Slider lfo1;
    juce::Slider lfo2;
    std::unique_ptr<SliderAttachment> lfo1Attachment;
    std::unique_ptr<SliderAttachment> lfo2Attachment;
  };

  void setupKnob(Knob& knob, const juce::String& name, const juce::String& paramId);
  void setupDepthRow(DepthRow& row,
                     const juce::String& target,
                     const juce::String& lfo1Id,
                     const juce::String& lfo2Id);
  void setParameterValue(const juce::String& paramId, float plainValue);
  void loadPreset(int presetIndex);
  void drawLfoMeter(juce::Graphics& g,
                    juce::Rectangle<float> bounds,
                    float phase,
                    juce::Colour colour,
                    const juce::String& label);

  MiVerbAudioProcessor& processor_;

  juce::Label title_;
  juce::Label sectionMain_;
  juce::Label sectionLfo_;
  juce::Label sectionMatrix_;
  juce::Label lfo1DepthHeader_;
  juce::Label lfo2DepthHeader_;

  juce::Label algorithmLabel_;
  juce::Label smoothingLabel_;
  juce::Label oversamplingLabel_;
  juce::Label modulationLabel_;
  juce::Label presetLabel_;
  juce::ComboBox algorithmCombo_;
  juce::ComboBox smoothingCombo_;
  juce::ComboBox oversamplingCombo_;
  juce::ComboBox modulationCombo_;
  juce::ComboBox presetCombo_;
  juce::TextButton loadPresetButton_ {"Load"};
  std::unique_ptr<ComboBoxAttachment> algorithmAttachment_;
  std::unique_ptr<ComboBoxAttachment> smoothingAttachment_;
  std::unique_ptr<ComboBoxAttachment> oversamplingAttachment_;
  std::unique_ptr<ComboBoxAttachment> modulationAttachment_;

  juce::ToggleButton lfo1Sync_;
  juce::ToggleButton lfo2Sync_;
  juce::ComboBox lfo1Division_;
  juce::ComboBox lfo2Division_;
  juce::Label lfo1DivisionLabel_;
  juce::Label lfo2DivisionLabel_;
  std::unique_ptr<ButtonAttachment> lfo1SyncAttachment_;
  std::unique_ptr<ButtonAttachment> lfo2SyncAttachment_;
  std::unique_ptr<ComboBoxAttachment> lfo1DivisionAttachment_;
  std::unique_ptr<ComboBoxAttachment> lfo2DivisionAttachment_;

  juce::Rectangle<float> lfo1MeterBounds_;
  juce::Rectangle<float> lfo2MeterBounds_;

  Knob mix_;
  Knob input_;
  Knob time_;
  Knob diffusion_;
  Knob damping_;
  Knob size_;
  Knob decayLow_;
  Knob decayHigh_;
  Knob decorrelation_;
  Knob lfo1Rate_;
  Knob lfo2Rate_;

  DepthRow rowMix_;
  DepthRow rowInput_;
  DepthRow rowTime_;
  DepthRow rowDiffusion_;
  DepthRow rowDamping_;
  DepthRow rowSize_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MiVerbAudioProcessorEditor)
};
