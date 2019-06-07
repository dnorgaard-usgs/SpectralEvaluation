#include "catch.hpp"
#include <SpectralEvaluation/Evaluation/RatioEvaluation.h>
#include <SpectralEvaluation/Evaluation/BasicScanEvaluationResult.h>

using namespace Evaluation;

TEST_CASE("IsSuitableScanForRatioEvaluation", "[RatioEvaluation][IsSuitableScanForRatioEvaluation]")
{
    RatioEvaluationSettings settings;
    BasicScanEvaluationResult scanResult;

    // TODO: Continue here...
    SECTION("Empty result - returns false")
    {
        REQUIRE(false == IsSuitableScanForRatioEvaluation(settings, scanResult));
    }

}