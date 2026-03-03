#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <limits>
#include <type_traits>

namespace miverb {

enum class Format {
  k12Bit,
  k16Bit,
  k32Bit
};

enum class LFOIndex {
  k1 = 0,
  k2 = 1
};

namespace detail {

inline int16_t clip16(int32_t value) {
  if (value > std::numeric_limits<int16_t>::max()) {
    return std::numeric_limits<int16_t>::max();
  }
  if (value < std::numeric_limits<int16_t>::min()) {
    return std::numeric_limits<int16_t>::min();
  }
  return static_cast<int16_t>(value);
}

inline float softLimit(float x) {
  const float x2 = x * x;
  return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

class CosineOscillator {
 public:
  void init(float frequencyPerSample) {
    phaseIncrement_ = frequencyPerSample;
  }

  float next() {
    phase_ += phaseIncrement_;
    if (phase_ >= 1.0f) {
      phase_ -= 1.0f;
    }
    return value();
  }

  float value() const {
    constexpr float twoPi = 6.28318530717958647693f;
    return std::cos(twoPi * phase_);
  }

 private:
  float phase_ = 0.0f;
  float phaseIncrement_ = 0.0f;
};

template <Format format>
struct DataType;

template <>
struct DataType<Format::k12Bit> {
  using Type = uint16_t;

  static float decompress(Type value) {
    return static_cast<float>(static_cast<int16_t>(value)) / 4096.0f;
  }

  static Type compress(float value) {
    return static_cast<Type>(clip16(static_cast<int32_t>(value * 4096.0f)));
  }
};

template <>
struct DataType<Format::k16Bit> {
  using Type = uint16_t;

  static float decompress(Type value) {
    return static_cast<float>(static_cast<int16_t>(value)) / 32768.0f;
  }

  static Type compress(float value) {
    return static_cast<Type>(clip16(static_cast<int32_t>(value * 32768.0f)));
  }
};

template <>
struct DataType<Format::k32Bit> {
  using Type = float;

  static float decompress(Type value) {
    return value;
  }

  static Type compress(float value) {
    return value;
  }
};

}  // namespace detail

template <size_t size, Format format = Format::k12Bit>
class FxEngine {
 public:
  using T = typename detail::DataType<format>::Type;

  static_assert((size & (size - 1)) == 0, "FxEngine size must be power-of-two");

  FxEngine() = default;

  void init(T* buffer) {
    buffer_ = buffer;
    clear();
  }

  void clear() {
    std::fill(&buffer_[0], &buffer_[size], T {});
    writePtr_ = 0;
  }

  struct Empty {};

  template <int32_t length, typename TailT = Empty>
  struct Reserve {
    using Tail = TailT;
    static constexpr int32_t kLength = length;
  };

  template <typename Memory, int32_t index>
  struct DelayLine {
    static constexpr int32_t kLength = DelayLine<typename Memory::Tail, index - 1>::kLength;
    static constexpr int32_t kBase = DelayLine<Memory, index - 1>::kBase +
                                     DelayLine<Memory, index - 1>::kLength + 1;
  };

  template <typename Memory>
  struct DelayLine<Memory, 0> {
    static constexpr int32_t kLength = Memory::kLength;
    static constexpr int32_t kBase = 0;
  };

  class Context {
    friend class FxEngine;

   public:
    void load(float value) {
      accumulator_ = value;
    }

    void read(float value, float scale) {
      accumulator_ += value * scale;
    }

    void read(float value) {
      accumulator_ += value;
    }

    void write(float& value) {
      value = accumulator_;
    }

    void write(float& value, float scale) {
      value = accumulator_;
      accumulator_ *= scale;
    }

    template <typename D>
    void write(D&, int32_t offset, float scale) {
      static_assert(D::kBase + D::kLength <= static_cast<int32_t>(size), "delay memory full");
      const T w = detail::DataType<format>::compress(accumulator_);
      if (offset == -1) {
        buffer_[(writePtr_ + D::kBase + D::kLength - 1) & kMask] = w;
      } else {
        buffer_[(writePtr_ + D::kBase + offset) & kMask] = w;
      }
      accumulator_ *= scale;
    }

    template <typename D>
    void write(D& d, float scale) {
      write(d, 0, scale);
    }

    template <typename D>
    void writeAllPass(D& d, int32_t offset, float scale) {
      write(d, offset, scale);
      accumulator_ += previousRead_;
    }

    template <typename D>
    void writeAllPass(D& d, float scale) {
      writeAllPass(d, 0, scale);
    }

    template <typename D>
    void read(D&, int32_t offset, float scale) {
      static_assert(D::kBase + D::kLength <= static_cast<int32_t>(size), "delay memory full");
      T r;
      if (offset == -1) {
        r = buffer_[(writePtr_ + D::kBase + D::kLength - 1) & kMask];
      } else {
        r = buffer_[(writePtr_ + D::kBase + offset) & kMask];
      }
      const float rf = detail::DataType<format>::decompress(r);
      previousRead_ = rf;
      accumulator_ += rf * scale;
    }

    template <typename D>
    void read(D& d, float scale) {
      read(d, 0, scale);
    }

    void lp(float& state, float coefficient) {
      state += coefficient * (accumulator_ - state);
      accumulator_ = state;
    }

    void hp(float& state, float coefficient) {
      state += coefficient * (accumulator_ - state);
      accumulator_ -= state;
    }

    void softLimit() {
      accumulator_ = detail::softLimit(accumulator_);
    }

    template <typename D>
    void interpolate(D&, float offset, float scale) {
      static_assert(D::kBase + D::kLength <= static_cast<int32_t>(size), "delay memory full");
      const int32_t offsetIntegral = static_cast<int32_t>(offset);
      const float offsetFractional = offset - static_cast<float>(offsetIntegral);
      const float a = detail::DataType<format>::decompress(buffer_[(writePtr_ + offsetIntegral + D::kBase) & kMask]);
      const float b = detail::DataType<format>::decompress(buffer_[(writePtr_ + offsetIntegral + D::kBase + 1) & kMask]);
      const float x = a + (b - a) * offsetFractional;
      previousRead_ = x;
      accumulator_ += x * scale;
    }

    template <typename D>
    void interpolate(D&, float offset, LFOIndex index, float amplitude, float scale) {
      offset += amplitude * lfoValue_[static_cast<size_t>(index)];
      static_assert(D::kBase + D::kLength <= static_cast<int32_t>(size), "delay memory full");
      const int32_t offsetIntegral = static_cast<int32_t>(offset);
      const float offsetFractional = offset - static_cast<float>(offsetIntegral);
      const float a = detail::DataType<format>::decompress(buffer_[(writePtr_ + offsetIntegral + D::kBase) & kMask]);
      const float b = detail::DataType<format>::decompress(buffer_[(writePtr_ + offsetIntegral + D::kBase + 1) & kMask]);
      const float x = a + (b - a) * offsetFractional;
      previousRead_ = x;
      accumulator_ += x * scale;
    }

    template <typename D>
    void interpolateHermite(D&, float offset, float scale) {
      static_assert(D::kBase + D::kLength <= static_cast<int32_t>(size), "delay memory full");
      const int32_t offsetIntegral = static_cast<int32_t>(offset);
      const float t = offset - static_cast<float>(offsetIntegral);
      const float xm1 =
          detail::DataType<format>::decompress(buffer_[(writePtr_ + offsetIntegral + D::kBase - 1) & kMask]);
      const float x0 =
          detail::DataType<format>::decompress(buffer_[(writePtr_ + offsetIntegral + D::kBase + 0) & kMask]);
      const float x1 =
          detail::DataType<format>::decompress(buffer_[(writePtr_ + offsetIntegral + D::kBase + 1) & kMask]);
      const float x2 =
          detail::DataType<format>::decompress(buffer_[(writePtr_ + offsetIntegral + D::kBase + 2) & kMask]);
      const float c = (x1 - xm1) * 0.5f;
      const float v = x0 - x1;
      const float w = c + v;
      const float a = w + v + (x2 - x0) * 0.5f;
      const float bNeg = w + a;
      const float x = (((a * t) - bNeg) * t + c) * t + x0;
      previousRead_ = x;
      accumulator_ += x * scale;
    }

    template <typename D>
    void interpolateHermite(D&, float offset, LFOIndex index, float amplitude, float scale) {
      offset += amplitude * lfoValue_[static_cast<size_t>(index)];
      static_assert(D::kBase + D::kLength <= static_cast<int32_t>(size), "delay memory full");
      const int32_t offsetIntegral = static_cast<int32_t>(offset);
      const float t = offset - static_cast<float>(offsetIntegral);
      const float xm1 =
          detail::DataType<format>::decompress(buffer_[(writePtr_ + offsetIntegral + D::kBase - 1) & kMask]);
      const float x0 =
          detail::DataType<format>::decompress(buffer_[(writePtr_ + offsetIntegral + D::kBase + 0) & kMask]);
      const float x1 =
          detail::DataType<format>::decompress(buffer_[(writePtr_ + offsetIntegral + D::kBase + 1) & kMask]);
      const float x2 =
          detail::DataType<format>::decompress(buffer_[(writePtr_ + offsetIntegral + D::kBase + 2) & kMask]);
      const float c = (x1 - xm1) * 0.5f;
      const float v = x0 - x1;
      const float w = c + v;
      const float a = w + v + (x2 - x0) * 0.5f;
      const float bNeg = w + a;
      const float x = (((a * t) - bNeg) * t + c) * t + x0;
      previousRead_ = x;
      accumulator_ += x * scale;
    }

   private:
    float accumulator_ = 0.0f;
    float previousRead_ = 0.0f;
    std::array<float, 2> lfoValue_ {0.0f, 0.0f};
    T* buffer_ = nullptr;
    int32_t writePtr_ = 0;
  };

  void setLfoFrequency(LFOIndex index, float frequencyPerSample) {
    lfo_[static_cast<size_t>(index)].init(frequencyPerSample);
  }

  void start(Context* c) {
    --writePtr_;
    if (writePtr_ < 0) {
      writePtr_ += static_cast<int32_t>(size);
    }

    c->accumulator_ = 0.0f;
    c->previousRead_ = 0.0f;
    c->buffer_ = buffer_;
    c->writePtr_ = writePtr_;

    if ((writePtr_ & 31) == 0) {
      c->lfoValue_[0] = lfo_[0].next();
      c->lfoValue_[1] = lfo_[1].next();
    } else {
      c->lfoValue_[0] = lfo_[0].value();
      c->lfoValue_[1] = lfo_[1].value();
    }
  }

 private:
  static constexpr int32_t kMask = static_cast<int32_t>(size - 1);

  int32_t writePtr_ = 0;
  T* buffer_ = nullptr;
  std::array<detail::CosineOscillator, 2> lfo_ {};
};

}  // namespace miverb
