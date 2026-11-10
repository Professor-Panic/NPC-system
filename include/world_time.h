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
inline WorldTime AddMonths(WorldTime date, int months)
{
    long long totalMonths =
        static_cast<long long>(date.year) * 12 +
        (date.month - 1) + months;

    date.year = static_cast<int>(totalMonths / 12);
    date.month = static_cast<int>(totalMonths % 12) + 1;

    return date;
}

// Subtracts years while preserving the day and month.
inline WorldTime SubtractYears(WorldTime date, int years)
{
    date.year -= years;
    return date;
}
