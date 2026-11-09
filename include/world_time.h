#pragma once
struct WorldTime {
    int day = 1;
    int month = 1;
    int year = 867;
};
inline long long GetTotalDays(const WorldTime& date)
{
    return static_cast<long long>(date.year) * 360
         + static_cast<long long>(date.month - 1) * 30
         + (date.day - 1);
}

inline bool IsDateBefore(
    const WorldTime& a,
    const WorldTime& b)
{
    return GetTotalDays(a) < GetTotalDays(b);
}

inline bool IsDateBeforeOrEqual(
    const WorldTime& a,
    const WorldTime& b)
{
    return GetTotalDays(a) <= GetTotalDays(b);
}