#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "CloudsFxEngine.h"

namespace miverb {

struct StereoFrame {
  float l = 0.0f;
  float r = 0.0f;
};

class CloudsReverb {
 public:
  void init(double sampleRate) {
    sampleRate_ = static_cast<float>(sampleRate > 1.0 ? sampleRate : 48000.0);
    engine_.init(memory_.data());
    engine_.setLfoFrequency(LFOIndex::k1, 0.5f / sampleRate_);
    engine_.setLfoFrequency(LFOIndex::k2, 0.3f / sampleRate_);
    lp_ = 0.7f;
    diffusion_ = 0.625f;
    hp_ = 0.0f;
    reverbTime_ = 0.5f;
    amount_ = 0.5f;
    inputGain_ = 1.0f;
    lpDecay1_ = 0.0f;
    lpDecay2_ = 0.0f;
    hpDecay1_ = 0.0f;
    hpDecay2_ = 0.0f;
    sampleRateScale_ = sampleRate_ / 32000.0f;
  }

  void process(StereoFrame* inOut, size_t size) {
    using E = FxEngine<32768, Format::k16Bit>;
    using Memory = typename E::template Reserve<113,
                   typename E::template Reserve<162,
                   typename E::template Reserve<241,
                   typename E::template Reserve<399,
                   typename E::template Reserve<1653,
                   typename E::template Reserve<2038,
                   typename E::template Reserve<3411,
                   typename E::template Reserve<1913,
                   typename E::template Reserve<1663,
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
    const float klp = lp_;
    const float khp = hp_;
    const float krt = reverbTime_;
    const float amount = amount_;
    const float gain = inputGain_;

    const float ap1Smear = 60.0f * sampleRateScale_;
    const float del2Base = 4680.0f * sampleRateScale_;

    float lp1 = lpDecay1_;
    float lp2 = lpDecay2_;
    float hp1 = hpDecay1_;
    float hp2 = hpDecay2_;

    while (size-- > 0) {
      float wet = 0.0f;
      float apout = 0.0f;
      engine_.start(&c);

      c.interpolate(ap1, 10.0f, LFOIndex::k1, ap1Smear, 1.0f);
      c.write(ap1, 100, 0.0f);

      c.read(inOut->l + inOut->r, gain);

      c.read(ap1, -1, kap);
      c.writeAllPass(ap1, -kap);
      c.read(ap2, -1, kap);
      c.writeAllPass(ap2, -kap);
      c.read(ap3, -1, kap);
      c.writeAllPass(ap3, -kap);
      c.read(ap4, -1, kap);
      c.writeAllPass(ap4, -kap);
      c.write(apout);

      c.load(apout);
      c.interpolate(del2, del2Base, LFOIndex::k2, 100.0f, krt);
      c.lp(lp1, klp);
      c.hp(hp1, khp);
      c.softLimit();
      c.read(dap1a, -1, -kap);
      c.writeAllPass(dap1a, kap);
      c.read(dap1b, -1, kap);
      c.writeAllPass(dap1b, -kap);
      c.write(del1, 2.0f);
      c.write(wet, 0.0f);

      inOut->l += (wet - inOut->l) * amount;

      c.load(apout);
      c.read(del1, -1, krt);
      c.lp(lp2, klp);
      c.hp(hp2, khp);
      c.softLimit();
      c.read(dap2a, -1, kap);
      c.writeAllPass(dap2a, -kap);
      c.read(dap2b, -1, -kap);
      c.writeAllPass(dap2b, kap);
      c.write(del2, 2.0f);
      c.write(wet, 0.0f);

      inOut->r += (wet - inOut->r) * amount;
      ++inOut;
    }

    lpDecay1_ = lp1;
    lpDecay2_ = lp2;
    hpDecay1_ = hp1;
    hpDecay2_ = hp2;
  }

  void setAmount(float amount) { amount_ = amount; }
  void setInputGain(float inputGain) { inputGain_ = inputGain; }
  void setTime(float reverbTime) { reverbTime_ = reverbTime; }
  void setDiffusion(float diffusion) { diffusion_ = diffusion; }
  void setLp(float lp) { lp_ = lp; }
  void setHp(float hp) { hp_ = hp; }

 private:
  FxEngine<32768, Format::k16Bit> engine_;
  std::array<uint16_t, 32768> memory_ {};

  float amount_ = 0.5f;
  float inputGain_ = 1.0f;
  float reverbTime_ = 0.5f;
  float diffusion_ = 0.625f;
  float lp_ = 0.7f;
  float hp_ = 0.0f;

  float lpDecay1_ = 0.0f;
  float lpDecay2_ = 0.0f;
  float hpDecay1_ = 0.0f;
  float hpDecay2_ = 0.0f;
  float sampleRate_ = 48000.0f;
  float sampleRateScale_ = 1.0f;
};

}  // namespace miverb
