#include <portaudio.h>

#include "Audio.h"
#include <chrono>
#include <iostream>
#include <thread>

class AudioContext {
    PaStream* stream;

  public:
    Ref(StereoSignal) output = nullptr;
    Ref(Clock) clock = nullptr;
    Ref(MonoSignal) pitch = nullptr;
    Ref(MonoSignal) gate = nullptr;

    AudioContext(float rate, float frames) {
        Pa_Initialize();
        Pa_OpenDefaultStream(&stream, 0, 2, paFloat32, rate, frames, &AudioContext::callback, this);
        Pa_StartStream(stream);
        clock = New(Clock);

        float updatesPerSecond = 44100 / frames;
        float updateTimeSeconds = 1 / updatesPerSecond;
        float updateTimeMicroseconds = updateTimeSeconds * 1000;
        std::cout << "Updates per second: " << updatesPerSecond << std::endl;
        std::cout << "Update time: " << updateTimeMicroseconds << "ms" << std::endl;
    }

    ~AudioContext() {
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
        Pa_Terminate();
    }
    static int callback(const void* input, void* output, unsigned long frameCount,
        const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags,
        void* userData) {
        AudioContext* context = (AudioContext*)userData;
        if (context->output == nullptr) return paContinue;

        auto start = std::chrono::high_resolution_clock::now();

        float* out = (float*)output;
        for (int i = 0; i < frameCount; i++) {
            context->clock.get()->value += 1 / 44100.0f;
            stereo sample = context->output->process();

            // Clip to [-1, 1]
            sample.first = std::min(std::max(sample.first, -1.0f), 1.0f);
            sample.second = std::min(std::max(sample.second, -1.0f), 1.0f);

            *out++ = sample.first;
            *out++ = sample.second;
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        // Note: This value should be less than the update time (@44.1KhZ, 1024 frames, ~23us)
        std::cout << "Callback took " << duration.count() << "us" << std::endl;

        return paContinue;
    }
};

int main() {
    AudioContext context(44100, 1024);
    // std::this_thread::sleep_for(std::chrono::seconds(4));

    auto phase = New(Phasor, context.clock);
    auto sine = New(Sine, phase, New(Constant, 440));

    auto lfo = New(StereoCombiner, New(Sine, phase, New(Constant, 1)));
    auto pan = New(Pan, sine, lfo);

    auto lpf = New(StereoSplitter,
        New(LowpassFilter, pan, New(Constant, 1000)));
    

    auto gain = New(Gain, lpf, New(Constant, .05));

    context.output = gain;

    std::cout << "\n\n\n\n\n\nPLEASE PRESS ENTER TO EXIT..." << std::endl;
    std::cin.get();
    return 0;
}