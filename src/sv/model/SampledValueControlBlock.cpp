#include "sv/model/SampledValueControlBlock.h"
#include <utility>
#include <memory>

using namespace sv;

SampledValueControlBlock::Ptr SampledValueControlBlock::create(const std::string& name)
{
    return std::make_shared<SampledValueControlBlock>(name);
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

void SampledValueControlBlock::setAppId(uint16_t appId)
{
    appId_ = appId;
}

uint16_t SampledValueControlBlock::getAppId() const
{
    return appId_;
}

void SampledValueControlBlock::setSmpRate(uint16_t rate)
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