#pragma once

#include <cstdint>
#include <random>

// ============================================================
// SHARED RANDOM NUMBER GENERATOR
// ============================================================
// One generator for the whole program. Everything random
// (names, ages, skills, death rolls, UUIDs) draws from it, so
// calling SeedRandom(n) once at startup makes a run repeatable.

inline std::mt19937_64& Rng()
{
    static std::mt19937_64 gen{ std::random_device{}() };
    return gen;
}

// Fixed seed: same seed + same code = same world.
inline void SeedRandom(std::uint64_t seed)
{
    Rng().seed(seed);
}

// Random seed, returned so you can print it and replay a run
// you want to investigate.
inline std::uint64_t SeedRandomFromDevice()
{
    std::random_device rd;

    const std::uint64_t seed =
        (static_cast<std::uint64_t>(rd()) << 32) | rd();

    SeedRandom(seed);
    return seed;
}
