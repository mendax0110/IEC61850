#include "sv/model/SampledValueControlBlock.h"
#include <utility>
#include <memory>

using namespace sv;

SampledValueControlBlock::Ptr SampledValueControlBlock::create(const std::string& name)
{
    return Ptr(new SampledValueControlBlock(name));
}

SampledValueControlBlock::SampledValueControlBlock(std::string name)
    : name_(std::move(name))
    , appId_(DEFAULT_APP_ID)
    , smpRate_(DEFAULT_SMP_RATE)
{
}

void SampledValueControlBlock::setMulticastAddress(const std::string& address)
{
    multicastAddress_ = address;
}

const std::string& SampledValueControlBlock::getMulticastAddress() const
{
    return multicastAddress_;
}

void SampledValueControlBlock::setAppId(const uint16_t appId)
{
    appId_ = appId;
}

uint16_t SampledValueControlBlock::getAppId() const
{
    return appId_;
}

void SampledValueControlBlock::setSmpRate(const uint16_t rate)
{
    smpRate_ = rate;
}

uint16_t SampledValueControlBlock::getSmpRate() const
{
    return smpRate_;
}

void SampledValueControlBlock::setDataSet(const std::string& dataSet)
{
    dataSet_ = dataSet;
}

const std::string& SampledValueControlBlock::getDataSet() const
{
    return dataSet_;
}

const std::string& SampledValueControlBlock::getName() const
{
    return name_;
}

void SampledValueControlBlock::setConfRev(const uint32_t revision)
{
    confRev_ = revision;
}

uint32_t SampledValueControlBlock::getConfRev() const
{
    return confRev_;
}

void SampledValueControlBlock::setSmpSynch(const SmpSynch synch)
{
    smpSynch_ = synch;
}

SmpSynch SampledValueControlBlock::getSmpSynch() const
{
    return smpSynch_;
}

void SampledValueControlBlock::setVlanId(const uint16_t vlanId)
{
    vlanId_ = vlanId;
}

uint16_t SampledValueControlBlock::getVlanId() const
{
    return vlanId_;
}

void SampledValueControlBlock::setUserPriority(const uint8_t priority)
{
    if (priority >= 1 && priority <= 7)
    {
        userPriority_ = priority;
    }
}

uint8_t SampledValueControlBlock::getUserPriority() const
{
    return userPriority_;
}

void SampledValueControlBlock::setSimulate(const bool simulate)
{
    simulate_ = simulate;
}

bool SampledValueControlBlock::getSimulate() const
{
    return simulate_;
}

void SampledValueControlBlock::setSamplesPerPeriod(const SamplesPerPeriod spp)
{
    samplesPerPeriod_ = spp;
}

SamplesPerPeriod SampledValueControlBlock::getSamplesPerPeriod() const
{
    return samplesPerPeriod_;
}

void SampledValueControlBlock::setSignalFrequency(const SignalFrequency freq)
{
    signalFrequency_ = freq;
}

SignalFrequency SampledValueControlBlock::getSignalFrequency() const
{
    return signalFrequency_;
}

void SampledValueControlBlock::setGrandmasterIdentity(const std::array<uint8_t, 8>& identity)
{
    gmIdentity_ = identity;
}

std::optional<std::array<uint8_t, 8>> SampledValueControlBlock::getGrandmasterIdentity() const
{
    return gmIdentity_;
}

void SampledValueControlBlock::clearGrandmasterIdentity()
{
    gmIdentity_.reset();
}

void SampledValueControlBlock::setDataType(const DataType type)
{
    dataType_ = type;
}

DataType SampledValueControlBlock::getDataType() const
{
    return dataType_;
}

void SampledValueControlBlock::setCurrentScaling(const int32_t factor)
{
    currentScaling_ = factor;
}

int32_t SampledValueControlBlock::getCurrentScaling() const
{
    return currentScaling_;
}

void SampledValueControlBlock::setVoltageScaling(const int32_t factor)
{
    voltageScaling_ = factor;
}

int32_t SampledValueControlBlock::getVoltageScaling() const
{
    return voltageScaling_;
}

PublisherConfig SampledValueControlBlock::toPublisherConfig() const
{
    PublisherConfig config;

    if (!multicastAddress_.empty())
    {
        const auto mac = MacAddress::tryParse(multicastAddress_);
        if (mac.has_value())
        {
            config.destMac = mac.value().bytes();
        }
    }

    config.userPriority = userPriority_;
    config.vlanID = vlanId_;
    config.appID = appId_;
    config.simulate = simulate_;
    config.smpSynch = smpSynch_;
    config.currentScaling = currentScaling_;
    config.voltageScaling = voltageScaling_;
    config.dataType = dataType_;

    return config;
}
