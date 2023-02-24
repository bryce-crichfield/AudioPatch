#pragma once
#include "Memory.h"
#include <cmath>
#include <iostream>
#include <utility>

using mono = float;
using stereo = std::pair<float, float>;

Def(MonoSignal) { virtual mono process() = 0; };
Def(StereoSignal) { virtual stereo process() = 0; };

Def(Reader), MonoSignal {
    mono& value;

    Reader(mono & value) : value(value) {}

    mono process() override { return value; }
};

Def(Constant), MonoSignal {
    mono value;

    Constant(mono value) : value(value) {}

    mono process() override { return value; }
};

Def(Phasor), MonoSignal {
    Ref(MonoSignal) time;

    Phasor(Ref(MonoSignal) time) : time(time) {}

    mono process() override {
        float p = 2 * M_PI * time->process();
        return p;
    }
};

Def(Clock), MonoSignal {
    // Simple a float that is incremented by the audio thread
    mono value = 0;

    mono process() override {
        // TODO: This might be necessary?
        // if (value > 1) value -= 1;
        return value;
    }
};

Def(Sine), StereoSignal {
    Ref(MonoSignal) phase;
    Ref(MonoSignal) pitch;

    Sine(Ref(MonoSignal) phase, Ref(MonoSignal) pitch) : phase(phase), pitch(pitch) {}

    stereo process() override {
        float p = phase->process();
        float f = pitch->process();
        float s = sin(p * f);
        return {s, s};
    }
};

Def(Saw), StereoSignal {
    Ref(MonoSignal) phase;
    Ref(MonoSignal) pitch;

    Saw(Ref(MonoSignal) phase, Ref(MonoSignal) pitch) : phase(phase), pitch(pitch) {}

    stereo process() override {
        float p = phase->process();
        float f = pitch->process();
        float s = 2 * (p * f - floor(p * f + 0.5));
        return {s, s};
    }
};

Def(Gain), StereoSignal {
    Ref(StereoSignal) input;
    Ref(MonoSignal) gain;

    Gain(Ref(StereoSignal) input, Ref(MonoSignal) gain) : input(input), gain(gain) {}

    stereo process() override {
        auto [left, right] = input->process();
        auto g = gain->process();
        return {left * g, right * g};
    }
};

Def(Pan), StereoSignal {
    Ref(StereoSignal) input;
    Ref(MonoSignal) pan;

    Pan(Ref(StereoSignal) input, Ref(MonoSignal) pan) : input(input), pan(pan) {}

    stereo process() override {
        auto [left, right] = input->process();
        auto p = pan->process();
        return {left * (1 - p), right * p};
    }
};

Def(StereoCombiner), MonoSignal {
    Ref(StereoSignal) input;

    StereoCombiner(Ref(StereoSignal) input) : input(input) {}

    mono process() override {
        auto [left, right] = input->process();
        return (left + right) / 2;
    }
};

Def(StereoSplitter), StereoSignal {
    Ref(MonoSignal) input;

    StereoSplitter(Ref(MonoSignal) input) : input(input) {}

    stereo process() override {
        auto s = input->process();
        return {s, s};
    }
};

// Provides a Boolean signal that is true every N seconds
Def(Timer), MonoSignal {
    Ref(MonoSignal) time;
    float last;
    float elapsed;
    float hz;

    Timer(Ref(MonoSignal) time, float hz) : time(time), last(0), elapsed(0), hz(hz) {}

    mono process() override {
        float current = time->process();
        float delta = current - last;
        last = current;

        float period = 1.0f / hz;

        elapsed += delta;
        if (elapsed > period) {
            elapsed -= period;
            return 1;
        } else {
            return 0;
        }
    }
};

Def(BiquadFilter), MonoSignal {
    Ref(StereoSignal) input;
    Ref(MonoSignal) frequency;

    float a0, a1, a2, b1, b2;

    float x1, x2, y1, y2;

    virtual void recalculate(){};

    BiquadFilter(Ref(StereoSignal) input, Ref(MonoSignal) frequency)
        : input(input), frequency(frequency) {
        recalculate();
    }

    mono process() override {
        recalculate();

        auto [left, right] = input->process();

        float x0 = (left + right) / 2;
        float y0 = a0 * x0 + a1 * x1 + a2 * x2 - b1 * y1 - b2 * y2;

        x2 = x1;
        x1 = x0;
        y2 = y1;
        y1 = y0;

        return y0;
    }
};

Def(LowpassFilter), BiquadFilter {

    LowpassFilter(Ref(StereoSignal) input, Ref(MonoSignal) frequency) :
        BiquadFilter(input, frequency){}

    void recalculate() override {
        float f = frequency->process();

        float theta = 2 * M_PI * f;
        float g = cos(theta) / (1.0 + sin(theta));
        a0 = (1 - g) / 2;
        a1 = (1 - g) / 2;
        a2 = 0;
        b1 = -g;
        b2 = 0;
    }
};
