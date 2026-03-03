#include "PluginProcessor.h"

#include "PluginEditor.h"
#include <array>
#include <cmath>

namespace {
constexpr std::array<float, 10> kLfoBeatsPerCycle = {
    0.25f,  // 1/16
    0.375f, // 1/8T
    0.5f,   // 1/8
    0.75f,  // 1/4T
    1.0f,   // 1/4
    1.5f,   // 1/2T
    2.0f,   // 1/2
    4.0f,   // 1 bar
    8.0f,   // 2 bars
    16.0f   // 4 bars
};
}  // namespace

MiVerbAudioProcessor::MiVerbAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters_(*this, nullptr, "PARAMS", createParameterLayout()) {
  oversampling2x_ = std::make_unique<juce::dsp::Oversampling<float>>(
      2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
  oversampling4x_ = std::make_unique<juce::dsp::Oversampling<float>>(
      2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
}

void MiVerbAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
  juce::ignoreUnused(samplesPerBlock);
  sampleRate_ = sampleRate > 1.0 ? sampleRate : 48000.0;
  oversampling2x_->reset();
  oversampling4x_->reset();
  oversampling2x_->initProcessing(static_cast<size_t>(juce::jmax(1, samplesPerBlock)));
  oversampling4x_->initProcessing(static_cast<size_t>(juce::jmax(1, samplesPerBlock)));
  oversamplingMode_ = -1;
  updateOversamplingMode(static_cast<int>(parameters_.getRawParameterValue("oversampling")->load()), samplesPerBlock);
  smoothingMode_ = -1;
  applySmoothingMode(static_cast<int>(parameters_.getRawParameterValue("smoothing")->load()));
  decorX1_ = decorY1_ = decorX2_ = decorY2_ = 0.0f;
}

void MiVerbAudioProcessor::releaseResources() {
}

bool MiVerbAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
  const auto in = layouts.getMainInputChannelSet();
  const auto out = layouts.getMainOutputChannelSet();
  if (in != out) {
    return false;
  }
  return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void MiVerbAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
  juce::ignoreUnused(midiMessages);
  juce::ScopedNoDenormals noDenormals;

  const auto totalNumInputChannels = getTotalNumInputChannels();
  const auto totalNumOutputChannels = getTotalNumOutputChannels();

  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
    buffer.clear(i, 0, buffer.getNumSamples());
  }

  const int numSamples = buffer.getNumSamples();
  if (numSamples <= 0) {
    return;
  }

  updateOversamplingMode(static_cast<int>(parameters_.getRawParameterValue("oversampling")->load()), numSamples);

  if (oversamplingMode_ == 0) {
    auto* left = buffer.getWritePointer(0);
    float* right = totalNumInputChannels > 1 ? buffer.getWritePointer(1) : buffer.getWritePointer(0);
    processInternal(left, right, numSamples);
  } else if (oversamplingMode_ == 1) {
    juce::dsp::AudioBlock<float> block(buffer);
    auto up = oversampling2x_->processSamplesUp(block);
    processInternal(up.getChannelPointer(0), up.getChannelPointer(1), static_cast<int>(up.getNumSamples()));
    oversampling2x_->processSamplesDown(block);
  } else {
    juce::dsp::AudioBlock<float> block(buffer);
    auto up = oversampling4x_->processSamplesUp(block);
    processInternal(up.getChannelPointer(0), up.getChannelPointer(1), static_cast<int>(up.getNumSamples()));
    oversampling4x_->processSamplesDown(block);
  }

  lfo1PhaseNormalized_.store(lfo1_.phaseNormalized(), std::memory_order_relaxed);
  lfo2PhaseNormalized_.store(lfo2_.phaseNormalized(), std::memory_order_relaxed);
}

void MiVerbAudioProcessor::processInternal(float* left, float* right, int numSamples) {
  const auto algorithm = static_cast<int>(parameters_.getRawParameterValue("algorithm")->load());
  const auto smoothing = static_cast<int>(parameters_.getRawParameterValue("smoothing")->load());
  const auto modRefined = static_cast<int>(parameters_.getRawParameterValue("mod_refine")->load()) == 1;
  applySmoothingMode(smoothing);
  lfo1_.setRefined(modRefined);
  lfo2_.setRefined(modRefined);
  updateModulationRates();

  const auto mixBase = parameters_.getRawParameterValue("mix")->load();
  const auto inputBase = parameters_.getRawParameterValue("input")->load();
  const auto timeBase = parameters_.getRawParameterValue("time")->load();
  const auto diffusionBase = parameters_.getRawParameterValue("diffusion")->load();
  const auto dampingBase = parameters_.getRawParameterValue("damping")->load();
  const auto sizeBase = parameters_.getRawParameterValue("size")->load();
  const auto decayLow = parameters_.getRawParameterValue("decay_low")->load();
  const auto decayHigh = parameters_.getRawParameterValue("decay_high")->load();
  const auto decorrelation = parameters_.getRawParameterValue("decorrelation")->load();

  const auto lfo1ToMix = parameters_.getRawParameterValue("lfo1_mix")->load();
  const auto lfo2ToMix = parameters_.getRawParameterValue("lfo2_mix")->load();
  const auto lfo1ToInput = parameters_.getRawParameterValue("lfo1_input")->load();
  const auto lfo2ToInput = parameters_.getRawParameterValue("lfo2_input")->load();
  const auto lfo1ToTime = parameters_.getRawParameterValue("lfo1_time")->load();
  const auto lfo2ToTime = parameters_.getRawParameterValue("lfo2_time")->load();
  const auto lfo1ToDiff = parameters_.getRawParameterValue("lfo1_diffusion")->load();
  const auto lfo2ToDiff = parameters_.getRawParameterValue("lfo2_diffusion")->load();
  const auto lfo1ToDamp = parameters_.getRawParameterValue("lfo1_damping")->load();
  const auto lfo2ToDamp = parameters_.getRawParameterValue("lfo2_damping")->load();
  const auto lfo1ToSize = parameters_.getRawParameterValue("lfo1_size")->load();
  const auto lfo2ToSize = parameters_.getRawParameterValue("lfo2_size")->load();

  const auto lowMul = juce::jmap(decayLow, 0.0f, 1.0f, 0.65f, 1.45f);
  const auto highMul = juce::jmap(decayHigh, 0.0f, 1.0f, 0.55f, 1.35f);
  const auto hpAmount = juce::jmap(decayLow, 0.0f, 1.0f, 0.22f, 0.0f);
  const auto decorA = juce::jmap(decorrelation, 0.0f, 1.0f, 0.0f, 0.82f);

  for (int i = 0; i < numSamples; ++i) {
    const auto l1 = lfo1_.nextSample(lfo1Rate_);
    const auto l2 = lfo2_.nextSample(lfo2Rate_);

    const auto mixTarget = juce::jlimit(0.0f, 1.0f, mixBase + l1 * lfo1ToMix + l2 * lfo2ToMix);
    const auto inputTarget = juce::jlimit(-24.0f, 24.0f, inputBase + (l1 * lfo1ToInput + l2 * lfo2ToInput) * 24.0f);
    const auto timeTarget = juce::jlimit(0.1f, 0.99f, timeBase + (l1 * lfo1ToTime + l2 * lfo2ToTime) * 0.45f);
    const auto diffTarget = juce::jlimit(0.1f, 0.95f, diffusionBase + (l1 * lfo1ToDiff + l2 * lfo2ToDiff) * 0.425f);
    const auto dampTarget = juce::jlimit(0.05f, 1.0f, dampingBase + (l1 * lfo1ToDamp + l2 * lfo2ToDamp) * 0.475f);
    const auto sizeTarget = juce::jlimit(0.1f, 1.0f, sizeBase + (l1 * lfo1ToSize + l2 * lfo2ToSize) * 0.45f);

    smooth_.mix.setTargetValue(mixTarget);
    smooth_.inputDb.setTargetValue(inputTarget);
    smooth_.time.setTargetValue(timeTarget);
    smooth_.diffusion.setTargetValue(diffTarget);
    smooth_.damping.setTargetValue(dampTarget);
    smooth_.size.setTargetValue(sizeTarget);

    const auto mix = smooth_.mix.getNextValue();
    const auto inputDb = smooth_.inputDb.getNextValue();
    const auto time = smooth_.time.getNextValue();
    const auto diffusion = smooth_.diffusion.getNextValue();
    const auto damping = smooth_.damping.getNextValue();
    const auto size = smooth_.size.getNextValue();

    miverb::StereoFrame dry {};
    dry.l = left[i];
    dry.r = right[i];

    auto wet = dry;

    const auto timeShaped = juce::jlimit(0.1f, 0.88f, time * std::sqrt(lowMul * highMul));
    const auto lpShaped = juce::jlimit(0.05f, 1.0f, damping * juce::jmap(decayHigh, 0.0f, 1.0f, 0.72f, 1.20f));
    const auto sizeShaped = juce::jlimit(0.1f, 1.0f, size * std::sqrt(lowMul));
    const auto safeInputDb = juce::jlimit(-24.0f, 6.0f, inputDb);

    if (algorithm == 0) {
      cloudsReverb_.setAmount(1.0f);
      cloudsReverb_.setInputGain(juce::Decibels::decibelsToGain(safeInputDb));
      cloudsReverb_.setTime(timeShaped);
      cloudsReverb_.setDiffusion(diffusion);
      cloudsReverb_.setLp(lpShaped);
      cloudsReverb_.setHp(hpAmount);
      cloudsReverb_.process(&wet, 1);
    } else {
      oliverbReverb_.setInputGain(juce::Decibels::decibelsToGain(safeInputDb));
      oliverbReverb_.setDecay(timeShaped);
      oliverbReverb_.setDiffusion(diffusion);
      oliverbReverb_.setLp(lpShaped);
      oliverbReverb_.setHp(hpAmount);
      oliverbReverb_.setSize(sizeShaped);
      oliverbReverb_.setModRate(0.12f + 1.8f * sizeShaped);
      oliverbReverb_.setModAmount(2.0f + 14.0f * sizeShaped);
      oliverbReverb_.setRatio(0.0f);
      oliverbReverb_.setPitchShiftAmount(0.0f);
      oliverbReverb_.process(&wet, 1);
    }

    if (decorA > 0.0f) {
      const float mid = 0.5f * (wet.l + wet.r);
      float side = 0.5f * (wet.l - wet.r);
      const float y1 = -decorA * side + decorX1_ + decorA * decorY1_;
      decorX1_ = side;
      decorY1_ = y1;
      const float y2 = -decorA * y1 + decorX2_ + decorA * decorY2_;
      decorX2_ = y1;
      decorY2_ = y2;
      side = y2;
      wet.l = mid + side;
      wet.r = mid - side;
    }

    left[i] = dry.l + (wet.l - dry.l) * mix;
    right[i] = dry.r + (wet.r - dry.r) * mix;
  }
}

juce::AudioProcessorEditor* MiVerbAudioProcessor::createEditor() {
  return new MiVerbAudioProcessorEditor(*this);
}

bool MiVerbAudioProcessor::hasEditor() const {
  return true;
}

const juce::String MiVerbAudioProcessor::getName() const {
  return JucePlugin_Name;
}

bool MiVerbAudioProcessor::acceptsMidi() const {
  return false;
}

bool MiVerbAudioProcessor::producesMidi() const {
  return false;
}

bool MiVerbAudioProcessor::isMidiEffect() const {
  return false;
}

double MiVerbAudioProcessor::getTailLengthSeconds() const {
  return 12.0;
}

int MiVerbAudioProcessor::getNumPrograms() {
  return 1;
}

int MiVerbAudioProcessor::getCurrentProgram() {
  return 0;
}

void MiVerbAudioProcessor::setCurrentProgram(int index) {
  juce::ignoreUnused(index);
}

const juce::String MiVerbAudioProcessor::getProgramName(int index) {
  juce::ignoreUnused(index);
  return {};
}

void MiVerbAudioProcessor::changeProgramName(int index, const juce::String& newName) {
  juce::ignoreUnused(index, newName);
}

void MiVerbAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
  if (auto state = parameters_.copyState(); auto xml = state.createXml()) {
    copyXmlToBinary(*xml, destData);
  }
}

void MiVerbAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
  if (auto xml = getXmlFromBinary(data, sizeInBytes)) {
    const auto tree = juce::ValueTree::fromXml(*xml);
    if (tree.isValid()) {
      parameters_.replaceState(tree);
    }
  }
}

juce::AudioProcessorValueTreeState::ParameterLayout MiVerbAudioProcessor::createParameterLayout() {
  std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      "algorithm", "Algorithm", juce::StringArray {"MiVerb", "Oliverb"}, 0));
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      "smoothing", "Smoothing", juce::StringArray {"Normal", "Explicit"}, 0));
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      "oversampling", "Oversampling", juce::StringArray {"Off", "2x", "4x"}, 0));
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      "mod_refine", "Modulation", juce::StringArray {"Classic", "Refined"}, 1));

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "mix", "Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.45f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "input", "Input", juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 0.0f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "time", "Time", juce::NormalisableRange<float>(0.1f, 0.99f, 0.001f), 0.75f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "diffusion", "Diffusion", juce::NormalisableRange<float>(0.1f, 0.95f, 0.001f), 0.625f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "damping", "Damping", juce::NormalisableRange<float>(0.05f, 1.0f, 0.001f), 0.7f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "size", "Size", juce::NormalisableRange<float>(0.1f, 1.0f, 0.001f), 0.6f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "decay_low", "Low Decay", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "decay_high", "High Decay", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "decorrelation", "Decorrelation", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.25f));

  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "lfo1_rate", "LFO 1 Rate", juce::NormalisableRange<float>(0.01f, 12.0f, 0.001f, 0.35f), 0.40f));
  params.push_back(std::make_unique<juce::AudioParameterFloat>(
      "lfo2_rate", "LFO 2 Rate", juce::NormalisableRange<float>(0.01f, 12.0f, 0.001f, 0.35f), 0.13f));
  params.push_back(std::make_unique<juce::AudioParameterBool>("lfo1_sync", "LFO 1 Sync", false));
  params.push_back(std::make_unique<juce::AudioParameterBool>("lfo2_sync", "LFO 2 Sync", false));
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      "lfo1_division", "LFO 1 Division",
      juce::StringArray {"1/16", "1/8T", "1/8", "1/4T", "1/4", "1/2T", "1/2", "1 bar", "2 bars", "4 bars"}, 4));
  params.push_back(std::make_unique<juce::AudioParameterChoice>(
      "lfo2_division", "LFO 2 Division",
      juce::StringArray {"1/16", "1/8T", "1/8", "1/4T", "1/4", "1/2T", "1/2", "1 bar", "2 bars", "4 bars"}, 6));

  auto addDepthParam = [&params](const juce::String& id, const juce::String& name) {
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        id, name, juce::NormalisableRange<float>(-1.0f, 1.0f, 0.001f), 0.0f));
  };

  addDepthParam("lfo1_mix", "LFO 1 > Mix");
  addDepthParam("lfo2_mix", "LFO 2 > Mix");
  addDepthParam("lfo1_input", "LFO 1 > Input");
  addDepthParam("lfo2_input", "LFO 2 > Input");
  addDepthParam("lfo1_time", "LFO 1 > Time");
  addDepthParam("lfo2_time", "LFO 2 > Time");
  addDepthParam("lfo1_diffusion", "LFO 1 > Diffusion");
  addDepthParam("lfo2_diffusion", "LFO 2 > Diffusion");
  addDepthParam("lfo1_damping", "LFO 1 > Damping");
  addDepthParam("lfo2_damping", "LFO 2 > Damping");
  addDepthParam("lfo1_size", "LFO 1 > Size");
  addDepthParam("lfo2_size", "LFO 2 > Size");

  return {params.begin(), params.end()};
}

void MiVerbAudioProcessor::updateModulationRates() {
  const auto lfo1Sync = parameters_.getRawParameterValue("lfo1_sync")->load() >= 0.5f;
  const auto lfo2Sync = parameters_.getRawParameterValue("lfo2_sync")->load() >= 0.5f;
  const auto bpm = static_cast<float>(getHostBpmOrDefault());

  if (lfo1Sync) {
    const auto div = juce::jlimit(0, static_cast<int>(kLfoBeatsPerCycle.size() - 1),
                                  static_cast<int>(parameters_.getRawParameterValue("lfo1_division")->load()));
    lfo1Rate_ = syncRateFromDivision(bpm, div);
  } else {
    lfo1Rate_ = juce::jlimit(0.01f, 12.0f, parameters_.getRawParameterValue("lfo1_rate")->load());
  }

  if (lfo2Sync) {
    const auto div = juce::jlimit(0, static_cast<int>(kLfoBeatsPerCycle.size() - 1),
                                  static_cast<int>(parameters_.getRawParameterValue("lfo2_division")->load()));
    lfo2Rate_ = syncRateFromDivision(bpm, div);
  } else {
    lfo2Rate_ = juce::jlimit(0.01f, 12.0f, parameters_.getRawParameterValue("lfo2_rate")->load());
  }
}

void MiVerbAudioProcessor::applySmoothingMode(int smoothingMode) {
  if (smoothingMode == smoothingMode_) {
    return;
  }

  const auto mix = parameters_.getRawParameterValue("mix")->load();
  const auto input = parameters_.getRawParameterValue("input")->load();
  const auto time = parameters_.getRawParameterValue("time")->load();
  const auto diffusion = parameters_.getRawParameterValue("diffusion")->load();
  const auto damping = parameters_.getRawParameterValue("damping")->load();
  const auto size = parameters_.getRawParameterValue("size")->load();

  const double rampSeconds = smoothingMode == 0 ? 0.008 : 0.060;
  smooth_.reset(processingSampleRate_, rampSeconds);
  smooth_.setCurrentAndTarget(mix, input, time, diffusion, damping, size);
  smoothingMode_ = smoothingMode;
}

void MiVerbAudioProcessor::updateOversamplingMode(int oversamplingMode, int maxBlockSize) {
  if (oversamplingMode == oversamplingMode_) {
    return;
  }

  if (maxBlockSize > 0) {
    oversampling2x_->reset();
    oversampling4x_->reset();
    oversampling2x_->initProcessing(static_cast<size_t>(maxBlockSize));
    oversampling4x_->initProcessing(static_cast<size_t>(maxBlockSize));
  }

  oversamplingMode_ = juce::jlimit(0, 2, oversamplingMode);
  oversamplingFactor_ = oversamplingMode_ == 0 ? 1 : (oversamplingMode_ == 1 ? 2 : 4);
  processingSampleRate_ = sampleRate_ * static_cast<double>(oversamplingFactor_);

  cloudsReverb_.init(processingSampleRate_);
  oliverbReverb_.init(processingSampleRate_);
  lfo1_.prepare(processingSampleRate_);
  lfo2_.prepare(processingSampleRate_);
  decorX1_ = decorY1_ = decorX2_ = decorY2_ = 0.0f;

  smoothingMode_ = -1;
  applySmoothingMode(static_cast<int>(parameters_.getRawParameterValue("smoothing")->load()));

  int latencySamples = 0;
  if (oversamplingMode_ == 1) {
    latencySamples = static_cast<int>(std::round(oversampling2x_->getLatencyInSamples()));
  } else if (oversamplingMode_ == 2) {
    latencySamples = static_cast<int>(std::round(oversampling4x_->getLatencyInSamples()));
  }
  setLatencySamples(latencySamples);
}

double MiVerbAudioProcessor::getHostBpmOrDefault() const {
  if (auto* playHead = getPlayHead()) {
    if (auto info = playHead->getPosition(); info.hasValue()) {
      if (auto bpm = info->getBpm(); bpm.hasValue() && *bpm > 0.0) {
        return *bpm;
      }
    }
  }
  return 120.0;
}

float MiVerbAudioProcessor::syncRateFromDivision(float bpm, int divisionIndex) {
  const auto idx = juce::jlimit(0, static_cast<int>(kLfoBeatsPerCycle.size() - 1), divisionIndex);
  const auto beatsPerCycle = kLfoBeatsPerCycle[static_cast<size_t>(idx)];
  const auto beatsPerSecond = bpm / 60.0f;
  return juce::jlimit(0.01f, 20.0f, beatsPerSecond / beatsPerCycle);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
  return new MiVerbAudioProcessor();
}
