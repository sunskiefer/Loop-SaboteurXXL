// JuceHeader.h — LoopSaboteurXXL's stand-in for the few JUCE pieces Loop Saboteur's engine uses.
//
// Loop Saboteur (clickysteve/Loop-Saboteur) is a JUCE 8 plugin, but its DSP only leans on JUCE for small helpers:
// jlimit/jmin/jmax, MathConstants, Random, AudioBuffer, ScopedNoDenormals, a play-head query, String/StringArray for
// labels, and the AudioProcessorValueTreeState for its knob values. MPC OS has no JUCE to link against, so this
// header provides exactly those, with JUCE's own behaviour where it matters to the sound:
//   - Random is JUCE's 48-bit LCG (same nextInt/nextFloat/nextBool/nextDouble maths);
//   - ScopedNoDenormals sets flush-to-zero like JUCE does (ARM FPSCR FZ, x86 MXCSR FTZ/DAZ);
//   - AudioProcessorValueTreeState keeps one atomic float per parameter (getRawParameterValue) and calls the
//     listeners synchronously on a set, as JUCE's does from setValueNotifyingHost.
// Anything else from JUCE was removed from the vendored sources (see third_party/loop-saboteur/VENDORED.md).
#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

#define JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(cls) \
    cls (const cls&) = delete;                            \
    cls& operator= (const cls&) = delete;

namespace juce
{
using int64 = long long;
using uint32 = unsigned int;
using uint64 = unsigned long long;

template <typename T> constexpr T jmax (T a, T b) { return a < b ? b : a; }
template <typename T> constexpr T jmin (T a, T b) { return b < a ? b : a; }
template <typename T> constexpr T jmax (T a, T b, T c) { return jmax (jmax (a, b), c); }
template <typename T> constexpr T jmin (T a, T b, T c) { return jmin (jmin (a, b), c); }
template <typename T> constexpr T jlimit (T lo, T hi, T v) { return v < lo ? lo : (hi < v ? hi : v); }
template <typename T> inline int roundToInt (T v) { return (int) std::lround ((double) v); }

template <typename T> struct MathConstants
{
    static constexpr T pi = static_cast<T> (3.141592653589793238L);
    static constexpr T twoPi = static_cast<T> (2 * 3.141592653589793238L);
    static constexpr T halfPi = static_cast<T> (3.141592653589793238L / 2);
    static constexpr T euler = static_cast<T> (2.71828182845904523536L);
    static constexpr T sqrt2 = static_cast<T> (1.4142135623730950488L);
};

// ---------------------------------------------------------------------------------------------- String
// A thin std::string. Only what the vendored engine uses: build from text/numbers, compare, concatenate.
class String
{
public:
    String() = default;
    String (const char* s) : s_ (s ? s : "") {}
    String (const std::string& s) : s_ (s) {}
    explicit String (int v) : s_ (std::to_string (v)) {}
    explicit String (long long v) : s_ (std::to_string (v)) {}
    explicit String (double v, int decimals = 0)
    {
        char b[64];
        if (decimals > 0) std::snprintf (b, sizeof b, "%.*f", decimals, v);
        else std::snprintf (b, sizeof b, "%g", v);
        s_ = b;
    }
    explicit String (float v, int decimals = 0) : String ((double) v, decimals) {}
    static String charToString (char c) { return String (std::string (1, c)); }

    const char* toRawUTF8() const noexcept { return s_.c_str(); }
    const std::string& toStdString() const noexcept { return s_; }
    bool isEmpty() const noexcept { return s_.empty(); }
    bool isNotEmpty() const noexcept { return ! s_.empty(); }
    int length() const noexcept { return (int) s_.size(); }

    bool operator== (const String& o) const noexcept { return s_ == o.s_; }
    bool operator!= (const String& o) const noexcept { return s_ != o.s_; }
    bool operator== (const char* o) const noexcept { return o != nullptr && s_ == o; }
    bool operator!= (const char* o) const noexcept { return ! (*this == o); }
    String& operator+= (const String& o) { s_ += o.s_; return *this; }
    String& operator+= (const char* o) { s_ += o; return *this; }
    friend String operator+ (String a, const String& b) { a += b; return a; }
    friend String operator+ (String a, const char* b) { a += b; return a; }
    friend String operator+ (const char* a, const String& b) { return String (a) + b; }

private:
    std::string s_;
};

inline bool operator== (const char* a, const String& b) noexcept { return b == a; }

class StringArray
{
public:
    StringArray() = default;
    StringArray (std::initializer_list<const char*> l) { for (auto* s : l) a_.push_back (String (s)); }
    int size() const noexcept { return (int) a_.size(); }
    const String& operator[] (int i) const noexcept
    {
        static const String empty;
        return (i >= 0 && i < (int) a_.size()) ? a_[(size_t) i] : empty;
    }
    void add (const String& s) { a_.push_back (s); }

private:
    std::vector<String> a_;
};

// ---------------------------------------------------------------------------------------------- Random
// JUCE's Random: a 48-bit linear congruential generator (juce_Random.cpp).
class Random
{
public:
    Random() : seed_ (1) { setSeed ((int64) 0x5f3759df ^ (int64) (uintptr_t) this); }
    explicit Random (int64 seed) : seed_ (seed) {}
    void setSeed (int64 s) noexcept { seed_ = s; }
    int nextInt() noexcept
    {
        seed_ = (int64) (((((uint64) seed_) * 0x5deece66dULL) + 11) & 0xffffffffffffULL);
        return (int) (seed_ >> 16);
    }
    int nextInt (int maxValue) noexcept
    {
        if (maxValue <= 0) return 0;
        return (int) ((((uint64) (uint32) nextInt()) * (uint64) maxValue) >> 32);
    }
    float nextFloat() noexcept
    {
        auto r = (uint32) nextInt();
        auto f = (float) r / (float) 4294967296.0;   // [0, 1)
        return f < 1.0f ? f : std::nextafter (1.0f, 0.0f);
    }
    double nextDouble() noexcept { return (double) (uint32) nextInt() / 4294967296.0; }
    bool nextBool() noexcept { return (nextInt() & 0x40000000) != 0; }

private:
    int64 seed_;
};

// ---------------------------------------------------------------------------------------------- audio
template <typename T> class AudioBuffer
{
public:
    AudioBuffer() = default;
    AudioBuffer (int ch, int n) { setSize (ch, n); }
    // keepExisting / clearExtra / avoidRealloc are accepted for JUCE's signature; contents always start cleared.
    void setSize (int ch, int n, bool = false, bool = false, bool = false)
    {
        chans_ = jmax (0, ch);
        n_ = jmax (0, n);
        data_.assign ((size_t) chans_ * (size_t) n_, T());
        ptrs_.resize ((size_t) chans_);
        for (int c = 0; c < chans_; ++c) ptrs_[(size_t) c] = data_.data() + (size_t) c * (size_t) n_;
    }
    // A view on caller-owned channel pointers (the engine's per-block float buffers).
    void setDataToReferTo (T* const* chans, int ch, int n)
    {
        data_.clear();
        chans_ = ch;
        n_ = n;
        ptrs_.assign (chans, chans + ch);
    }
    int getNumChannels() const noexcept { return chans_; }
    int getNumSamples() const noexcept { return n_; }
    void clear() noexcept { for (int c = 0; c < chans_; ++c) std::fill (ptrs_[(size_t) c], ptrs_[(size_t) c] + n_, T()); }
    const T* getReadPointer (int c) const noexcept { return ptrs_[(size_t) c]; }
    T* getWritePointer (int c) noexcept { return ptrs_[(size_t) c]; }
    T* const* getArrayOfWritePointers() noexcept { return ptrs_.data(); }
    const T* const* getArrayOfReadPointers() const noexcept { return ptrs_.data(); }
    T getSample (int c, int i) const noexcept { return ptrs_[(size_t) c][i]; }
    void setSample (int c, int i, T v) noexcept { ptrs_[(size_t) c][i] = v; }
    void copyFrom (int destCh, int destStart, const AudioBuffer& src, int srcCh, int srcStart, int num) noexcept
    {
        if (&src == this && destCh == srcCh && destStart == srcStart) return;
        std::memmove (ptrs_[(size_t) destCh] + destStart, src.ptrs_[(size_t) srcCh] + srcStart, sizeof (T) * (size_t) num);
    }

private:
    int chans_ = 0, n_ = 0;
    std::vector<T> data_;
    std::vector<T*> ptrs_;
};

struct MidiBuffer {};

namespace FloatVectorOperations
{
inline void clear (float* d, int n) noexcept { std::fill (d, d + n, 0.0f); }
}

// Flush denormals to zero for the scope, as JUCE's ScopedNoDenormals does.
class ScopedNoDenormals
{
public:
    ScopedNoDenormals() noexcept
    {
#if defined(__arm__) && defined(__ARM_PCS_VFP)
        unsigned int f;
        asm volatile ("vmrs %0, fpscr" : "=r"(f));
        saved_ = f;
        f |= (1u << 24);   // FZ
        asm volatile ("vmsr fpscr, %0" : : "r"(f));
#elif defined(__aarch64__)
        uint64_t f;
        asm volatile ("mrs %0, fpcr" : "=r"(f));
        saved_ = (unsigned int) f;
        f |= (1u << 24);
        asm volatile ("msr fpcr, %0" : : "r"(f));
#elif defined(__SSE__) || defined(__x86_64__)
        unsigned int m;
        asm volatile ("stmxcsr %0" : "=m"(m));
        saved_ = m;
        m |= 0x8040;       // FTZ | DAZ
        asm volatile ("ldmxcsr %0" : : "m"(m));
#endif
    }
    ~ScopedNoDenormals() noexcept
    {
#if defined(__arm__) && defined(__ARM_PCS_VFP)
        asm volatile ("vmsr fpscr, %0" : : "r"(saved_));
#elif defined(__aarch64__)
        uint64_t f = saved_;
        asm volatile ("msr fpcr, %0" : : "r"(f));
#elif defined(__SSE__) || defined(__x86_64__)
        asm volatile ("ldmxcsr %0" : : "m"(saved_));
#endif
    }

private:
    unsigned int saved_ = 0;
};

// ---------------------------------------------------------------------------------------------- play head
// JUCE 8's AudioPlayHead::getPosition() answers with optionals; LoopSaboteurXXL fills it from MPC's song position.
template <typename T> class Optional
{
public:
    Optional() = default;
    Optional (T v) : v_ (v), has_ (true) {}
    explicit operator bool() const noexcept { return has_; }
    const T& operator*() const noexcept { return v_; }
    const T* operator->() const noexcept { return &v_; }

private:
    T v_ {};
    bool has_ = false;
};

class AudioPlayHead
{
public:
    class PositionInfo
    {
    public:
        Optional<double> getBpm() const noexcept { return bpm; }
        Optional<double> getPpqPosition() const noexcept { return ppq; }
        bool getIsPlaying() const noexcept { return playing; }
        Optional<double> bpm, ppq;
        bool playing = false;
    };
    Optional<PositionInfo> getPosition() const noexcept { return Optional<PositionInfo> (info); }
    PositionInfo info;
};

// ---------------------------------------------------------------------------------------------- parameters
// The engine's knob values. One atomic float per parameter, in the parameter's own units; a set clamps to the
// range, rounds whole-number/choice parameters, and tells the listeners for that id at once.
class AudioProcessorValueTreeState
{
public:
    struct Listener
    {
        virtual ~Listener() = default;
        virtual void parameterChanged (const String& parameterID, float newValue) = 0;
    };

    struct ParameterSpec
    {
        const char* id;
        float minValue, maxValue, defaultValue;
        bool discrete;     // whole numbers (choice / int / bool)
    };

    class Parameter
    {
    public:
        // In JUCE these convert to/from 0..1; here the stored value is already in the parameter's units, so the
        // conversion is the identity and setValueNotifyingHost takes a real value (the engine's only caller,
        // loadSceneToKnobs, passes convertTo0to1(v) straight back in).
        float convertTo0to1 (float v) const noexcept { return v; }
        void setValueNotifyingHost (float v) { owner->setValue (index, v); }
        AudioProcessorValueTreeState* owner = nullptr;
        int index = 0;
    };

    using ParameterLayout = std::vector<ParameterSpec>;

    explicit AudioProcessorValueTreeState (const ParameterLayout& layout)
        : specs_ (layout), values_ (new std::atomic<float>[layout.size()]), params_ (layout.size())
    {
        for (size_t i = 0; i < specs_.size(); ++i)
        {
            values_[i].store (specs_[i].defaultValue);
            params_[i].owner = this;
            params_[i].index = (int) i;
        }
    }

    int indexOf (const char* id) const noexcept
    {
        for (size_t i = 0; i < specs_.size(); ++i)
            if (std::strcmp (specs_[i].id, id) == 0) return (int) i;
        return -1;
    }
    int indexOf (const String& id) const noexcept { return indexOf (id.toRawUTF8()); }
    int size() const noexcept { return (int) specs_.size(); }
    const ParameterSpec& spec (int i) const noexcept { return specs_[(size_t) i]; }

    std::atomic<float>* getRawParameterValue (const char* id) noexcept
    {
        const int i = indexOf (id);
        return i < 0 ? nullptr : &values_[(size_t) i];
    }
    Parameter* getParameter (const char* id) noexcept
    {
        const int i = indexOf (id);
        return i < 0 ? nullptr : &params_[(size_t) i];
    }

    float getValue (int i) const noexcept { return values_[(size_t) i].load(); }
    void setValue (int i, float v)
    {
        if (i < 0 || i >= (int) specs_.size()) return;
        const auto& s = specs_[(size_t) i];
        if (! std::isfinite (v)) v = s.defaultValue;
        v = jlimit (s.minValue, s.maxValue, v);
        if (s.discrete) v = std::round (v);
        values_[(size_t) i].store (v);
        for (auto& l : listeners_)
            if (l.first == i) l.second->parameterChanged (String (s.id), v);
    }
    void setValue (const char* id, float v) { setValue (indexOf (id), v); }

    void addParameterListener (const char* id, Listener* l) { listeners_.push_back ({ indexOf (id), l }); }
    void removeParameterListener (const char* id, Listener* l)
    {
        const int i = indexOf (id);
        listeners_.erase (std::remove_if (listeners_.begin(), listeners_.end(),
                                          [&] (const std::pair<int, Listener*>& p) { return p.first == i && p.second == l; }),
                          listeners_.end());
    }

private:
    std::vector<ParameterSpec> specs_;
    std::unique_ptr<std::atomic<float>[]> values_;
    std::vector<Parameter> params_;
    std::vector<std::pair<int, Listener*>> listeners_;
};

} // namespace juce

using juce::MathConstants;
