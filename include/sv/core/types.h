#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>
#include "mac.h"

/// @brief sv namespace \namespace sv
namespace sv
{
    /// @brief IEC 61850 SV Constants
    constexpr uint16_t SV_ETHER_TYPE = 0x88BA;
    constexpr uint16_t DEFAULT_APP_ID = 0x4000;
    constexpr uint16_t DEFAULT_SMP_RATE = 4000;
    constexpr uint16_t APP_ID_MIN = 0x4000;
    constexpr uint16_t APP_ID_MAX = 0x7FFF;
    constexpr uint16_t VLAN_TAG_TPID = 0x8100;
    constexpr size_t MAX_ASDUS_PER_MESSAGE = 8;
    constexpr size_t VALUES_PER_ASDU = 8;

    /// @brief SamplesPerPeriod enumeration representing supported samples per period. \enum SamplesPerPeriod
    enum class SamplesPerPeriod : uint16_t
    {
        SPP_80 = 80,
        SPP_256 = 256,
    };

    /// @brief SignalFrequency enumeration representing supported signal frequencies. \enum SignalFrequency
    enum class SignalFrequency : uint16_t
    {
        FREQ_16_7_HZ = 167,
        FREQ_25_HZ = 250,
        FREQ_50_HZ = 500,
        FREQ_60_HZ = 600
    };

    /// @brief SmpSynch enumeration representing sample synchronization types. \enum SmpSynch
    enum class SmpSynch : uint8_t
    {
        None = 0,
        Local = 1,
        Global = 2
    };

    /// @brief DataType enumeration representing supported data types. \enum DataType
    enum class DataType : uint8_t
    {
        INT32,
        UINT32,
        FLOAT32
    };

    /// @brief Timestamp type representing nanosecond precision time points.
    using Timestamp = std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>;

    /// @brief Quality structure representing data quality flags. \struct Quality
    struct Quality
    {
        uint8_t validity : 2{};
        bool overflow : 1{};
        bool outOfRange : 1{};
        bool badReference : 1{};
        bool oscillatory : 1{};
        bool failure : 1{};
        bool oldData : 1{};
        bool inconsistent : 1{};
        bool inaccurate : 1{};
        bool source : 1{};
        bool test : 1{};
        bool operatorBlocked : 1{};
        bool derived : 1{};
        uint32_t reserved : 18{};

        /**
         * @brief Constructs Quality from raw uint32_t value.
         * @param raw The raw uint32_t value.
         */
        explicit Quality(uint32_t raw = 0) { *reinterpret_cast<uint32_t*>(this) = raw; }

        /**
         * @brief Converts Quality to raw uint32_t value.
         * @return A uint32_t representing the raw quality flags.
         */
        [[nodiscard]] uint32_t toRaw() const { return *reinterpret_cast<const uint32_t*>(this); }

        /**
         * @brief Checks if the quality is good (validity == 0).
         * @return True if quality is good, false otherwise.
         */
        [[nodiscard]] bool isGood() const { return validity == 0; }
    };

    static_assert(sizeof(Quality) == 4, "Quality struct must be 4 bytes");

    /// @brief AnalogValue structure representing an analog value with quality. \struct AnalogValue
    struct AnalogValue
    {
        std::variant<int32_t, uint32_t, float> value;
        Quality quality;

        /**
         * @brief Gets the scaled integer representation of the value.
         * @return The value as int32_t, or 0 if type is unsupported.
         */
        [[nodiscard]] int32_t getScaledInt() const
        {
            if (auto* v = std::get_if<int32_t>(&value)) return *v;
            if (auto* v = std::get_if<uint32_t>(&value)) return static_cast<int32_t>(*v);
            if (auto* v = std::get_if<float>(&value)) return static_cast<int32_t>(*v);
            return 0;
        }

        /**
         * @brief Gets the float representation of the value.
         * @return The value as float, or 0.0f if type is unsupported.
         */
        [[nodiscard]] float getFloat() const
        {
            if (auto* v = std::get_if<float>(&value)) return *v;
            if (auto* v = std::get_if<int32_t>(&value)) return static_cast<float>(*v);
            if (auto* v = std::get_if<uint32_t>(&value)) return static_cast<float>(*v);
            return 0.0f;
        }
    };

    /// @brief ASDU structure representing an Application Service Data Unit. \struct ASDU
    struct ASDU
    {
        std::string svID;
        uint16_t smpCnt;
        uint32_t confRev;
        SmpSynch smpSynch;
        std::vector<AnalogValue> dataSet;
        std::optional<std::array<uint8_t, 8>> gmIdentity;
        Timestamp timestamp;

        /**
         * @brief Validates the ASDU structure.
         * @return True if valid, false otherwise.
         */
        [[nodiscard]] bool isValid() const
        {
            return !svID.empty() && svID.length() >= 2 && dataSet.size() == VALUES_PER_ASDU;
        }
    };

    /// @brief SVMessage structure representing a Sampled Values message. \struct SVMessage
    struct SVMessage
    {
        std::array<uint8_t, 6> destMac;
        std::array<uint8_t, 6> srcMac;
        uint8_t userPriority{};
        uint16_t vlanID{};
        uint16_t appID{};
        uint16_t length{};
        bool simulate;
        std::vector<ASDU> asdus;

        /**
         * @brief Validates the SVMessage structure.
         * @return True if valid, false otherwise.
         */
        [[nodiscard]] bool isValid() const
        {
            if (asdus.empty() || asdus.size() > MAX_ASDUS_PER_MESSAGE) return false;
            if (appID < APP_ID_MIN || appID > APP_ID_MAX) return false;
            for (const auto& asdu : asdus)
            {
                if (!asdu.isValid()) return false;
            }
            return true;
        }
    };

    /// @brief ScalingFactors structure defining default scaling factors. \struct ScalingFactors
    struct ScalingFactors
    {
        static constexpr int32_t CURRENT_DEFAULT = 1000;
        static constexpr int32_t VOLTAGE_DEFAULT = 100;
    };

    /// @brief PublisherConfig structure representing configuration for an SV publisher. \struct PublisherConfig
    struct PublisherConfig
    {
        std::array<uint8_t, 6> destMac = sv::MacAddress::svMulticastBase().bytes();
        uint8_t userPriority = 4;
        uint16_t vlanID = 0;
        uint16_t appID = DEFAULT_APP_ID;
        bool simulate = false;
        SmpSynch smpSynch = SmpSynch::None;
        int32_t currentScaling = ScalingFactors::CURRENT_DEFAULT;
        int32_t voltageScaling = ScalingFactors::VOLTAGE_DEFAULT;
        DataType dataType = DataType::INT32;
    };

    /// @brief SubscriberConfig structure representing configuration for an SV subscriber. \struct SubscriberConfig
    struct SubscriberConfig
    {
        uint16_t appID = DEFAULT_APP_ID;
        std::string svID;
        int32_t currentScaling = ScalingFactors::CURRENT_DEFAULT;
        int32_t voltageScaling = ScalingFactors::VOLTAGE_DEFAULT;
        DataType dataType = DataType::INT32;
    };

    /**
     * @brief Converts SmpSynch enum to string.
     * @param synch The SmpSynch value.
     * @return A string representation of the SmpSynch.
     */
    inline std::string smpSynchToString(const SmpSynch synch)
    {
        switch (synch)
        {
            case SmpSynch::None: return "None";
            case SmpSynch::Local: return "Local";
            case SmpSynch::Global: return "Global";
            default: return "Unknown";
        }
    }

    /**
     * @brief Converts DataType enum to string.
     * @param type The DataType value.
     * @return A string representation of the DataType.
     */
    inline std::string dataTypeToString(const DataType type)
    {
        switch (type)
        {
            case DataType::INT32: return "INT32";
            case DataType::UINT32: return "UINT32";
            case DataType::FLOAT32: return "FLOAT32";
            default: return "Unknown";
        }
    }

    /**
     * @brief Converts SamplesPerPeriod enum to string.
     * @param spp The SamplesPerPeriod value.
     * @return A string representation of the SamplesPerPeriod.
     */
    inline std::string samplesPerPeriodToString(const SamplesPerPeriod spp)
    {
        switch (spp)
        {
            case SamplesPerPeriod::SPP_80: return "80";
            case SamplesPerPeriod::SPP_256: return "256";
            default: return "Unknown";
        }
    }

    /**
     * @brief Converts SignalFrequency enum to string.
     * @param freq The SignalFrequency value.
     * @return A string representation of the SignalFrequency.
     */
    inline std::string signalFrequencyToString(const SignalFrequency freq)
    {
        switch (freq)
        {
            case SignalFrequency::FREQ_16_7_HZ: return "16.7 Hz";
            case SignalFrequency::FREQ_25_HZ: return "25 Hz";
            case SignalFrequency::FREQ_50_HZ: return "50 Hz";
            case SignalFrequency::FREQ_60_HZ: return "60 Hz";
            default: return "Unknown";
        }
    }
}