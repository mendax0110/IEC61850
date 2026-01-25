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
    svcb->setConfRev(1);
    svcb->setDataSet("PhsMeas1");
    svcb->setSmpSynch(sv::SmpSynch::Local);
    svcb->setSamplesPerPeriod(sv::SamplesPerPeriod::SPP_80);
    svcb->setSignalFrequency(sv::SignalFrequency::FREQ_50_HZ);
    svcb->setVlanId(0);
    svcb->setUserPriority(4);
    svcb->setSimulate(false);
    svcb->setDataType(sv::DataType::INT32);
    svcb->setCurrentScaling(sv::ScalingFactors::CURRENT_DEFAULT);
    svcb->setVoltageScaling(sv::ScalingFactors::VOLTAGE_DEFAULT);

    std::cout << "\n=== SVCB Configuration ===" << std::endl;
    std::cout << "  svID: " << svcb->getName() << std::endl;
    std::cout << "  AppID: " << std::hex << svcb->getAppId() << std::dec << std::endl;
    std::cout << "  Sample Rate: " << svcb->getSmpRate() << " Hz" << std::endl;
    std::cout << "  Sample/Period: " << static_cast<int>(svcb->getSamplesPerPeriod()) << std::endl;
    std::cout << "  Signal Frequency: " << static_cast<int>(svcb->getSignalFrequency()) / 10.0 << " Hz" << std::endl;
    std::cout << "  Synchronization: ";
    switch (svcb->getSmpSynch())
    {
        case sv::SmpSynch::None: std::cout << "None"; break;
        case sv::SmpSynch::Local: std::cout << "Local"; break;
        case sv::SmpSynch::Global: std::cout << "Global"; break;
        default: std::cout << "Unknown"; break;
    }
    std::cout << std::endl;
    std::cout << "  User Priority: " << static_cast<int>(svcb->getUserPriority()) << std::endl;
    std::cout << "  VLAN ID: " << svcb->getVlanId() << (svcb->getVlanId() == 0 ? " (no VLAN)" : "") << std::endl;
    std::cout << "  Simulate: " << (svcb->getSimulate() ? "Yes" : "No") << std::endl;
    std::cout << "  Current Scaling: " << svcb->getCurrentScaling() << std::endl;
    std::cout << "  Voltage Scaling: " << svcb->getVoltageScaling() << std::endl;
    std::cout << "=========================" << std::endl;

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
    for (int frame = 0; frame < 20; ++frame)
    {
        const auto samplesPerPeriod = static_cast<double>(svcb->getSamplesPerPeriod());
        const double frequency = static_cast<double>(svcb->getSignalFrequency()) / 10.0;
        const double time = frame / (samplesPerPeriod * frequency);
        const double omega = 2.0 * M_PI * frequency;

        constexpr double currentAmplitude = 100.0;
        values[0].value = static_cast<int32_t>(currentAmplitude * std::sin(omega * time) * sv::ScalingFactors::CURRENT_DEFAULT);
        values[1].value = static_cast<int32_t>(currentAmplitude * std::sin(omega * time - (2.0 * M_PI / 3.0)) * sv::ScalingFactors::CURRENT_DEFAULT);
        values[2].value = static_cast<int32_t>(currentAmplitude * std::sin(omega * time + (2.0 * M_PI / 3.0)) * sv::ScalingFactors::CURRENT_DEFAULT);
        values[3].value = 0;

        constexpr double voltageAmplitude = 230.0;
        values[4].value = static_cast<int32_t>(voltageAmplitude * std::sin(omega * time) * sv::ScalingFactors::VOLTAGE_DEFAULT);
        values[5].value = static_cast<int32_t>(voltageAmplitude * std::sin(omega * time - (2.0 * M_PI / 3.0)) * sv::ScalingFactors::VOLTAGE_DEFAULT);
        values[6].value = static_cast<int32_t>(voltageAmplitude * std::sin(omega * time + (2.0 * M_PI / 3.0)) * sv::ScalingFactors::VOLTAGE_DEFAULT);
        values[7].value = 0;

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

        if (frame % 5 == 0)
        {
            const int32_t currentVal = values[0].getScaledInt();
            const int32_t voltageVal = values[4].getScaledInt();

            std::cout << "Frame " << std::setw(2) << (frame + 1) << ": "
                      << "Ia=" << std::setw(6) << currentVal / svcb->getCurrentScaling() << "A, "
                      << "Va=" << std::setw(6) << voltageVal / svcb->getVoltageScaling() << "V"
                      << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    server->stop();
    std::cout << "Server stopped." << std::endl;

    return 0;
}