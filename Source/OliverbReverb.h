#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <random>
#include <type_traits>

#include "CloudsFxEngine.h"
#include "CloudsReverb.h"

namespace miverb {

namespace detail_oliverb {
constexpr float kPi = 3.14159265358979323846f;
}

class OliverbReverb {
 public:
  void init(double sampleRate) {
    sampleRate_ = static_cast<float>(sampleRate > 1.0 ? sampleRate : 48000.0);
    sampleRateScale_ = sampleRate_ / 32000.0f;
    engine_.init(memory_.data());

    diffusion_ = 0.625f;
    size_ = 0.5f;
    smoothSize_ = size_;
    modAmount_ = 0.0f;
    modRate_ = 0.0f;
    inputGain_ = 1.0f;
    decay_ = 0.5f;
    lp_ = 1.0f;
    hp_ = 0.0f;
    phase_ = 0.0f;
    ratio_ = 0.0f;
    pitchShiftAmount_ = 1.0f;
    lpDecay1_ = lpDecay2_ = 0.0f;
    hpDecay1_ = hpDecay2_ = 0.0f;

    for (auto& lfo : lfo_) {
      lfo.init();
    }
  }

  void process(StereoFrame* inOut, size_t size) {
    using E = FxEngine<16384, Format::k16Bit>;
    using Memory = typename E::template Reserve<113,
                   typename E::template Reserve<162,
                   typename E::template Reserve<241,
                   typename E::template Reserve<399,
                   typename E::template Reserve<1253,
                   typename E::template Reserve<1738,
                   typename E::template Reserve<3411,
                   typename E::template Reserve<1513,
                   typename E::template Reserve<1363,
                   typename E::template Reserve<4782>>>>>>>>>>;

    typename E::template DelayLine<Memory, 0> ap1;
    typename E::template DelayLine<Memory, 1> ap2;
    typename E::template DelayLine<Memory, 2> ap3;
    typename E::template DelayLine<Memory, 3> ap4;
    typename E::template DelayLine<Memory, 4> dap1a;
    typename E::template DelayLine<Memory, 5> dap1b;
    typename E::template DelayLine<Memory, 6> del1;
    typename E::template DelayLine<Memory, 7> dap2a;
    typename E::template DelayLine<Memory, 8> dap2b;
    typename E::template DelayLine<Memory, 9> del2;
    typename E::Context c;

    const float kap = diffusion_;
    float lp1 = lpDecay1_;
    float lp2 = lpDecay2_;
    float hp1 = hpDecay1_;
    float hp2 = hpDecay2_;

    float slope = modRate_ * modRate_;
    slope *= slope * slope;
    slope /= 200.0f;
    for (auto& lfo : lfo_) {
      lfo.setSlope(slope);
    }

    while (size-- > 0) {
      engine_.start(&c);

      smoothSize_ += 0.01f * (size_ - smoothSize_);

      const float psSize = (128.0f + (3410.0f - 128.0f) * smoothSize_) * sampleRateScale_;
      phase_ += (1.0f - ratio_) / psSize;
      while (phase_ >= 1.0f) {
        phase_ -= 1.0f;
      }
      while (phase_ <= 0.0f) {
        phase_ += 1.0f;
      }

      float tri = 2.0f * (phase_ >= 0.5f ? 1.0f - phase_ : phase_);
      tri = window(tri);
      float phase = phase_ * psSize;
      float half = phase + psSize * 0.5f;
      if (half >= psSize) {
        half -= psSize;
      }

      const auto interpLfo = [&](auto& d, RandomOscillator& lfo, float gain) {
        using D = std::remove_reference_t<decltype(d)>;
        float offset = (static_cast<float>(D::kLength) - 1.0f) * smoothSize_;
        offset += lfo.next() * modAmount_ * sampleRateScale_;
        offset = std::clamp(offset, 1.0f, static_cast<float>(D::kLength - 1));
        c.interpolateHermite(d, offset, gain);
      };

      const auto interpStatic = [&](auto& d, float gain) {
        using D = std::remove_reference_t<decltype(d)>;
        float offset = (static_cast<float>(D::kLength) - 1.0f) * smoothSize_;
        offset = std::clamp(offset, 1.0f, static_cast<float>(D::kLength - 1));
        c.interpolateHermite(d, offset, gain);
      };

      c.interpolate(ap1, 10.0f, LFOIndex::k1, 60.0f * sampleRateScale_, 1.0f);
      c.write(ap1, 100, 0.0f);

      c.read(inOut->l + inOut->r, inputGain_);
      interpLfo(ap1, lfo_[1], kap);
      c.writeAllPass(ap1, -kap);
      interpLfo(ap2, lfo_[2], kap);
      c.writeAllPass(ap2, -kap);
      interpLfo(ap3, lfo_[3], kap);
      c.writeAllPass(ap3, -kap);
      interpLfo(ap4, lfo_[4], kap);
      c.writeAllPass(ap4, -kap);

      float apout = 0.0f;
      c.write(apout);

      interpLfo(del2, lfo_[5], decay_ * (1.0f - pitchShiftAmount_));
      c.interpolateHermite(del2, phase, tri * decay_ * pitchShiftAmount_);
      c.interpolateHermite(del2, half, (1.0f - tri) * decay_ * pitchShiftAmount_);
      c.lp(lp1, lp_);
      c.hp(hp1, hp_);
      c.softLimit();
      interpLfo(dap1a, lfo_[6], -kap);
      c.writeAllPass(dap1a, kap);
      interpStatic(dap1b, kap);
      c.writeAllPass(dap1b, -kap);
      c.write(del1, 2.0f);
      c.write(inOut->l, 0.0f);

      c.load(apout);
      interpLfo(del1, lfo_[7], decay_ * (1.0f - pitchShiftAmount_));
      c.interpolateHermite(del1, phase, tri * decay_ * pitchShiftAmount_);
      c.interpolateHermite(del1, half, (1.0f - tri) * decay_ * pitchShiftAmount_);
      c.lp(lp2, lp_);
      c.hp(hp2, hp_);
      c.softLimit();
      interpLfo(dap2a, lfo_[8], kap);
      c.writeAllPass(dap2a, -kap);
      interpStatic(dap2b, -kap);
      c.writeAllPass(dap2b, kap);
      c.write(del2, 2.0f);
      c.write(inOut->r, 0.0f);

      ++inOut;
    }

    lpDecay1_ = lp1;
    lpDecay2_ = lp2;
    hpDecay1_ = hp1;
    hpDecay2_ = hp2;
  }

  void setInputGain(float v) { inputGain_ = v; }
  void setDecay(float v) { decay_ = v; }
  void setDiffusion(float v) { diffusion_ = v; }
  void setLp(float v) { lp_ = v; }
  void setHp(float v) { hp_ = v; }
  void setSize(float v) { size_ = v; }
  void setModAmount(float v) { modAmount_ = v; }
  void setModRate(float v) { modRate_ = v; }
  void setRatio(float v) { ratio_ = v; }
  void setPitchShiftAmount(float v) { pitchShiftAmount_ = v; }

 private:
  class RandomOscillator {
   public:
    void init() {
      phase_ = 0.0f;
      value_ = 0.0f;
      nextValue_ = random();
      direction_ = false;
      phaseIncrement_ = 0.0f;
    }

    void setSlope(float slope) {
      const float gap = std::max(0.001f, std::abs(nextValue_ - value_));
      phaseIncrement_ = std::min(1.0f, slope / gap);
    }

    float next() {
      phase_ += phaseIncrement_;
      if (phase_ > 1.0f) {
        phase_ -= 1.0f;
        value_ = nextValue_;
        direction_ = !direction_;
        const float rnd = (1.0f - 0.3f) * uniform_01_(rng_) + 0.3f;
        nextValue_ = direction_ ? value_ + (1.0f - value_) * rnd : value_ - (1.0f + value_) * rnd;
      }
      const float s = 0.5f - 0.5f * std::cos(detail_oliverb::kPi * phase_);
      return value_ + (nextValue_ - value_) * s;
    }

   private:
    float random() { return uniform_bi_(rng_); }

    std::minstd_rand rng_ {0x5A17u};
    std::uniform_real_distribution<float> uniform_bi_ {-1.0f, 1.0f};
    std::uniform_real_distribution<float> uniform_01_ {0.0f, 1.0f};
    float phase_ = 0.0f;
    float phaseIncrement_ = 0.0f;
    float value_ = 0.0f;
    float nextValue_ = 0.0f;
    bool direction_ = false;
  };

  static float window(float x) {
    x = std::clamp(x, 0.0f, 1.0f);
    return 0.5f - 0.5f * std::cos(detail_oliverb::kPi * x);
  }

  FxEngine<16384, Format::k16Bit> engine_;
  std::array<uint16_t, 16384> memory_ {};
  std::array<RandomOscillator, 9> lfo_ {};

  float inputGain_ = 1.0f;
  float decay_ = 0.5f;
  float diffusion_ = 0.625f;
  float lp_ = 1.0f;
  float hp_ = 0.0f;
  float size_ = 0.5f;
  float smoothSize_ = 0.5f;
  float modAmount_ = 0.0f;
  float modRate_ = 0.0f;
  float pitchShiftAmount_ = 1.0f;

  float lpDecay1_ = 0.0f;
  float lpDecay2_ = 0.0f;
  float hpDecay1_ = 0.0f;
  float hpDecay2_ = 0.0f;

  float phase_ = 0.0f;
  float ratio_ = 0.0f;
  float sampleRate_ = 48000.0f;
  float sampleRateScale_ = 1.0f;
};

}  // namespace miverb
