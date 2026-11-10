#pragma once
#include <algorithm>
#include <cmath>
#include <initializer_list>
#include "uuid.h"
#include "helpers.h"
#include <span>
inline float Saturate(float t){return std::clamp(t,0.0f,1.0f);}
inline float Normalise(float value, float min, float max)
{
    if (max <= min) return 0.0f;   // degenerate range: decide once, up front
    return std::clamp((value - min) / (max - min), 0.0f, 1.0f);
}
inline float Linear(float t){
    return Saturate(t);
}
inline float Inverse(float t){
    return 1.0f-Saturate(t);
}
inline float Pow(float t,float exponent){
   return std::pow(Saturate(t),exponent);
}
inline float Logistic(float t,float steepness,float midpoint){
    return 1 / (1 + std::exp(-steepness * (Saturate(t) - midpoint)));
}
inline float PeakTriangle(float t,float centre,float width){
    t=1 - std::abs(Saturate(t) - centre)/ width;
    return std::max(0.0f,t);
}
inline float PeakGaussian(float t, float centre, float width)
{
    if (width <= 0.0f)                       // degenerate: only the exact centre scores
        return t == centre ? 1.0f : 0.0f;

    const float d = (Saturate(t) - centre) / width;

    return std::exp(-0.5f * d * d);
}
enum class CombineMode
{
    Raw,            // plain product, no compensation
    ModFactor,      // Dave Mark's modification factor
    GeometricMean   // product ^ (1/n)
};

// Combines 0-1 consideration scores into a single 0-1 score.
// Any 0 vetoes the action in every mode.
inline float Combine(std::span<const float> scores,
                     CombineMode mode = CombineMode::ModFactor)
{
    // No considerations means nothing recommends the action, so it
    // should never be chosen by accident.
    if (scores.empty())
        return 0.0f;

    float product = 1.0f;

    for (float s : scores)
    {
        s = Saturate(s);

        if (s == 0.0f)           // veto: skip the rest of the work
            return 0.0f;

        product *= s;
    }

    const float n = static_cast<float>(scores.size());

    switch (mode)
    {
    case CombineMode::ModFactor:
    {
        const float modFactor = 1.0f - 1.0f / n;
        const float makeUp    = (1.0f - product) * modFactor;
        return product + makeUp * product;
    }

    case CombineMode::GeometricMean:
        return std::pow(product, 1.0f / n);

    case CombineMode::Raw:
    default:
        return product;
    }
}

// Convenience so you can write Combine({a, b, c}).
inline float Combine(std::initializer_list<float> scores,
                     CombineMode mode = CombineMode::ModFactor)
{
    return Combine(std::span<const float>(scores.begin(), scores.size()), mode);
}
struct ScoredAction
{
    Uuid  id;
    float score = 0.0f;
};

// Chooses one action from the scored candidates.
//   topN      - only the N best-scoring actions can be chosen.
//   sharpness - weight = score^sharpness. 1 = proportional to score,
//               higher = favours the best action more strongly.
// Returns Uuid{} (IsNull() == true) if nothing qualifies.
inline Uuid PickAction(std::vector<ScoredAction> candidates,
                       int topN = 3,
                       float sharpness = 1.0f)
{
    // Drop vetoed entries. Written as !(score > 0) so NaN is dropped too.
    candidates.erase(
        std::remove_if(candidates.begin(), candidates.end(),
            [](const ScoredAction& c) { return !(c.score > 0.0f); }),
        candidates.end());

    if (candidates.empty() || topN <= 0)
        return {};

    // Best first. stable_sort keeps input order among ties, so the
    // same inputs always give the same result (matters for seeds).
    std::stable_sort(candidates.begin(), candidates.end(),
        [](const ScoredAction& a, const ScoredAction& b)
        { return a.score > b.score; });

    if (candidates.size() > static_cast<std::size_t>(topN))
        candidates.resize(static_cast<std::size_t>(topN));

    // Weighted random pick over what's left.
    float total = 0.0f;

    for (const ScoredAction& c : candidates)
        total += std::pow(c.score, sharpness);

    if (!(total > 0.0f))
        return {};

    float roll = GetRandomFloatValue(0.0f, total);

    for (const ScoredAction& c : candidates)
    {
        roll -= std::pow(c.score, sharpness);

        if (roll < 0.0f)
            return c.id;
    }

    // Float rounding can leave a sliver of roll unspent; the last
    // candidate is the correct answer in that case.
    return candidates.back().id;
}