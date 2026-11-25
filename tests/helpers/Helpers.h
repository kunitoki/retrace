/*
  ==============================================================================

   Retrace - shared test helpers.

   Small, header-only utilities shared by the tests: a factory that hands back a
   prepared processor, a one-call processing wrapper and a note renderer, so
   individual tests stay focused on the behaviour under test.

  ==============================================================================
*/

#pragma once

#include "RetraceProcessor.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace test
{

inline constexpr float sampleRate = 48000.0f;
inline constexpr int blockSize = 512;

//==============================================================================
/** Returns a processor prepared like a host prepares it.

    It plays the renderer role, the one the MCP server renders audio with, so no test
    opens a network port.
*/
inline std::unique_ptr<RetraceProcessor> makePreparedProcessor()
{
    auto processor = std::make_unique<RetraceProcessor> (RetraceProcessor::Role::renderer);
    processor->setPlaybackConfiguration (sampleRate, blockSize);
    return processor;
}

/** Renders a single block through the processor with the given MIDI events. */
inline void renderBlock (RetraceProcessor& processor, yup::AudioBuffer<float>& audio, yup::MidiBuffer& midi)
{
    yup::ParameterChangeBuffer params;
    yup::AudioProcessContext<float> context { audio, midi, params };
    processor.processBlock (context);
}

//==============================================================================
/** What a stretch of rendered audio looked like. */
struct Rendered
{
    float peak = 0.0f;  /**< The largest absolute sample across both channels. */
    bool finite = true; /**< False when any sample was NaN or infinite. */
};

/** Folds a block into the result. */
inline void measureBlock (const yup::AudioBuffer<float>& audio, Rendered& rendered)
{
    for (int channel = 0; channel < audio.getNumChannels(); ++channel)
    {
        const auto* data = audio.getReadPointer (channel);

        for (int sample = 0; sample < audio.getNumSamples(); ++sample)
        {
            if (! std::isfinite (data[sample]))
                rendered.finite = false;
            else
                rendered.peak = std::max (rendered.peak, std::abs (data[sample]));
        }
    }
}

/** Renders a number of blocks, holding a note from the first one when noteNumber is not negative. */
inline Rendered renderBlocks (RetraceProcessor& processor, int numBlocks, int noteNumber = -1)
{
    yup::AudioBuffer<float> audio (2, blockSize);
    yup::MidiBuffer midi;
    Rendered rendered;

    for (int block = 0; block < numBlocks; ++block)
    {
        if (block == 0 && noteNumber >= 0)
            midi.addEvent (yup::MidiMessage::noteOn (1, noteNumber, 0.8f), 0);

        renderBlock (processor, audio, midi);
        measureBlock (audio, rendered);
    }

    return rendered;
}

//==============================================================================
/** Returns the value of every parameter, in the order the processor lists them. */
inline std::vector<float> parameterValues (const RetraceProcessor& processor)
{
    std::vector<float> values;

    for (const auto& parameter : processor.getParameters())
        values.push_back (parameter->getValue());

    return values;
}

/** Returns the whole patch as presets store it, as text. */
inline std::string patchText (RetraceProcessor& processor)
{
    const auto xml = processor.createPatch().createXml();
    return xml != nullptr ? xml->toString().toStdString() : std::string();
}

} // namespace test
