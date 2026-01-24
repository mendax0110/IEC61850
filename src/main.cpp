#include <iostream>
#include <thread>
#include <string>
#include <math.h>

#include "sv/core/mac.h"
#include "sv/model/IedModel.h"
#include "sv/model/IedServer.h"
#include "sv/model/LogicalNode.h"
#include "sv/model/SampledValueControlBlock.h"

int main(int argc, char* argv[])
{
    std::string interface;
    if (argc > 1)
    {
        interface = argv[1];
    }

    std::cout << "IEC61850 SV Server Demo" << std::endl;
    std::cout << "Interface: " << (interface.empty() ? "(auto-detect)" : interface) << std::endl;

    const auto model = sv::IedModel::create("SVModel");

    const auto ln = sv::LogicalNode::create("MU01");
    model->addLogicalNode(ln);

    const auto svcb = sv::SampledValueControlBlock::create("SV01");
    const auto mac = sv::MacAddress::tryParse("01:0C:CD:04:00:01");
    if (mac.has_value())
    {
        svcb->setMulticastAddress(mac.value().toString());
        std::cout << "Multicast MAC: " << mac.value() << std::endl;
    }
    else
    {
        std::cerr << "Error: Invalid MAC address format" << std::endl;
        return 1;
    }
    svcb->setAppId(0x4000);
    svcb->setSmpRate(4000);
    ln->addSampledValueControlBlock(svcb);

    const auto server = sv::IedServer::create(model, interface);
    if (!server)
    {
        std::cerr << "Error: Failed to create server. Check interface name." << std::endl;
        return 1;
    }

    std::cout << "Starting server..." << std::endl;
    server->start();

    std::vector<sv::AnalogValue> values(sv::VALUES_PER_ASDU);
    std::cout << "Sending 10 sampled value frames..." << std::endl;
    for (int frame = 0; frame < 10; ++frame)
    {
        const double time = frame * 0.001;
        constexpr double freq = 50.0;
        constexpr double omega = 2.0 * M_PI * freq;

        constexpr double currentAmplitude = 100.0;
        values[0].value = static_cast<int32_t>(currentAmplitude * std::sin(omega * time) * sv::ScalingFactors::CURRENT_DEFAULT);
        values[1].value = static_cast<int32_t>(currentAmplitude * std::sin(omega * time - (2.0 * M_PI / 3.0)) * sv::ScalingFactors::CURRENT_DEFAULT);
        values[2].value = static_cast<int32_t>(currentAmplitude * std::sin(omega * time + (2.0 * M_PI / 3.0)) * sv::ScalingFactors::CURRENT_DEFAULT);
        values[3].value = static_cast<int32_t>(0);

        constexpr double voltageAmplitude = 230.0;
        values[4].value = static_cast<int32_t>(voltageAmplitude * std::sin(omega * time) * sv::ScalingFactors::VOLTAGE_DEFAULT);
        values[5].value = static_cast<int32_t>(voltageAmplitude * std::sin(omega * time - (2.0 * M_PI / 3.0)) * sv::ScalingFactors::VOLTAGE_DEFAULT);
        values[6].value = static_cast<int32_t>(voltageAmplitude * std::sin(omega * time + (2.0 * M_PI / 3.0)) * sv::ScalingFactors::VOLTAGE_DEFAULT);
        values[7].value = static_cast<int32_t>(0);

        for (auto& val : values)
        {
            val.quality = sv::Quality(0);
            val.quality.validity = 0;
            val.quality.overflow = false;
            val.quality.outOfRange = false;
            val.quality.badReference = false;
            val.quality.oscillatory = false;
            val.quality.failure = false;
            val.quality.oldData = false;
            val.quality.inconsistent = false;
            val.quality.inaccurate = false;
            val.quality.source = false;
            val.quality.test = false;
            val.quality.operatorBlocked = false;
            val.quality.derived = false;
        }

        server->updateSampledValue(svcb, values);
        std::cout << "Sent frame " << (frame + 1) << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    server->stop();
    std::cout << "Server stopped." << std::endl;

    return 0;
}