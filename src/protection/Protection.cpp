#include "../include/sv/protection/Protection.h"
#include <cmath>
#include <numbers>

using namespace sv;

DistanceProtection::Ptr DistanceProtection::create(const DistanceProtectionSettings& settings)
{
    if (!settings.isValid())
    {
        throw std::invalid_argument("Invalid distance protection settings");
    }
    return Ptr(new DistanceProtection(settings));
}

DistanceProtection::DistanceProtection(const DistanceProtectionSettings& settings)
    : settings_(settings)
{
}

DistanceProtectionResult DistanceProtection::update(const std::complex<double> voltageV, const std::complex<double> currentA)
{
    DistanceProtectionResult result;

    if (!enabled_.load(std::memory_order_acquire))
    {
        return result;
    }

    const double voltageMag = std::abs(voltageV);
    const double currentMag = std::abs(currentA);

    if (voltageMag < settings_.voltageThresholdV || currentMag < settings_.currentThresholdA)
    {
        reset();
        return result;
    }

    const std::complex<double> impedance = voltageV / currentA;
    const double impedanceMag = std::abs(impedance);
    const double impedanceAngle = std::arg(impedance);

    result.measuredImpedanceOhm = impedanceMag;
    result.measuredAngleRad = impedanceAngle;

    if (!checkDirecation(impedance))
    {
        reset();
        return result;
    }

    const auto now = std::chrono::steady_clock::now();

    if (settings_.zone1.enabled && checkZone(settings_.zone1, impedanceMag, impedanceAngle))
    {
        if (!zone1Active_.load(std::memory_order_acquire))
        {
            zone1Active_.store(true, std::memory_order_release);
            zone1StartTime_ = now;
        }

        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - zone1StartTime_);
        if (elapsed >= settings_.zone1.delay)
        {
            result.zone1Trip = true;
            result.tripTime = now;
            invokeCallBack(result);
        }
    }
    else
    {
        zone1Active_.store(false, std::memory_order_release);
    }

    if (settings_.zone2.enabled && checkZone(settings_.zone2, impedanceMag, impedanceAngle))
    {
        if (!zone2Active_.load(std::memory_order_acquire))
        {
            zone2Active_.store(true, std::memory_order_release);
            zone2StartTime_ = now;
        }

        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - zone2StartTime_);
        if (elapsed >= settings_.zone2.delay)
        {
            result.zone2Trip = true;
            result.tripTime = now;
            invokeCallBack(result);
        }
    }
    else
    {
        zone2Active_.store(false, std::memory_order_release);
    }

    if (settings_.zone3.enabled && checkZone(settings_.zone3, impedanceMag, impedanceAngle))
    {
        if (!zone3Active_.load(std::memory_order_acquire))
        {
            zone3Active_.store(true, std::memory_order_release);
            zone3StartTime_ = now;
        }

        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - zone3StartTime_);
        if (elapsed >= settings_.zone3.delay)
        {
            result.zone3Trip = true;
            result.tripTime = now;
            invokeCallBack(result);
        }
    }
    else
    {
        zone3Active_.store(false, std::memory_order_release);
    }

    return result;
}

void DistanceProtection::reset()
{
    zone1Active_.store(false, std::memory_order_release);
    zone2Active_.store(false, std::memory_order_release);
    zone3Active_.store(false, std::memory_order_release);
}

void DistanceProtection::setSettings(const DistanceProtectionSettings& settings)
{
    if (!settings.isValid())
    {
        throw std::invalid_argument("Invalid distance protection settings");
    }
    std::lock_guard<std::mutex> lock(settingsMutex_);
    settings_ = settings;
}

DistanceProtectionSettings DistanceProtection::getSettings() const
{
    std::lock_guard<std::mutex> lock(settingsMutex_);
    return settings_;
}

void DistanceProtection::setEnabled(const bool enabled) noexcept
{
    enabled_.store(enabled, std::memory_order_release);
    if (!enabled)
    {
        reset();
    }
}

bool DistanceProtection::isEnabled() const noexcept
{
    return enabled_.load(std::memory_order_acquire);
}

void DistanceProtection::onTrip(ProtectionTripCallback callback)
{
    std::lock_guard<std::mutex> lock(callbackMutex_);
    callback_ = std::move(callback);
}

bool DistanceProtection::checkZone(const DistanceZone& zone, const double impedance, const double angle) const noexcept
{
    if (!zone.enabled || impedance > zone.reachOhm)
    {
        return false;
    }

    const double normalizedAngle = std::fmod(std::abs(angle), 2.0 * std::numbers::pi);
    return normalizedAngle <= zone.angleRad || normalizedAngle >= (2.0 * std::numbers::pi - zone.angleRad);
}

bool DistanceProtection::checkDirecation(const std::complex<double> impedance) const noexcept
{
    if (settings_.directionForward)
    {
        return impedance.real() > 0.0;
    }
    else
    {
        return impedance.real() < 0.0;
    }
}

void DistanceProtection::invokeCallBack(const DistanceProtectionResult& result)
{
    std::lock_guard<std::mutex> lock(callbackMutex_);
    if (callback_)
    {
        callback_(result);
    }
}

DifferentialProtection::Ptr DifferentialProtection::create(const DifferentialProtectionSettings& settings)
{
    if (!settings.isValid())
    {
        throw std::invalid_argument("Invalid differential protection settings");
    }
    return Ptr(new DifferentialProtection(settings));
}

DifferentialProtection::DifferentialProtection(const DifferentialProtectionSettings& settings)
    : settings_(settings)
{
}

DifferentialProtectionResult DifferentialProtection::update(const std::complex<double> current1A, const std::complex<double> current2A)
{
    DifferentialProtectionResult result;

    if (!enabled_.load(std::memory_order_acquire))
    {
        return result;
    }

    const std::complex<double> operatingCurrent = current1A - current2A;
    const std::complex<double> restraintCurrentComplex = (current1A + current2A) * 0.5;

    const double operatingMag = std::abs(operatingCurrent);
    const double restraintMag = std::abs(restraintCurrentComplex);

    result.operatingCurrentA = operatingMag;
    result.restraintCurrentA = restraintMag;

    if (operatingMag >= settings_.instantaneousThresholdA)
    {
        result.trip = true;
        result.instantaneous = true;
        result.tripTime = std::chrono::steady_clock::now();

        std::lock_guard<std::mutex> lock(callbackMutex_);
        if (callback_)
        {
            callback_(result);
        }
        return result;
    }

    if (checkCharacteristic(operatingMag, restraintMag))
    {
        result.trip = true;
        result.instantaneous = false;
        result.tripTime = std::chrono::steady_clock::now();

        std::lock_guard<std::mutex> lock(callbackMutex_);
        if (callback_)
        {
            callback_(result);
        }
    }

    return result;
}

void DifferentialProtection::reset()
{
}

void DifferentialProtection::setSettings(const DifferentialProtectionSettings& settings)
{
    if (!settings.isValid())
    {
        throw std::invalid_argument("Invalid differential protection settings");
    }
    std::lock_guard<std::mutex> lock(settingsMutex_);
    settings_ = settings;
}

DifferentialProtectionSettings DifferentialProtection::getSettings() const
{
    std::lock_guard<std::mutex> lock(settingsMutex_);
    return settings_;
}

void DifferentialProtection::setEnabled(const bool enabled) noexcept
{
    enabled_.store(enabled, std::memory_order_release);
}

bool DifferentialProtection::isEnabled() const noexcept
{
    return enabled_.load(std::memory_order_acquire);
}

void DifferentialProtection::onTrip(std::function<void(const DifferentialProtectionResult&)> callback)
{
    std::lock_guard<std::mutex> lock(callbackMutex_);
    callback_ = std::move(callback);
}

bool DifferentialProtection::checkCharacteristic(const double operating, const double restraint) const noexcept
{
    if (operating < settings_.minOperatingCurrentA)
    {
        return false;
    }

    if (restraint < settings_.minRestraintCurrentA)
    {
        return operating >= settings_.minOperatingCurrentA;
    }

    const double slopeThreshold = restraint * (settings_.slopePercent / 100.0);
    return operating >= slopeThreshold;
}
