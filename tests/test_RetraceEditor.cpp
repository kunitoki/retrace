/*
  ==============================================================================

   Retrace - editor tests.

  ==============================================================================
*/

#include <gtest/gtest.h>

#include "helpers/Helpers.h"

#include "RetraceEditor.h"

#include <memory>

TEST (RetraceEditor, OpensAtItsDesignSize)
{
    auto processor = test::makePreparedProcessor();

    std::unique_ptr<yup::AudioProcessorEditor> editor (processor->createEditor());
    ASSERT_NE (editor, nullptr);

    EXPECT_TRUE (editor->isResizable());
    EXPECT_TRUE (editor->shouldPreserveAspectRatio());
    EXPECT_EQ (editor->getPreferredSize(), (yup::Size<int> (1280, 900)));
    EXPECT_EQ (editor->getInitialSize(), editor->getPreferredSize());
    EXPECT_EQ (editor->getSize().roundToInt(), editor->getPreferredSize());

    EXPECT_EQ (processor->getEditorComponent(), editor.get());
    editor.reset();
    EXPECT_EQ (processor->getEditorComponent(), nullptr);
}

TEST (RetraceEditor, ReopensAtTheSizeItWasLeftAt)
{
    auto processor = test::makePreparedProcessor();
    const yup::Size<int> halfSize (640, 450);

    {
        std::unique_ptr<yup::AudioProcessorEditor> editor (processor->createEditor());
        ASSERT_NE (editor, nullptr);

        editor->setSize (halfSize.to<float>());
    }

    EXPECT_EQ (processor->getViewOptions().editorSize, halfSize);

    std::unique_ptr<yup::AudioProcessorEditor> reopened (processor->createEditor());
    ASSERT_NE (reopened, nullptr);

    EXPECT_EQ (reopened->getInitialSize(), halfSize);
    EXPECT_EQ (reopened->getSize().roundToInt(), halfSize);
}
