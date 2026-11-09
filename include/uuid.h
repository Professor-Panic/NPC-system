
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <random>
#include <string>
#include <string_view>
#include <unordered_map>
#include <stdexcept>
#include <utility>

// ============================================================
// UUID
// ============================================================

struct Uuid
{
    std::uint64_t hi = 0;
    std::uint64_t lo = 0;

    bool IsNull() const
    {
        return hi == 0 && lo == 0;
    }

    friend bool operator==(const Uuid& a, const Uuid& b)
    {
        return a.hi == b.hi && a.lo == b.lo;
    }

    friend bool operator!=(const Uuid& a, const Uuid& b)
    {
        return !(a == b);
    }

    static Uuid Generate()
    {
        static std::mt19937_64 rng = [] {
            std::random_device rd;
            std::seed_seq seed{rd(), rd(), rd(), rd()};
            return std::mt19937_64(seed);
        }();

        Uuid u;

        do
        {
            u.hi = rng();
            u.lo = rng();
        }
        while (u.IsNull());

        // UUID v4 bits
        u.hi = (u.hi & 0xFFFFFFFFFFFF0FFFull)
             | 0x0000000000004000ull;

        u.lo = (u.lo & 0x3FFFFFFFFFFFFFFFull)
             | 0x8000000000000000ull;

        return u;
    }

    std::string ToString() const
    {
        if (IsNull())
            return {};

        char buf[37];

        std::snprintf(
            buf,
            sizeof(buf),
            "%08x-%04x-%04x-%04x-%012llx",
            static_cast<unsigned>(hi >> 32),
            static_cast<unsigned>((hi >> 16) & 0xFFFF),
            static_cast<unsigned>(hi & 0xFFFF),
            static_cast<unsigned>(lo >> 48),
            static_cast<unsigned long long>(lo & 0xFFFFFFFFFFFFull)
        );

        return buf;
    }

    static Uuid Parse(std::string_view s)
    {
        Uuid u;
        int digits = 0;

        for (char c : s)
        {
            if (c == '-')
                continue;

            int d;

            if (c >= '0' && c <= '9')
                d = c - '0';
            else if (c >= 'a' && c <= 'f')
                d = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F')
                d = c - 'A' + 10;
            else
                return {};

            if (digits >= 32)
                return {};

            std::uint64_t& half = digits < 16 ? u.hi : u.lo;

            half = (half << 4) | static_cast<std::uint64_t>(d);
            ++digits;
        }

        return digits == 32 ? u : Uuid{};
    }
};

// UUID hashing for unordered containers
namespace std
{
    template<>
    struct hash<Uuid>
    {
        size_t operator()(const Uuid& u) const noexcept
        {
            return static_cast<size_t>(
                u.hi ^ (u.lo * 0x9E3779B97F4A7C15ull)
            );
        }
    };
}

// ============================================================
// GENERIC RECORD STORE
// ============================================================

// All stored types must contain:
// Uuid id;

template<typename T>
class RecordStore
{
private:

    std::unordered_map<Uuid, T> records;

public:

    // Insert a record and assign UUID if necessary.
    Uuid Insert(T record)
    {
        if (record.id.IsNull())
            record.id = Uuid::Generate();

        Uuid id = record.id;

        auto [it, inserted] =
            records.emplace(id, std::move(record));

        if (!inserted)
            throw std::runtime_error("Duplicate record UUID");

        return id;
    }

    // Retrieve record
    T* Get(Uuid id)
    {
        auto it = records.find(id);

        return it == records.end()
            ? nullptr
            : &it->second;
    }

    const T* Get(Uuid id) const
    {
        auto it = records.find(id);

        return it == records.end()
            ? nullptr
            : &it->second;
    }

    // Check existence
    bool Contains(Uuid id) const
    {
        return records.contains(id);
    }

    // Remove record
    bool Remove(Uuid id)
    {
        return records.erase(id) > 0;
    }

    // Number of records
    std::size_t Size() const
    {
        return records.size();
    }

    bool Empty() const
    {
        return records.empty();
    }

    void Clear()
    {
        records.clear();
    }

    // Iteration
    auto begin() { return records.begin(); }
    auto end() { return records.end(); }

    auto begin() const { return records.begin(); }
    auto end() const { return records.end(); }
};
template<typename T>
struct IdRef
{
    Uuid id;
};