#include "sv/model/IedClient.h"
#include "sv/network/NetworkReceiver.h"
#include "sv/core/logging.h"
#include <iostream>
#include <mutex>
#include <utility>

using namespace sv;

IedClient::Ptr IedClient::create(IedModel::Ptr model, const std::string& interface)
{
    std::string iface = interface;
    if (iface.empty())
    {
        iface = getFirstEthernetInterface();
        if (iface.empty())
        {
            LOG_ERROR("No suitable Ethernet interface found");
            return nullptr;
        }
    }
    return std::shared_ptr<IedClient>(new IedClient(std::move(model), iface));
}

IedClient::IedClient(IedModel::Ptr model, std::string interface)
    : model_(std::move(model))
    , interface_(std::move(interface))
{
}

void IedClient::start()
{
    start([this](const ASDU& asdu)
    {
        std::lock_guard<std::mutex> lock(receivedMutex_);
        receivedASDUs_.push_back(asdu);
        std::cout << "Received ASDU: " << asdu.svID << " with " << asdu.dataSet.size() << " values" << std::endl;
    });
}


void IedClient::start(const ASDUCallback& callback)
{
    try
    {
        ASSERT(callback, "Callback is null");

        if (!receiver_)
        {
            receiver_ = EthernetNetworkReceiver::create(interface_);
        }

        receiver_->start(callback);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in start: " + std::string(e.what()));
    }
}

void IedClient::stop() const
{
    try
    {
        if (receiver_)
        {
            receiver_->stop();
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in stop: " + std::string(e.what()));
    }
}

std::vector<ASDU> IedClient::receiveSampledValues()
{
    try
    {
        std::lock_guard<std::mutex> lock(receivedMutex_);
        std::vector<ASDU> result = std::move(receivedASDUs_);
        receivedASDUs_.clear();
        return result;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in receiveSampledValues: " + std::string(e.what()));
        return {};
    }
}

IedModel::Ptr IedClient::getModel() const
{
    return model_;
}