/*
  ==============================================================================

   Retrace - audio benchmarks.

   These benchmarks time the audio callback of the processor. Run them with
   `just bench` (or build the `retrace_benchmarks` target and execute it
   directly to pass google/benchmark arguments).

  ==============================================================================
*/

#include <benchmark/benchmark.h>

#include <yup_events/yup_events.h>

int main (int argc, char** argv)
{
    // The processor posts to the message thread, so it needs one to exist.
    const yup::ScopedYupInitialiser_GUI yupInitialiser;

    benchmark::Initialize (&argc, argv);

    if (benchmark::ReportUnrecognizedArguments (argc, argv))
        return 1;

    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}
