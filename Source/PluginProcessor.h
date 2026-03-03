#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <cmath>

#include "CloudsReverb.h"
#include "OliverbReverb.h"

class MiVerbAudioProcessor : public juce::AudioProcessor {
 public:
  MiVerbAudioProcessor();
  ~MiVerbAudioProcessor() override = default;

  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

  bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
  void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

  juce::AudioProcessorEditor* createEditor() override;
  bool hasEditor() const override;

  const juce::String getName() const override;
  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String& newName) override;

  void getStateInformation(juce::MemoryBlock& destData) override;
  void setStateInformation(const void* data, int sizeInBytes) override;

  juce::AudioProcessorValueTreeState& getValueTreeState() { return parameters_; }
  float getLfo1PhaseNormalized() const { return lfo1PhaseNormalized_.load(std::memory_order_relaxed); }
  float getLfo2PhaseNormalized() const { return lfo2PhaseNormalized_.load(std::memory_order_relaxed); }

  static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

 private:
  struct SimpleLfo {
    void prepare(double sampleRate) {
      sampleRate_ = sampleRate > 1.0 ? sampleRate : 48000.0;
      phase_ = 0.0;
    }

    float nextSample(float rateHz) {
      const auto targetIncrement = juce::MathConstants<double>::twoPi * static_cast<double>(rateHz) / sampleRate_;
      if (refined_) {
        increment_ += 0.0025 * (targetIncrement - increment_);
      } else {
        increment_ = targetIncrement;
      }
      phase_ += increment_;
      if (phase_ >= juce::MathConstants<double>::twoPi) {
        phase_ -= juce::MathConstants<double>::twoPi;
      }
      return std::sin(static_cast<float>(phase_));
    }

    void setRefined(bool refined) { refined_ = refined; }

    float phaseNormalized() const {
      return static_cast<float>(phase_ / juce::MathConstants<double>::twoPi);
    }

   private:
    double sampleRate_ = 48000.0;
    double phase_ = 0.0;
    double increment_ = 0.0;
    bool refined_ = false;
  };

  struct ParamSmoothers {
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mix;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> inputDb;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> time;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> diffusion;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> damping;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> size;

    void reset(double sampleRate, double rampSeconds) {
      mix.reset(sampleRate, rampSeconds);
      inputDb.reset(sampleRate, rampSeconds);
      time.reset(sampleRate, rampSeconds);
      diffusion.reset(sampleRate, rampSeconds);
      damping.reset(sampleRate, rampSeconds);
      size.reset(sampleRate, rampSeconds);
    }

    void setCurrentAndTarget(float mixV, float inV, float tV, float diffV, float dampV, float sizeV) {
      mix.setCurrentAndTargetValue(mixV);
      inputDb.setCurrentAndTargetValue(inV);
      time.setCurrentAndTargetValue(tV);
      diffusion.setCurrentAndTargetValue(diffV);
      damping.setCurrentAndTargetValue(dampV);
      size.setCurrentAndTargetValue(sizeV);
    }
  };

  void updateModulationRates();
  void applySmoothingMode(int smoothingMode);
  void updateOversamplingMode(int oversamplingMode, int maxBlockSize);
  void processInternal(float* left, float* right, int numSamples);
  double getHostBpmOrDefault() const;
  static float syncRateFromDivision(float bpm, int divisionIndex);

  juce::AudioProcessorValueTreeState parameters_;
  miverb::CloudsReverb cloudsReverb_;
  miverb::OliverbReverb oliverbReverb_;

  SimpleLfo lfo1_;
  SimpleLfo lfo2_;
  ParamSmoothers smooth_;

  float lfo1Rate_ = 0.4f;
  float lfo2Rate_ = 0.13f;
  std::atomic<float> lfo1PhaseNormalized_ {0.0f};
  std::atomic<float> lfo2PhaseNormalized_ {0.0f};

  double sampleRate_ = 48000.0;
  double processingSampleRate_ = 48000.0;
  int smoothingMode_ = -1;
  int oversamplingMode_ = -1;
  int oversamplingFactor_ = 1;

  std::unique_ptr<juce::dsp::Oversampling<float>> oversampling2x_;
  std::unique_ptr<juce::dsp::Oversampling<float>> oversampling4x_;

  float decorX1_ = 0.0f;
  float decorY1_ = 0.0f;
  float decorX2_ = 0.0f;
  float decorY2_ = 0.0f;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MiVerbAudioProcessor)
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
