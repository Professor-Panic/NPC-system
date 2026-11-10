
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
#include <vector>
#include "rng.h"

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
        // Uses the shared RNG so seeded runs produce the same UUIDs.
        Uuid u;

        do
        {
            u.hi = Rng()();
            u.lo = Rng()();
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
// REFERENCES AND PAIR KEYS
// ============================================================

template<typename T>
struct IdRef
{
    Uuid id;
};

// Identifies a relationship between two characters.
// Ordered: (A, B) != (B, A).
struct PairKey
{
    Uuid a;
    Uuid b;

    friend bool operator==(const PairKey& x, const PairKey& y)
    {
        return x.a == y.a && x.b == y.b;
    }

    // Smaller UUID first, so Unordered(x, y) == Unordered(y, x).
    static PairKey Unordered(Uuid x, Uuid y)
    {
        const bool xFirst =
            x.hi < y.hi || (x.hi == y.hi && x.lo <= y.lo);

        return xFirst ? PairKey{x, y} : PairKey{y, x};
    }
};

namespace std
{
    template<>
    struct hash<PairKey>
    {
        size_t operator()(const PairKey& k) const noexcept
        {
            const size_t h1 = hash<Uuid>{}(k.a);
            const size_t h2 = hash<Uuid>{}(k.b);

            return h1 ^ (h2 + 0x9E3779B97F4A7C15ull
                            + (h1 << 6) + (h1 >> 2));
        }
    };
}

// ============================================================
// RECORD STORE
// ============================================================

// How a store indexes records by character pair.
//   None      - no pair index; look up by UUID or predicate only.
//   Directed  - one record per ordered pair (A,B) != (B,A).
//               Use for opinions.
//   Symmetric - one record per unordered pair (A,B) == (B,A).
//               Use for mutual relations like friendship.
//
// Directed / Symmetric require T to contain:
//   IdRef<...> chara_a;  IdRef<...> chara_b;
// and allow only ONE record per pair, so marriages
// (which can repeat for the same couple) stay on None.
enum class PairMode
{
    None,
    Directed,
    Symmetric
};

// All stored types must contain:
// Uuid id;

template<typename T, PairMode Mode = PairMode::None>
class RecordStore
{
private:
    std::unordered_map<Uuid, T> records;
    std::unordered_map<PairKey, Uuid> pairIndex; // unused when Mode == None

    static PairKey MakeKey(Uuid a, Uuid b)
    {
        return Mode == PairMode::Symmetric
            ? PairKey::Unordered(a, b)
            : PairKey{a, b};
    }

public:

    // Insert a record and assign UUID if necessary.
    Uuid Insert(T record)
    {
        if (record.id.IsNull())
            record.id = Uuid::Generate();

        const Uuid id = record.id;

        if constexpr (Mode != PairMode::None)
        {
            const PairKey key =
                MakeKey(record.chara_a.id, record.chara_b.id);

            if (pairIndex.contains(key))
                throw std::runtime_error("Duplicate pair record");

            auto [it, inserted] =
                records.emplace(id, std::move(record));

            if (!inserted)
                throw std::runtime_error("Duplicate record UUID");

            pairIndex.emplace(key, id);
        }
        else
        {
            auto [it, inserted] =
                records.emplace(id, std::move(record));

            if (!inserted)
                throw std::runtime_error("Duplicate record UUID");
        }

        return id;
    }

    // Retrieve record by UUID.
    T* Get(Uuid id)
    {
        auto it = records.find(id);
        return it == records.end() ? nullptr : &it->second;
    }

    const T* Get(Uuid id) const
    {
        auto it = records.find(id);
        return it == records.end() ? nullptr : &it->second;
    }

    // ---- Pair access (only compiles for Directed / Symmetric) ----

    // O(1) lookup by character pair. nullptr if none exists.
    T* GetPair(Uuid a, Uuid b)
    {
        static_assert(Mode != PairMode::None,
            "GetPair requires PairMode::Directed or Symmetric");

        auto it = pairIndex.find(MakeKey(a, b));
        return it == pairIndex.end() ? nullptr : Get(it->second);
    }

    const T* GetPair(Uuid a, Uuid b) const
    {
        static_assert(Mode != PairMode::None,
            "GetPair requires PairMode::Directed or Symmetric");

        auto it = pairIndex.find(MakeKey(a, b));
        return it == pairIndex.end() ? nullptr : Get(it->second);
    }

    // Returns the pair's record, creating a default one if missing.
    T& GetOrCreate(Uuid a, Uuid b)
    {
        static_assert(Mode != PairMode::None,
            "GetOrCreate requires PairMode::Directed or Symmetric");

        if (T* existing = GetPair(a, b))
            return *existing;

        T record{};
        record.chara_a = {a};
        record.chara_b = {b};

        return *Get(Insert(std::move(record)));
    }

    // ---- Searching ----

    // Find the first record matching a predicate.
    template<typename Predicate>
    T* Find(Predicate predicate)
    {
        for (auto& [id, record] : records)
        {
            if (predicate(record))
                return &record;
        }

        return nullptr;
    }

    template<typename Predicate>
    const T* Find(Predicate predicate) const
    {
        for (const auto& [id, record] : records)
        {
            if (predicate(record))
                return &record;
        }

        return nullptr;
    }

    // Find all records matching a predicate.
    template<typename Predicate>
    std::vector<T*> FindAll(Predicate predicate)
    {
        std::vector<T*> result;

        for (auto& [id, record] : records)
        {
            if (predicate(record))
                result.push_back(&record);
        }

        return result;
    }

    template<typename Predicate>
    std::vector<const T*> FindAll(Predicate predicate) const
    {
        std::vector<const T*> result;

        for (const auto& [id, record] : records)
        {
            if (predicate(record))
                result.push_back(&record);
        }

        return result;
    }

    // Check whether any record matches.
    template<typename Predicate>
    bool Any(Predicate predicate) const
    {
        for (const auto& [id, record] : records)
        {
            if (predicate(record))
                return true;
        }

        return false;
    }

    // Count matching records.
    template<typename Predicate>
    std::size_t Count(Predicate predicate) const
    {
        std::size_t count = 0;

        for (const auto& [id, record] : records)
        {
            if (predicate(record))
                ++count;
        }

        return count;
    }

    // ---- Bookkeeping ----

    // Check existence by UUID.
    bool Contains(Uuid id) const
    {
        return records.contains(id);
    }

    // Remove record by UUID (and its pair index entry).
    bool Remove(Uuid id)
    {
        if constexpr (Mode != PairMode::None)
        {
            auto it = records.find(id);

            if (it == records.end())
                return false;

            pairIndex.erase(
                MakeKey(it->second.chara_a.id,
                        it->second.chara_b.id));

            records.erase(it);
            return true;
        }
        else
        {
            return records.erase(id) > 0;
        }
    }

    // Number of records.
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
        pairIndex.clear();
    }

    // Iteration.
    auto begin() { return records.begin(); }
    auto end() { return records.end(); }

    auto begin() const { return records.begin(); }
    auto end() const { return records.end(); }
};
