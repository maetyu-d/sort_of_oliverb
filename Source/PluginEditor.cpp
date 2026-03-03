#include "PluginEditor.h"
#include <cmath>
#include <vector>

namespace {
struct UiPreset {
  const char* name;
  int algorithm;
  int smoothing;
  int oversampling;
  int modRefine;
  float mix;
  float input;
  float time;
  float diffusion;
  float damping;
  float size;
  float decayLow;
  float decayHigh;
  float decorrelation;
  float lfo1Rate;
  float lfo2Rate;
  bool lfo1Sync;
  bool lfo2Sync;
  int lfo1Division;
  int lfo2Division;
  float lfo1Mix;
  float lfo2Mix;
  float lfo1Input;
  float lfo2Input;
  float lfo1Time;
  float lfo2Time;
  float lfo1Diffusion;
  float lfo2Diffusion;
  float lfo1Damping;
  float lfo2Damping;
  float lfo1Size;
  float lfo2Size;
};

const std::vector<UiPreset> kUiPresets = {
    {"Clocked Bloom Hall", 1, 1, 1, 1, 0.48f, 1.5f, 0.78f, 0.72f, 0.58f, 0.86f, 0.66f, 0.55f, 0.30f, 0.40f, 0.13f,
     true, true, 7, 3, 0.00f, 0.00f, 0.00f, 0.00f, 0.22f, 0.00f, 0.00f, 0.18f, 0.00f, 0.00f, 0.45f, 0.00f},
    {"Breathing Chamber", 0, 1, 0, 1, 0.36f, 0.0f, 0.63f, 0.55f, 0.74f, 0.50f, 0.58f, 0.52f, 0.18f, 0.11f, 0.23f,
     false, false, 4, 6, 0.12f, 0.00f, 0.00f, 0.00f, 0.35f, 0.00f, 0.00f, 0.00f, 0.00f, -0.22f, 0.00f, 0.00f},
    {"Shifting Crystal Room", 1, 0, 1, 0, 0.42f, 2.0f, 0.69f, 0.83f, 0.44f, 0.67f, 0.42f, 0.78f, 0.46f, 0.40f, 0.13f,
     true, true, 6, 2, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.14f, 0.38f, 0.00f, 0.00f, 0.00f, 0.00f, -0.26f},
    {"Collapsed Cathedral Pulse", 0, 0, 1, 1, 0.32f, -4.0f, 0.54f, 0.56f, 0.74f, 0.46f, 0.34f, 0.40f, 0.10f, 0.18f,
     0.27f, false, false, 4, 6, 0.04f, 0.00f, 0.00f, 0.00f, 0.06f, 0.00f, 0.00f, -0.12f, 0.00f, 0.00f, 0.00f, 0.00f},
    {"Rubber Stairwell", 1, 1, 0, 1, 0.40f, -1.0f, 0.57f, 0.46f, 0.66f, 0.48f, 0.52f, 0.56f, 0.34f, 0.38f, 1.90f,
     false, false, 4, 6, 0.00f, -0.18f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.20f, 0.00f, 0.50f, 0.00f},
    {"Orbiting Tunnel", 0, 1, 1, 1, 0.47f, 1.0f, 0.74f, 0.62f, 0.60f, 0.55f, 0.62f, 0.48f, 0.40f, 0.40f, 0.13f,
     true, true, 9, 1, 0.00f, 0.16f, 0.00f, 0.00f, 0.42f, 0.00f, 0.00f, 0.24f, 0.00f, 0.00f, 0.00f, 0.00f},
    {"Tidal Plate", 1, 1, 2, 1, 0.33f, -2.0f, 0.66f, 0.58f, 0.78f, 0.73f, 0.92f, 0.34f, 0.22f, 0.06f, 0.31f, false,
     false, 4, 6, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.18f, 0.00f, 0.00f, -0.20f, 0.00f, 0.40f, 0.00f},
    {"Neon Box Flutter", 0, 0, 0, 0, 0.29f, 4.0f, 0.49f, 0.40f, 0.50f, 0.44f, 0.46f, 0.80f, 0.62f, 0.40f, 0.13f,
     true, true, 4, 0, 0.20f, 0.00f, 0.00f, 0.00f, 0.00f, -0.20f, 0.00f, 0.30f, 0.00f, 0.00f, 0.00f, 0.00f},
    {"Slow-Motion Superhall", 1, 1, 2, 1, 0.58f, 0.0f, 0.95f, 0.88f, 0.41f, 0.98f, 0.85f, 0.45f, 0.54f, 0.40f, 0.13f,
     true, true, 9, 7, 0.00f, 0.00f, 0.00f, 0.00f, 0.18f, 0.00f, 0.00f, 0.22f, 0.00f, 0.00f, -0.15f, 0.00f},
    {"Fractal Closet", 0, 1, 0, 1, 0.24f, -3.0f, 0.38f, 0.77f, 0.82f, 0.30f, 0.50f, 0.74f, 0.28f, 0.72f, 0.13f,
     false, false, 4, 6, 0.00f, 0.00f, 0.00f, 0.00f, 0.00f, 0.26f, 0.00f, 0.00f, -0.28f, 0.00f, 0.35f, 0.00f},
};

void styleLabel(juce::Label& label, bool section = false) {
  label.setColour(juce::Label::textColourId, section ? juce::Colour::fromRGB(230, 233, 238) : juce::Colour::fromRGB(190, 195, 203));
  label.setFont(juce::FontOptions {section ? 15.0f : 12.0f, section ? juce::Font::bold : juce::Font::plain});
}

void styleCombo(juce::ComboBox& box) {
  box.setColour(juce::ComboBox::backgroundColourId, juce::Colour::fromRGB(28, 31, 36));
  box.setColour(juce::ComboBox::outlineColourId, juce::Colour::fromRGB(62, 68, 76));
  box.setColour(juce::ComboBox::textColourId, juce::Colour::fromRGB(230, 233, 238));
  box.setColour(juce::ComboBox::arrowColourId, juce::Colour::fromRGB(136, 175, 196));
}
}  // namespace

MiVerbAudioProcessorEditor::MiVerbAudioProcessorEditor(MiVerbAudioProcessor& p)
    : AudioProcessorEditor(&p), processor_(p) {
  setSize(1020, 700);
  startTimerHz(30);

  title_.setText("MiVerb", juce::dontSendNotification);
  title_.setFont(juce::FontOptions {30.0f, juce::Font::bold});
  title_.setJustificationType(juce::Justification::centredLeft);
  title_.setColour(juce::Label::textColourId, juce::Colour::fromRGB(240, 243, 247));
  addAndMakeVisible(title_);

  sectionMain_.setText("Reverb", juce::dontSendNotification);
  sectionLfo_.setText("LFO", juce::dontSendNotification);
  sectionMatrix_.setText("Mod Matrix", juce::dontSendNotification);
  styleLabel(sectionMain_, true);
  styleLabel(sectionLfo_, true);
  styleLabel(sectionMatrix_, true);
  addAndMakeVisible(sectionMain_);
  addAndMakeVisible(sectionLfo_);
  addAndMakeVisible(sectionMatrix_);

  lfo1DepthHeader_.setText("LFO 1", juce::dontSendNotification);
  lfo2DepthHeader_.setText("LFO 2", juce::dontSendNotification);
  lfo1DepthHeader_.setJustificationType(juce::Justification::centred);
  lfo2DepthHeader_.setJustificationType(juce::Justification::centred);
  styleLabel(lfo1DepthHeader_);
  styleLabel(lfo2DepthHeader_);
  addAndMakeVisible(lfo1DepthHeader_);
  addAndMakeVisible(lfo2DepthHeader_);

  algorithmLabel_.setText("Algorithm", juce::dontSendNotification);
  smoothingLabel_.setText("Smoothing", juce::dontSendNotification);
  oversamplingLabel_.setText("Oversampling", juce::dontSendNotification);
  modulationLabel_.setText("Modulation", juce::dontSendNotification);
  presetLabel_.setText("Preset", juce::dontSendNotification);
  styleLabel(algorithmLabel_);
  styleLabel(smoothingLabel_);
  styleLabel(oversamplingLabel_);
  styleLabel(modulationLabel_);
  styleLabel(presetLabel_);
  addAndMakeVisible(algorithmLabel_);
  addAndMakeVisible(smoothingLabel_);
  addAndMakeVisible(oversamplingLabel_);
  addAndMakeVisible(modulationLabel_);
  addAndMakeVisible(presetLabel_);

  algorithmCombo_.addItem("MiVerb", 1);
  algorithmCombo_.addItem("Oliverb", 2);
  smoothingCombo_.addItem("Normal", 1);
  smoothingCombo_.addItem("Explicit", 2);
  oversamplingCombo_.addItem("Off", 1);
  oversamplingCombo_.addItem("2x", 2);
  oversamplingCombo_.addItem("4x", 3);
  modulationCombo_.addItem("Classic", 1);
  modulationCombo_.addItem("Refined", 2);
  styleCombo(algorithmCombo_);
  styleCombo(smoothingCombo_);
  styleCombo(oversamplingCombo_);
  styleCombo(modulationCombo_);
  styleCombo(presetCombo_);
  addAndMakeVisible(algorithmCombo_);
  addAndMakeVisible(smoothingCombo_);
  addAndMakeVisible(oversamplingCombo_);
  addAndMakeVisible(modulationCombo_);
  addAndMakeVisible(presetCombo_);
  addAndMakeVisible(loadPresetButton_);

  loadPresetButton_.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(42, 47, 54));
  loadPresetButton_.setColour(juce::TextButton::buttonOnColourId, juce::Colour::fromRGB(59, 93, 115));
  loadPresetButton_.setColour(juce::TextButton::textColourOffId, juce::Colour::fromRGB(226, 230, 236));

  algorithmAttachment_ = std::make_unique<ComboBoxAttachment>(processor_.getValueTreeState(), "algorithm", algorithmCombo_);
  smoothingAttachment_ = std::make_unique<ComboBoxAttachment>(processor_.getValueTreeState(), "smoothing", smoothingCombo_);
  oversamplingAttachment_ =
      std::make_unique<ComboBoxAttachment>(processor_.getValueTreeState(), "oversampling", oversamplingCombo_);
  modulationAttachment_ =
      std::make_unique<ComboBoxAttachment>(processor_.getValueTreeState(), "mod_refine", modulationCombo_);

  for (int i = 0; i < static_cast<int>(kUiPresets.size()); ++i) {
    presetCombo_.addItem(kUiPresets[static_cast<size_t>(i)].name, i + 1);
  }
  presetCombo_.setSelectedId(1, juce::dontSendNotification);
  loadPresetButton_.onClick = [this]() {
    const int idx = presetCombo_.getSelectedItemIndex();
    if (idx >= 0) {
      loadPreset(idx);
    }
  };

  setupKnob(mix_, "Mix", "mix");
  setupKnob(input_, "Input", "input");
  setupKnob(time_, "Time", "time");
  setupKnob(diffusion_, "Diffusion", "diffusion");
  setupKnob(damping_, "Damping", "damping");
  setupKnob(size_, "Size", "size");
  setupKnob(decayLow_, "Low Decay", "decay_low");
  setupKnob(decayHigh_, "High Decay", "decay_high");
  setupKnob(decorrelation_, "Decorrelation", "decorrelation");

  setupKnob(lfo1Rate_, "LFO 1 Rate", "lfo1_rate");
  setupKnob(lfo2Rate_, "LFO 2 Rate", "lfo2_rate");

  lfo1Sync_.setButtonText("Sync");
  lfo2Sync_.setButtonText("Sync");
  lfo1Sync_.setColour(juce::ToggleButton::textColourId, juce::Colour::fromRGB(208, 213, 221));
  lfo2Sync_.setColour(juce::ToggleButton::textColourId, juce::Colour::fromRGB(208, 213, 221));
  addAndMakeVisible(lfo1Sync_);
  addAndMakeVisible(lfo2Sync_);
  lfo1SyncAttachment_ = std::make_unique<ButtonAttachment>(processor_.getValueTreeState(), "lfo1_sync", lfo1Sync_);
  lfo2SyncAttachment_ = std::make_unique<ButtonAttachment>(processor_.getValueTreeState(), "lfo2_sync", lfo2Sync_);

  const juce::StringArray divisions {"1/16", "1/8T", "1/8", "1/4T", "1/4", "1/2T", "1/2", "1 bar", "2 bars", "4 bars"};
  for (int i = 0; i < divisions.size(); ++i) {
    lfo1Division_.addItem(divisions[i], i + 1);
    lfo2Division_.addItem(divisions[i], i + 1);
  }
  styleCombo(lfo1Division_);
  styleCombo(lfo2Division_);
  addAndMakeVisible(lfo1Division_);
  addAndMakeVisible(lfo2Division_);
  lfo1DivisionAttachment_ = std::make_unique<ComboBoxAttachment>(processor_.getValueTreeState(), "lfo1_division", lfo1Division_);
  lfo2DivisionAttachment_ = std::make_unique<ComboBoxAttachment>(processor_.getValueTreeState(), "lfo2_division", lfo2Division_);

  lfo1DivisionLabel_.setText("Division", juce::dontSendNotification);
  lfo2DivisionLabel_.setText("Division", juce::dontSendNotification);
  lfo1DivisionLabel_.setJustificationType(juce::Justification::centredLeft);
  lfo2DivisionLabel_.setJustificationType(juce::Justification::centredLeft);
  styleLabel(lfo1DivisionLabel_);
  styleLabel(lfo2DivisionLabel_);
  addAndMakeVisible(lfo1DivisionLabel_);
  addAndMakeVisible(lfo2DivisionLabel_);

  setupDepthRow(rowMix_, "Mix", "lfo1_mix", "lfo2_mix");
  setupDepthRow(rowInput_, "Input", "lfo1_input", "lfo2_input");
  setupDepthRow(rowTime_, "Time", "lfo1_time", "lfo2_time");
  setupDepthRow(rowDiffusion_, "Diffusion", "lfo1_diffusion", "lfo2_diffusion");
  setupDepthRow(rowDamping_, "Damping", "lfo1_damping", "lfo2_damping");
  setupDepthRow(rowSize_, "Size", "lfo1_size", "lfo2_size");
}

void MiVerbAudioProcessorEditor::paint(juce::Graphics& g) {
  juce::ColourGradient bg(
      juce::Colour::fromRGB(16, 18, 22),
      0.0f,
      0.0f,
      juce::Colour::fromRGB(23, 27, 33),
      0.0f,
      static_cast<float>(getHeight()),
      false);
  bg.addColour(0.55, juce::Colour::fromRGB(19, 22, 28));
  g.setGradientFill(bg);
  g.fillAll();

  auto bounds = getLocalBounds().toFloat().reduced(16.0f);
  g.setColour(juce::Colour::fromRGB(43, 48, 56));
  g.drawRoundedRectangle(bounds, 14.0f, 1.0f);

  const auto topPanel = juce::Rectangle<float>(bounds.getX() + 14.0f, bounds.getY() + 94.0f, bounds.getWidth() - 28.0f, 232.0f);
  const auto bottomPanel = juce::Rectangle<float>(bounds.getX() + 14.0f, bounds.getY() + 336.0f, bounds.getWidth() - 28.0f, bounds.getHeight() - 350.0f);

  g.setColour(juce::Colour::fromRGB(24, 27, 33));
  g.fillRoundedRectangle(topPanel, 10.0f);
  g.fillRoundedRectangle(bottomPanel, 10.0f);

  g.setColour(juce::Colour::fromRGB(47, 52, 61));
  g.drawRoundedRectangle(topPanel, 10.0f, 1.0f);
  g.drawRoundedRectangle(bottomPanel, 10.0f, 1.0f);

  drawLfoMeter(g, lfo1MeterBounds_, processor_.getLfo1PhaseNormalized(), juce::Colour::fromRGB(128, 179, 210), "LFO 1");
  drawLfoMeter(g, lfo2MeterBounds_, processor_.getLfo2PhaseNormalized(), juce::Colour::fromRGB(186, 162, 134), "LFO 2");
}

void MiVerbAudioProcessorEditor::resized() {
  auto area = getLocalBounds().reduced(28);

  auto header = area.removeFromTop(64);
  title_.setBounds(header.removeFromLeft(180));

  const int controlW = 106;
  const int controlH = 24;
  const int pad = 14;

  algorithmLabel_.setBounds(header.getX(), header.getY(), controlW, 16);
  algorithmCombo_.setBounds(header.getX(), header.getBottom() - controlH, controlW, controlH);

  smoothingLabel_.setBounds(header.getX() + controlW + pad, header.getY(), controlW, 16);
  smoothingCombo_.setBounds(header.getX() + controlW + pad, header.getBottom() - controlH, controlW, controlH);

  oversamplingLabel_.setBounds(header.getX() + (controlW + pad) * 2, header.getY(), controlW, 16);
  oversamplingCombo_.setBounds(header.getX() + (controlW + pad) * 2, header.getBottom() - controlH, controlW, controlH);

  modulationLabel_.setBounds(header.getX() + (controlW + pad) * 3, header.getY(), controlW, 16);
  modulationCombo_.setBounds(header.getX() + (controlW + pad) * 3, header.getBottom() - controlH, controlW, controlH);

  presetLabel_.setBounds(header.getX() + (controlW + pad) * 4, header.getY(), 170, 16);
  presetCombo_.setBounds(header.getX() + (controlW + pad) * 4, header.getBottom() - controlH, 170, controlH);
  loadPresetButton_.setBounds(presetCombo_.getRight() + 8, header.getBottom() - controlH, 74, controlH);

  area.removeFromTop(10);
  auto topPanel = area.removeFromTop(232);
  area.removeFromTop(10);
  auto bottomPanel = area;

  sectionMain_.setBounds(topPanel.removeFromTop(24).removeFromLeft(120));
  topPanel.reduce(12, 6);

  std::array<Knob*, 9> knobs {&mix_, &input_, &time_, &diffusion_, &damping_, &size_, &decayLow_, &decayHigh_, &decorrelation_};
  const int knobW = topPanel.getWidth() / static_cast<int>(knobs.size());
  for (int i = 0; i < static_cast<int>(knobs.size()); ++i) {
    auto x = topPanel.getX() + i * knobW;
    knobs[static_cast<size_t>(i)]->label.setBounds(x + 2, topPanel.getY(), knobW - 4, 18);
    knobs[static_cast<size_t>(i)]->slider.setBounds(x + 2, topPanel.getY() + 20, knobW - 4, topPanel.getHeight() - 20);
  }

  auto bottomHeader = bottomPanel.removeFromTop(24);
  sectionLfo_.setBounds(bottomHeader.removeFromLeft(90));
  sectionMatrix_.setBounds(bottomHeader.removeFromLeft(180));

  bottomPanel.reduce(12, 6);
  auto lfoPanel = bottomPanel.removeFromLeft(360);
  bottomPanel.removeFromLeft(10);
  auto matrixPanel = bottomPanel;

  const int lfoColW = (lfoPanel.getWidth() - 16) / 2;
  auto lfo1Area = juce::Rectangle<int>(lfoPanel.getX(), lfoPanel.getY(), lfoColW, lfoPanel.getHeight());
  auto lfo2Area = juce::Rectangle<int>(lfoPanel.getX() + lfoColW + 16, lfoPanel.getY(), lfoColW, lfoPanel.getHeight());

  auto layoutLfo = [](juce::Rectangle<int> box,
                      Knob& rate,
                      juce::ToggleButton& sync,
                      juce::Label& divLabel,
                      juce::ComboBox& div,
                      juce::Rectangle<float>& meter) {
    rate.label.setBounds(box.getX(), box.getY(), box.getWidth(), 18);
    rate.slider.setBounds(box.getX(), box.getY() + 20, box.getWidth(), 116);
    sync.setBounds(box.getX(), box.getY() + 140, 64, 22);
    divLabel.setBounds(box.getX() + 70, box.getY() + 140, 70, 18);
    div.setBounds(box.getX() + 70, box.getY() + 158, box.getWidth() - 70, 24);
    meter = juce::Rectangle<float>(static_cast<float>(box.getX() + (box.getWidth() - 66) / 2), static_cast<float>(box.getBottom() - 78), 66.0f, 66.0f);
  };

  layoutLfo(lfo1Area, lfo1Rate_, lfo1Sync_, lfo1DivisionLabel_, lfo1Division_, lfo1MeterBounds_);
  layoutLfo(lfo2Area, lfo2Rate_, lfo2Sync_, lfo2DivisionLabel_, lfo2Division_, lfo2MeterBounds_);

  lfo1DepthHeader_.setBounds(matrixPanel.getX() + 170, matrixPanel.getY(), 120, 18);
  lfo2DepthHeader_.setBounds(matrixPanel.getX() + 300, matrixPanel.getY(), 120, 18);

  auto placeRow = [&](DepthRow& row, int rowIndex) {
    const int y = matrixPanel.getY() + 24 + rowIndex * 34;
    row.targetLabel.setBounds(matrixPanel.getX(), y, 160, 20);
    row.lfo1.setBounds(matrixPanel.getX() + 160, y, 130, 20);
    row.lfo2.setBounds(matrixPanel.getX() + 290, y, 130, 20);
  };

  placeRow(rowMix_, 0);
  placeRow(rowInput_, 1);
  placeRow(rowTime_, 2);
  placeRow(rowDiffusion_, 3);
  placeRow(rowDamping_, 4);
  placeRow(rowSize_, 5);
}

void MiVerbAudioProcessorEditor::setupKnob(Knob& knob, const juce::String& name, const juce::String& paramId) {
  knob.label.setText(name, juce::dontSendNotification);
  knob.label.setJustificationType(juce::Justification::centred);
  styleLabel(knob.label);
  addAndMakeVisible(knob.label);

  knob.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
  knob.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
  knob.slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(122, 154, 174));
  knob.slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour::fromRGB(52, 58, 67));
  knob.slider.setColour(juce::Slider::thumbColourId, juce::Colour::fromRGB(222, 224, 229));
  knob.slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour::fromRGB(220, 225, 232));
  knob.slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
  knob.slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour::fromRGB(30, 34, 40));
  addAndMakeVisible(knob.slider);

  knob.attachment = std::make_unique<SliderAttachment>(processor_.getValueTreeState(), paramId, knob.slider);
}

void MiVerbAudioProcessorEditor::setupDepthRow(DepthRow& row,
                                               const juce::String& target,
                                               const juce::String& lfo1Id,
                                               const juce::String& lfo2Id) {
  row.targetLabel.setText(target, juce::dontSendNotification);
  row.targetLabel.setJustificationType(juce::Justification::centredLeft);
  styleLabel(row.targetLabel);
  addAndMakeVisible(row.targetLabel);

  row.lfo1.setSliderStyle(juce::Slider::LinearHorizontal);
  row.lfo1.setTextBoxStyle(juce::Slider::TextBoxRight, false, 52, 18);
  row.lfo1.setColour(juce::Slider::trackColourId, juce::Colour::fromRGB(128, 179, 210));
  row.lfo1.setColour(juce::Slider::thumbColourId, juce::Colour::fromRGB(183, 214, 234));
  row.lfo1.setColour(juce::Slider::textBoxTextColourId, juce::Colour::fromRGB(219, 225, 233));
  row.lfo1.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour::fromRGB(30, 34, 40));
  row.lfo1.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
  addAndMakeVisible(row.lfo1);

  row.lfo2.setSliderStyle(juce::Slider::LinearHorizontal);
  row.lfo2.setTextBoxStyle(juce::Slider::TextBoxRight, false, 52, 18);
  row.lfo2.setColour(juce::Slider::trackColourId, juce::Colour::fromRGB(186, 162, 134));
  row.lfo2.setColour(juce::Slider::thumbColourId, juce::Colour::fromRGB(219, 195, 168));
  row.lfo2.setColour(juce::Slider::textBoxTextColourId, juce::Colour::fromRGB(219, 225, 233));
  row.lfo2.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour::fromRGB(30, 34, 40));
  row.lfo2.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
  addAndMakeVisible(row.lfo2);

  row.lfo1Attachment = std::make_unique<SliderAttachment>(processor_.getValueTreeState(), lfo1Id, row.lfo1);
  row.lfo2Attachment = std::make_unique<SliderAttachment>(processor_.getValueTreeState(), lfo2Id, row.lfo2);
}

void MiVerbAudioProcessorEditor::setParameterValue(const juce::String& paramId, float plainValue) {
  if (auto* p = processor_.getValueTreeState().getParameter(paramId)) {
    if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(p)) {
      const auto normalized = ranged->convertTo0to1(plainValue);
      ranged->beginChangeGesture();
      ranged->setValueNotifyingHost(normalized);
      ranged->endChangeGesture();
    }
  }
}

void MiVerbAudioProcessorEditor::loadPreset(int presetIndex) {
  if (presetIndex < 0 || presetIndex >= static_cast<int>(kUiPresets.size())) {
    return;
  }

  const auto& pr = kUiPresets[static_cast<size_t>(presetIndex)];
  setParameterValue("algorithm", static_cast<float>(pr.algorithm));
  setParameterValue("smoothing", static_cast<float>(pr.smoothing));
  setParameterValue("oversampling", static_cast<float>(pr.oversampling));
  setParameterValue("mod_refine", static_cast<float>(pr.modRefine));
  setParameterValue("mix", pr.mix);
  setParameterValue("input", pr.input);
  setParameterValue("time", pr.time);
  setParameterValue("diffusion", pr.diffusion);
  setParameterValue("damping", pr.damping);
  setParameterValue("size", pr.size);
  setParameterValue("decay_low", pr.decayLow);
  setParameterValue("decay_high", pr.decayHigh);
  setParameterValue("decorrelation", pr.decorrelation);
  setParameterValue("lfo1_rate", pr.lfo1Rate);
  setParameterValue("lfo2_rate", pr.lfo2Rate);
  setParameterValue("lfo1_sync", pr.lfo1Sync ? 1.0f : 0.0f);
  setParameterValue("lfo2_sync", pr.lfo2Sync ? 1.0f : 0.0f);
  setParameterValue("lfo1_division", static_cast<float>(pr.lfo1Division));
  setParameterValue("lfo2_division", static_cast<float>(pr.lfo2Division));
  setParameterValue("lfo1_mix", pr.lfo1Mix);
  setParameterValue("lfo2_mix", pr.lfo2Mix);
  setParameterValue("lfo1_input", pr.lfo1Input);
  setParameterValue("lfo2_input", pr.lfo2Input);
  setParameterValue("lfo1_time", pr.lfo1Time);
  setParameterValue("lfo2_time", pr.lfo2Time);
  setParameterValue("lfo1_diffusion", pr.lfo1Diffusion);
  setParameterValue("lfo2_diffusion", pr.lfo2Diffusion);
  setParameterValue("lfo1_damping", pr.lfo1Damping);
  setParameterValue("lfo2_damping", pr.lfo2Damping);
  setParameterValue("lfo1_size", pr.lfo1Size);
  setParameterValue("lfo2_size", pr.lfo2Size);
}

void MiVerbAudioProcessorEditor::drawLfoMeter(juce::Graphics& g,
                                              juce::Rectangle<float> bounds,
                                              float phase,
                                              juce::Colour colour,
                                              const juce::String& label) {
  const auto circle = bounds.reduced(3.0f);
  const auto centre = circle.getCentre();
  const auto radius = circle.getWidth() * 0.5f;

  g.setColour(juce::Colour::fromRGB(22, 25, 30));
  g.fillEllipse(circle);
  g.setColour(juce::Colour::fromRGB(66, 72, 80));
  g.drawEllipse(circle, 1.0f);

  const auto p = juce::jlimit(0.0f, 1.0f, phase);
  const auto start = -juce::MathConstants<float>::halfPi;
  const auto end = start + juce::MathConstants<float>::twoPi * p;
  juce::Path sweep;
  sweep.addCentredArc(centre.x, centre.y, radius - 4.0f, radius - 4.0f, 0.0f, start, end, true);
  g.setColour(colour);
  g.strokePath(sweep, juce::PathStrokeType(2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

  const auto angle = start + juce::MathConstants<float>::twoPi * p;
  const auto tip = juce::Point<float>(centre.x + std::cos(angle) * (radius - 8.0f), centre.y + std::sin(angle) * (radius - 8.0f));
  g.setColour(juce::Colour::fromRGB(231, 235, 241));
  g.drawLine(centre.x, centre.y, tip.x, tip.y, 1.6f);
  g.fillEllipse(centre.x - 2.2f, centre.y - 2.2f, 4.4f, 4.4f);

  g.setColour(juce::Colour::fromRGB(184, 191, 201));
  g.setFont(juce::FontOptions {11.0f, juce::Font::plain});
  g.drawFittedText(label, bounds.toNearestInt().translated(0, -12), juce::Justification::centredBottom, 1);
}

void MiVerbAudioProcessorEditor::timerCallback() {
  repaint(lfo1MeterBounds_.toNearestInt().expanded(18));
  repaint(lfo2MeterBounds_.toNearestInt().expanded(18));
}
