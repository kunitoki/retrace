/*
  ==============================================================================

   Retrace - audio processor benchmarks.

  ==============================================================================
*/

#include <benchmark/benchmark.h>

#include "RetraceProcessor.h"

#include <array>

namespace
{
constexpr float sampleRate = 48000.0f;
constexpr std::array<int, 4> chord { 48, 55, 60, 64 };

/** Returns the index of a factory preset by name, or -1. */
int findPreset (const RetraceProcessor& processor, const char* name)
{
    for (int index = 0; index < processor.getNumPresets(); ++index)
        if (processor.getPresetName (index) == name)
            return index;

    return -1;
}

//==============================================================================
/** Times the audio callback while a four note chord of a factory preset is held.

    The realtime counter is how many seconds of audio one second of processing renders:
    below 1 the preset cannot play this chord in real time on this machine.
*/
void BM_ProcessBlock (benchmark::State& state, const char* presetName)
{
    const auto blockSize = static_cast<int> (state.range (0));

    RetraceProcessor processor (RetraceProcessor::Role::renderer);

    const auto presetIndex = findPreset (processor, presetName);

    if (presetIndex < 0)
    {
        state.SkipWithError ("no such factory preset");
        return;
    }

    processor.setCurrentPreset (presetIndex);
    processor.setPlaybackConfiguration (sampleRate, blockSize);

    yup::AudioBuffer<float> audio (2, blockSize);
    yup::MidiBuffer midi;
    yup::ParameterChangeBuffer params;
    yup::AudioProcessContext<float> context { audio, midi, params };

    for (const auto note : chord)
        midi.addEvent (yup::MidiMessage::noteOn (1, note, 0.8f), 0);

    // The note ons, and whatever the first block allocates, stay out of the measure.
    processor.processBlock (context);

    for (auto _ : state)
    {
        processor.processBlock (context);
        benchmark::DoNotOptimize (audio.getReadPointer (0));
    }

    const auto samples = static_cast<double> (state.iterations()) * static_cast<double> (blockSize);
    state.SetItemsProcessed (static_cast<yup::int64> (samples));
    state.counters["realtime"] = benchmark::Counter (samples / static_cast<double> (sampleRate), benchmark::Counter::kIsRate);
}
} // namespace

BENCHMARK_CAPTURE (BM_ProcessBlock, Init, "Init")->Arg (64)->Arg (512);
BENCHMARK_CAPTURE (BM_ProcessBlock, FMEPiano, "FM E-Piano")->Arg (64)->Arg (512);
BENCHMARK_CAPTURE (BM_ProcessBlock, SuperSawLead, "Super Saw Lead")->Arg (64)->Arg (512);
BENCHMARK_CAPTURE (BM_ProcessBlock, CinematicStrings, "Cinematic Strings")->Arg (64)->Arg (512);
BENCHMARK_CAPTURE (BM_ProcessBlock, NeuroReese, "Neuro Reese")->Arg (64)->Arg (512);
