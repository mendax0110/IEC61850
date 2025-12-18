#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace sv
{

// IEC 61850 SV Constants
constexpr uint16_t SV_ETHER_TYPE = 0x88BA;
constexpr uint16_t DEFAULT_APP_ID = 0x4000;
constexpr uint16_t DEFAULT_SMP_RATE = 4000;

using Timestamp = std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>;

struct Quality
{
    bool validity : 1;
    bool overflow : 1;
    // Add more flags as needed
};

struct AnalogValue
{
    std::variant<int16_t, float> value;
    Quality quality;
};

struct ASDU
{
    std::string svID;
    uint16_t smpCnt;
    uint32_t confRev;
    bool smpSynch;
    std::vector<AnalogValue> dataSet;
    Timestamp timestamp;
};

} // namespace sv