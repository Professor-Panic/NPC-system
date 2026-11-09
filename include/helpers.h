#pragma once
#include "fstream"
#include "nlohmann/json.hpp"
#include <random>
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
