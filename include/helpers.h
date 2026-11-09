#pragma once
#include "fstream"
#include "nlohmann/json.hpp"
#include <random>
#include <algorithm>
#include <cmath>
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
    static std::mt19937 gen{ std::random_device{}() };

    return std::uniform_int_distribution<int>(min, max)(gen);
}
inline int GetRandomFloatValue(float min, float max)
{
    static std::mt19937 gen{ std::random_device{}() };

    return std::uniform_real_distribution<float>(min, max)(gen);
}
inline int GetRandomIntValueBiased(int min, int max, float bias = 0.5f)
{
    static std::mt19937 gen{ std::random_device{}() };

    bias = std::clamp(bias, 0.0f, 1.0f);

    float random = std::uniform_real_distribution<float>(0.0f, 1.0f)(gen);

    float exponent = bias < 0.5f
        ? 1.0f / (1.0f + (0.5f - bias) * 18.0f)
        : 1.0f + (bias - 0.5f) * 18.0f;

    float value = bias < 0.5f
        ? std::pow(random, exponent)
        : 1.0f - std::pow(1.0f - random, exponent);

    return min + static_cast<int>(std::round(value * (max - min)));
}