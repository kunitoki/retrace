/*
  ==============================================================================

   Retrace - audio processor tests.

  ==============================================================================
*/

#include <gtest/gtest.h>

#include "helpers/Helpers.h"

#include <set>
#include <string>

namespace
{
constexpr int middleC = 60;
constexpr int blocksPerHalfSecond = 47;
} // namespace

TEST (RetraceProcessor, ParametersHaveUniqueIdsAndStartInRange)
{
    RetraceProcessor processor (RetraceProcessor::Role::renderer);

    const auto parameters = processor.getParameters();
    ASSERT_FALSE (parameters.empty());

    std::set<std::string> ids;
    std::set<yup::uint32> hostIds;

    for (const auto& parameter : parameters)
    {
        const auto id = parameter->getID().toStdString();
        SCOPED_TRACE (id);

        EXPECT_FALSE (id.empty());
        EXPECT_TRUE (ids.insert (id).second) << "duplicate parameter id";
        EXPECT_TRUE (hostIds.insert (parameter->getHostParameterID()).second) << "duplicate host parameter id";

        EXPECT_GE (parameter->getValue(), parameter->getMinimumValue());
        EXPECT_LE (parameter->getValue(), parameter->getMaximumValue());
        EXPECT_EQ (processor.getParameterByID (parameter->getID()).get(), parameter.get());
    }
}

TEST (RetraceProcessor, ReportsCapabilities)
{
    RetraceProcessor processor (RetraceProcessor::Role::renderer);

    EXPECT_TRUE (processor.hasEditor());
    EXPECT_TRUE (processor.supportsDataTreeState());
    EXPECT_GT (processor.getNumVoices(), 0);
    EXPECT_EQ (processor.getMCPServer(), nullptr);
}

TEST (RetraceProcessor, SilentWithoutNotes)
{
    auto processor = test::makePreparedProcessor();

    const auto rendered = test::renderBlocks (*processor, 8);

    EXPECT_TRUE (rendered.finite);
    EXPECT_FLOAT_EQ (rendered.peak, 0.0f);
}

TEST (RetraceProcessor, NoteOnProducesFiniteSound)
{
    auto processor = test::makePreparedProcessor();

    const auto rendered = test::renderBlocks (*processor, 16, middleC);

    EXPECT_TRUE (rendered.finite);
    EXPECT_GT (rendered.peak, 0.01f);
    EXPECT_EQ (processor->getNumActiveVoices(), 1);
}

TEST (RetraceProcessor, FlushReleasesHeldNotes)
{
    auto processor = test::makePreparedProcessor();

    ASSERT_GT (test::renderBlocks (*processor, 4, middleC).peak, 0.0f);

    processor->flush();

    // A note cut short fades over a few milliseconds instead of clicking, within one block.
    const auto fade = test::renderBlocks (*processor, 1);
    EXPECT_TRUE (fade.finite);
    EXPECT_EQ (processor->getNumActiveVoices(), 0);

    const auto rendered = test::renderBlocks (*processor, 4);

    EXPECT_TRUE (rendered.finite);
    EXPECT_FLOAT_EQ (rendered.peak, 0.0f);
}

TEST (RetraceProcessor, EveryFactoryPresetPlaysFiniteSound)
{
    const auto numPresets = RetraceProcessor (RetraceProcessor::Role::renderer).getNumPresets();
    ASSERT_GT (numPresets, 1) << "the factory bank is not embedded";

    for (int index = 0; index < numPresets; ++index)
    {
        // A processor of its own, so no tail of the preset before is taken for sound.
        auto processor = test::makePreparedProcessor();
        SCOPED_TRACE (processor->getPresetName (index).toStdString());

        processor->setCurrentPreset (index);
        ASSERT_EQ (processor->getCurrentPreset(), index);

        const auto rendered = test::renderBlocks (*processor, blocksPerHalfSecond, middleC);

        EXPECT_TRUE (rendered.finite);
        EXPECT_GT (rendered.peak, 0.0f);
    }
}

TEST (RetraceProcessor, StateRoundTripRestoresThePatch)
{
    auto processor = test::makePreparedProcessor();
    ASSERT_GT (processor->getNumPresets(), 1);

    processor->setCurrentPreset (processor->getNumPresets() - 1);

    const auto expectedValues = test::parameterValues (*processor);
    const auto expectedPatch = test::patchText (*processor);
    ASSERT_FALSE (expectedPatch.empty());

    yup::DataTree state;
    ASSERT_TRUE (processor->saveStateIntoDataTree (state).wasOk());

    processor->setCurrentPreset (0);
    ASSERT_NE (test::parameterValues (*processor), expectedValues);

    ASSERT_TRUE (processor->loadStateFromDataTree (state).wasOk());

    EXPECT_EQ (test::parameterValues (*processor), expectedValues);
    EXPECT_EQ (test::patchText (*processor), expectedPatch);
    EXPECT_EQ (processor->getCurrentPreset(), processor->getNumPresets() - 1);
}
