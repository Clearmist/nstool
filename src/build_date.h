#pragma once
#include <cstring>
#include <string>

// Parse __DATE__ macro to ISO 8601 format (YYYY-MM-DD) at runtime
// __DATE__ format is "Mmm dd yyyy" (e.g., "Dec  9 2025")

inline std::string getISOBuildDate()
{
    const char *date = __DATE__;

    // Parse month
    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    int month = 0;

    for (int i = 0; i < 12; i++)
    {
        if (strncmp(date, months[i], 3) == 0)
        {
            month = i + 1;
            break;
        }
    }

    // Parse day (positions 4 and 5). May have a leading space.
    int day = (date[4] == ' ' ? 0 : (date[4] - '0') * 10) + (date[5] - '0');

    // Parse year (positions 7-10).
    int year = (date[7] - '0') * 1000 + (date[8] - '0') * 100 + (date[9] - '0') * 10 + (date[10] - '0');

    // Format as YYYY-MM-DD.
    char buffer[11];

    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d", year, month, day);

    return std::string(buffer);
}

// Get cached ISO build date
inline const std::string &BUILD_DATE_ISO()
{
    static const std::string iso_date = getISOBuildDate();

    return iso_date;
}
