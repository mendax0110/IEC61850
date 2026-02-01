#pragma once

#include <complex>
#include <memory>
#include <functional>
#include <chrono>
#include <atomic>
#include <mutex>
#include <numbers>

/// @brief sv namespace \namespace sv
namespace sv
{
    /// @brief Distance Zone structure for protection settings \struct DistanceZone
    struct DistanceZone
    {
        double reachOhm{10.0};
        double angleRad{1.047};
        std::chrono::microseconds delay{0};
        bool enabled{true};

        /**
         * @brief Validates the distance zone settings
         * @return true if settings are valid
         */
        constexpr bool isValid() const noexcept
        {
            return reachOhm > 0.0 && angleRad >= 0.0 && angleRad <= std::numbers::pi;
        }
    };

    /// @brief Distance Protection Settings structure \struct DistanceProtectionSettings
    struct DistanceProtectionSettings
    {
        DistanceZone zone1;
        DistanceZone zone2;
        DistanceZone zone3;
        double voltageThresholdV{20.0};
        double currentThresholdA{0.5};
        bool directionForward{true};

        /**
         * @brief Default constructor initializing zones with default values
         */
        DistanceProtectionSettings()
        {
            zone1.reachOhm = 10.0;
            zone1.angleRad = 1.047;
            zone1.delay = std::chrono::microseconds(0);

            zone2.reachOhm = 20.0;
            zone2.angleRad = 1.047;
            zone2.delay = std::chrono::microseconds(300);

            zone3.reachOhm = 30.0;
            zone3.angleRad = 1.047;
            zone3.delay = std::chrono::microseconds(600);
        }

        /**
         * @brief Validates the distance protection settings
         * @return true if settings are valid
         */
        bool isValid() const noexcept
        {
            return zone1.isValid() && zone2.isValid() && zone3.isValid() &&
                   voltageThresholdV > 0.0 && currentThresholdA > 0.0;
        }
    };

    /// @brief Distance Protection Result structure \struct DistanceProtectionResult
    struct DistanceProtectionResult
    {
        bool zone1Trip{false};
        bool zone2Trip{false};
        bool zone3Trip{false};
        double measuredImpedanceOhm{0.0};
        double measuredAngleRad{0.0};
        std::chrono::steady_clock::time_point tripTime;
    };

    /// @brief Protection trip callback type \typedef ProtectionTripCallback
    using ProtectionTripCallback = std::function<void(const DistanceProtectionResult&)>;

    /// @brief Distance Protection class \class DistanceProtection
    class DistanceProtection
    {
    public:
        using Ptr = std::shared_ptr<DistanceProtection>;

        /**
         * @brief Creates a new DistanceProtection instance
         * @param settings The distance protection settings
         * @return A shared pointer to the created DistanceProtection
         */
        [[nodiscard]] static Ptr create(const DistanceProtectionSettings& settings = DistanceProtectionSettings());

        /**
         * @brief Destructor
         */
        ~DistanceProtection() = default;

        DistanceProtection(const DistanceProtection&) = delete;
        DistanceProtection& operator=(const DistanceProtection&) = delete;
        DistanceProtection(DistanceProtection&&) noexcept = delete;
        DistanceProtection& operator=(DistanceProtection&&) noexcept = delete;

        /**
         * @brief Updates the protection with new voltage and current measurements
         * @param voltageV The complex voltage in volts
         * @param currentA The complex current in amperes
         * @return A DistanceProtectionResult with trip information
         */
        [[nodiscard]] DistanceProtectionResult update(std::complex<double> voltageV, std::complex<double> currentA);

        /**
         * @brief Resets the internal state of the protection
         */
        void reset();

        /**
         * @brief Sets the distance protection settings
         * @param settings The new distance protection settings
         */
        void setSettings(const DistanceProtectionSettings& settings);

        /**
         * @brief Gets the current distance protection settings
         * @return The distance protection settings
         */
        [[nodiscard]] DistanceProtectionSettings getSettings() const;

        /**
         * @brief Sets whether the protection is enabled
         * @param enabled true to enable, false to disable
         */
        void setEnabled(bool enabled) noexcept;

        /**
         * @brief Checks if the protection is enabled
         * @return true if enabled, false otherwise
         */
        [[nodiscard]] bool isEnabled() const noexcept;

        /**
         * @brief Registers a callback for trip events
         * @param callback The callback function to register
         */
        void onTrip(ProtectionTripCallback callback);

    private:
        /**
         * @brief Constructor is private.
         * @param settings The distance protection settings
         */
        explicit DistanceProtection(const DistanceProtectionSettings& settings);

        /**
         * @brief Checks the zone..
         * @param zone The distance zone to check
         * @param impedance The measured impedance
         * @param angle The measured angle
         * @return A bool indicating if the zone is tripped
         */
        [[nodiscard]] bool checkZone(const DistanceZone& zone, double impedance, double angle) const noexcept;

        /**
         * @brief Checks the direction of the fault.
         * @param impedance The measured impedance
         * @return A bool indicating if the direction is correct
         */
        [[nodiscard]] bool checkDirecation(std::complex<double> impedance) const noexcept;

        /**
         * @brief Invokes the registered callback with the result.
         * @param result The distance protection result
         */
        void invokeCallBack(const DistanceProtectionResult& result);

        DistanceProtectionSettings settings_;
        mutable std::mutex settingsMutex_;

        std::atomic<bool> enabled_{true};
        std::chrono::steady_clock::time_point zone1StartTime_;
        std::chrono::steady_clock::time_point zone2StartTime_;
        std::chrono::steady_clock::time_point zone3StartTime_;

        std::atomic<bool> zone1Active_{false};
        std::atomic<bool> zone2Active_{false};
        std::atomic<bool> zone3Active_{false};

        ProtectionTripCallback callback_;
        std::mutex callbackMutex_;
    };

    /// @brief Differential Protection Settings structure \struct DifferentialProtectionSettings
    struct DifferentialProtectionSettings
    {
        double slopePercent{25.0};
        double minOperatingCurrentA{0.3};
        double minRestraintCurrentA{1.0};
        double instantaneousThresholdA{10.0};
        bool enabled{true};

        /**
         * @brief Validates the differential protection settings
         * @return true if settings are valid
         */
        constexpr bool isValid() const noexcept
        {
            return slopePercent > 0.0 && slopePercent <= 100.0 &&
                    minOperatingCurrentA > 0.0 && minRestraintCurrentA > 0.0 &&
                    instantaneousThresholdA > 0.0;
        }
    };

    /// @brief Differential Protection Result structure \struct DifferentialProtectionResult
    struct DifferentialProtectionResult
    {
        bool trip{false};
        double operatingCurrentA{0.0};
        double restraintCurrentA{0.0};
        bool instantaneous{false};
        std::chrono::steady_clock::time_point tripTime;
    };

    /// @brief Differential Protection class \class DifferentialProtection
    class DifferentialProtection
    {
    public:
        using Ptr = std::shared_ptr<DifferentialProtection>;

        /**
         * @brief Creates a new DifferentialProtection instance
         * @param settings The differential protection settings
         * @return A shared pointer to the created DifferentialProtection
         */
        [[nodiscard]] static Ptr create(const DifferentialProtectionSettings& settings = DifferentialProtectionSettings());

        /**
         * @brief Destructor
         */
        ~DifferentialProtection() = default;

        DifferentialProtection(const DifferentialProtection&) = delete;
        DifferentialProtection& operator=(const DifferentialProtection&) = delete;
        DifferentialProtection(DifferentialProtection&&) noexcept = delete;
        DifferentialProtection& operator=(DifferentialProtection&&) noexcept = delete;

        /**
         * @brief Updates the protection with new current measurements from both sides
         * @param current1A The complex current from side 1 in amperes
         * @param current2A The complex current from side 2 in amperes
         * @return A DifferentialProtectionResult with trip information
         */
        [[nodiscard]] DifferentialProtectionResult update(std::complex<double> current1A, std::complex<double> current2A);


        /**
         * @brief Resets the internal state of the protection
         */
        void reset();

        /**
         * @brief Sets the differential protection settings
         * @param settings The new differential protection settings
         */
        void setSettings(const DifferentialProtectionSettings& settings);

        /**
         * @brief Gets the current differential protection settings
         * @return The differential protection settings
         */
        [[nodiscard]] DifferentialProtectionSettings getSettings() const;

        /**
         * @brief Sets whether the protection is enabled
         * @param enabled true to enable, false to disable
         */
        void setEnabled(bool enabled) noexcept;

        /**
         * @brief Checks if the protection is enabled
         * @return true if enabled, false otherwise
         */
        [[nodiscard]] bool isEnabled() const noexcept;

        /**
         * @brief Registers a callback for trip events
         * @param callback The callback function to register
         */
        void onTrip(std::function<void(const DifferentialProtectionResult&)> callback);

    private:
        /**
         * @brief Constructor is private.
         * @param settings The differential protection settings
         */
        explicit DifferentialProtection(const DifferentialProtectionSettings& settings);

        /**
         * @brief Checks the characteristic curve.
         * @param operating The operating current
         * @param restraint The restraint current
         * @return A bool indicating if the characteristic is met
         */
        [[nodiscard]] bool checkCharacteristic(double operating, double restraint) const noexcept;

        DifferentialProtectionSettings settings_;
        mutable std::mutex settingsMutex_;

        std::atomic<bool> enabled_{true};

        std::function<void(const DifferentialProtectionResult&)> callback_;
        std::mutex callbackMutex_;
    };

}