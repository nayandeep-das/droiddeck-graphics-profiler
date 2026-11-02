
#include "hypothesis_engine.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

WaylandLogSample makeSample(
    const std::string& timestamp,
    double fps,
    long long gpuFrames,
    double renderAvg,
    double acquireAvg,
    double fenceAvg,
    double releaseAvg)
{
    WaylandLogSample sample;

    sample.timestamp = timestamp;
    sample.hasStatsData = true;
    sample.hasPerfData = true;
    sample.displayedFps = fps;
    sample.gpuFramesFromGames = gpuFrames;
    sample.framesOnScreen = static_cast<long long>(fps * 10);
    sample.windowsOpen = 1;
    sample.surfaceActive = true;

    sample.renderSceneAvgMs = renderAvg;
    sample.acquireAvgMs = acquireAvg;
    sample.fenceWaitAvgMs = fenceAvg;
    sample.releaseAvgMs = releaseAvg;

    return sample;
}

const DiagnosticHypothesis& findHypothesis(
    const HypothesisAnalysis& analysis,
    HypothesisType type)
{
    for (const auto& hypothesis : analysis.hypotheses) {
        if (hypothesis.type == type) {
            return hypothesis;
        }
    }

    assert(false && "Expected hypothesis was not found");
    return analysis.hypotheses.front();
}

void testEmptyInput()
{
    const auto result = analyzeHypotheses({});

    assert(result.analyzedSamples == 0);
    assert(result.criticalSamples == 0);
    assert(result.hypotheses.size() == 5);

    for (const auto& hypothesis : result.hypotheses) {
        assert(hypothesis.evidenceScore == 0.0);
        assert(hypothesis.supportingEvidence.empty());
    }
}

void testFrameProductionEvidence()
{
    std::vector<WaylandLogSample> samples;

    for (int i = 0; i < 4; ++i) {
        samples.push_back(makeSample(
            "10:00:0" + std::to_string(i),
            5.0, 50, 2.0, 0.1, 0.2, 2.0));
    }

    for (int i = 0; i < 4; ++i) {
        samples.push_back(makeSample(
            "10:01:0" + std::to_string(i),
            35.0, 700, 6.0, 0.2, 4.0, 10.0));
    }

    const auto result = analyzeHypotheses(samples);
    const auto& hypothesis =
        findHypothesis(result, HypothesisType::FrameProduction);

    assert(result.analyzedSamples == 8);
    assert(result.criticalSamples == 4);
    assert(!hypothesis.supportingEvidence.empty());
    assert(hypothesis.evidenceScore > 0.0);
}

void testReleaseTimingContradictoryEvidence()
{
    std::vector<WaylandLogSample> samples;

    for (int i = 0; i < 3; ++i) {
        samples.push_back(makeSample(
            "10:00:0" + std::to_string(i),
            5.0, 50, 2.0, 0.1, 0.2, 2.0));
    }

    for (int i = 0; i < 3; ++i) {
        samples.push_back(makeSample(
            "10:01:0" + std::to_string(i),
            35.0, 700, 6.0, 0.2, 4.0, 10.0));
    }

    const auto result = analyzeHypotheses(samples);
    const auto& hypothesis =
        findHypothesis(result, HypothesisType::ReleaseTiming);

    assert(!hypothesis.contradictoryEvidence.empty());
}

void testRenderBottleneckContradictoryEvidence()
{
    std::vector<WaylandLogSample> samples;

    for (int i = 0; i < 3; ++i) {
        samples.push_back(makeSample(
            "10:00:0" + std::to_string(i),
            5.0, 50, 2.0, 0.1, 0.2, 2.0));
    }

    for (int i = 0; i < 3; ++i) {
        samples.push_back(makeSample(
            "10:01:0" + std::to_string(i),
            35.0, 700, 6.0, 0.2, 4.0, 10.0));
    }

    const auto result = analyzeHypotheses(samples);
    const auto& hypothesis =
        findHypothesis(result, HypothesisType::RenderBottleneck);

    assert(!hypothesis.contradictoryEvidence.empty());
}

void testLifecycleWithoutTransitions()
{
    std::vector<WaylandLogSample> samples;

    for (int i = 0; i < 3; ++i) {
        samples.push_back(makeSample(
            "10:00:0" + std::to_string(i),
            5.0, 50, 2.0, 0.1, 0.2, 2.0));
    }

    for (int i = 0; i < 3; ++i) {
        samples.push_back(makeSample(
            "10:01:0" + std::to_string(i),
            35.0, 700, 6.0, 0.2, 4.0, 10.0));
    }

    const auto result = analyzeHypotheses(samples);
    const auto& hypothesis =
        findHypothesis(result, HypothesisType::PresentationLifecycle);

    assert(!hypothesis.contradictoryEvidence.empty());
}

void testInsufficientGameplayGroups()
{
    std::vector<WaylandLogSample> samples;

    samples.push_back(makeSample(
        "10:00:00", 5.0, 50, 2.0, 0.1, 0.2, 2.0));

    samples.push_back(makeSample(
        "10:00:10", 15.0, 200, 3.0, 0.1, 0.3, 3.0));

    const auto result = analyzeHypotheses(samples);

    assert(result.analyzedSamples == 2);
    assert(result.criticalSamples == 1);
}

void testHypothesisSorting()
{
    std::vector<WaylandLogSample> samples;

    for (int i = 0; i < 4; ++i) {
        samples.push_back(makeSample(
            "10:00:0" + std::to_string(i),
            5.0, 50, 2.0, 0.1, 0.2, 2.0));
    }

    for (int i = 0; i < 4; ++i) {
        samples.push_back(makeSample(
            "10:01:0" + std::to_string(i),
            35.0, 700, 6.0, 0.2, 4.0, 10.0));
    }

    const auto result = analyzeHypotheses(samples);

    for (std::size_t i = 1; i < result.hypotheses.size(); ++i) {
        assert(
            result.hypotheses[i - 1].evidenceScore >=
            result.hypotheses[i].evidenceScore);
    }
}

int main()
{
    testEmptyInput();
    testFrameProductionEvidence();
    testReleaseTimingContradictoryEvidence();
    testRenderBottleneckContradictoryEvidence();
    testLifecycleWithoutTransitions();
    testInsufficientGameplayGroups();
    testHypothesisSorting();

    std::cout << "All hypothesis engine tests passed.\n";
    return 0;
}
