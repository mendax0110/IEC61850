#include "sv/model/IedServer.h"
#include "sv/model/SampledValueControlBlock.h"
#include "sv/network/NetworkReceiver.h"
#include "sv/network/NetworkSender.h"
#include "sv/core/types.h"
#include "sv/core/logging.h"
#include <atomic>
#include <chrono>
#include <iostream>
#include <utility>

using namespace sv;

IedServer::Ptr IedServer::create(IedModel::Ptr model, const std::string& interface)
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
    return std::shared_ptr<IedServer>(new IedServer(std::move(model), iface));
}

IedServer::IedServer(IedModel::Ptr model, std::string interface)
    : model_(std::move(model))
    , interface_(std::move(interface))
    , running_(false)
{
}

void IedServer::start()
{
    try
    {
        ASSERT(model_, "Model is null");

        if (running_.load())
        {
            return;
        }
        if (!sender_)
        {
            sender_ = EthernetNetworkSender::create(interface_);
        }
        running_.store(true);
        senderThread_ = std::thread([this]() { run(); });
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in start: " + std::string(e.what()));
        running_.store(false);
    }
}

void IedServer::stop()
{
    try
    {
        if (!running_.load())
        {
            return;
        }
        running_.store(false);
        if (senderThread_.joinable())
        {
            senderThread_.join();
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in stop: " + std::string(e.what()));
    }
}

void IedServer::updateSampledValue(const std::shared_ptr<SampledValueControlBlock>& svcb, const std::vector<AnalogValue>& values) const
{
    try
    {
        ASSERT(svcb, "SVCB is null");
        ASSERT(!values.empty(), "Values are empty");

        ASDU asdu;
        asdu.svID = svcb->getName();
        static std::atomic<uint16_t> smpCnt{0};
        asdu.smpCnt = smpCnt.fetch_add(1);
        asdu.confRev = 1;
        asdu.smpSynch = SmpSynch::Local;
        asdu.dataSet = values;

        if (asdu.dataSet.size() != VALUES_PER_ASDU)
        {
            LOG_ERROR("Invalid number of values for ASDU: " + std::to_string(asdu.dataSet.size()) + ", expected: " + std::to_string(VALUES_PER_ASDU));
            return;
        }

        if (asdu.smpSynch == SmpSynch::Global)
        {
            LOG_INFO("Global synchronization not implemented, defaulting to Local");
            asdu.smpSynch = SmpSynch::Local;
        }

        asdu.timestamp = std::chrono::system_clock::now();

        if (!asdu.isValid())
        {
            LOG_ERROR("ASDU is not valid");
            return;
        }

        if (sender_)
        {
            sender_->sendASDU(svcb, asdu);
        }
        else
        {
            LOG_ERROR("Sender not initialized");
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in updateSampledValue: " + std::string(e.what()));
    }
}

IedModel::Ptr IedServer::getModel() const
{
    return model_;
}

void IedServer::run() const
{
    while (running_.load())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
