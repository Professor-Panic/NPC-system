#pragma once
#include <fstream>
#include <stdexcept>
#include <string>
#include "nlohmann/json.hpp"
#include <random>
#include <algorithm>
#include <cmath>
#include "rng.h"
using json = nlohmann::json;

inline json LoadJson(const std::string& path)
{
    std::ifstream file(path);

    if (!file.is_open())
        throw std::runtime_error("Failed to open JSON file");

    json data;
    file >> data;

    return data;
}

inline int GetRandomIntValue(int min, int max)
{
    return std::uniform_int_distribution<int>(min, max)(Rng());
}

inline float GetRandomFloatValue(float min, float max)
{
    return std::uniform_real_distribution<float>(min, max)(Rng());
}

inline int GetRandomIntValueBiased(int min, int max, float bias = 0.5f)
{
    bias = std::clamp(bias, 0.0f, 1.0f);

    float random = std::uniform_real_distribution<float>(0.0f, 1.0f)(Rng());

    float exponent = bias < 0.5f
        ? 1.0f / (1.0f + (0.5f - bias) * 18.0f)
        : 1.0f + (bias - 0.5f) * 18.0f;

    float value = bias < 0.5f
        ? std::pow(random, exponent)
        : 1.0f - std::pow(1.0f - random, exponent);

    return min + static_cast<int>(std::round(value * (max - min)));
}
