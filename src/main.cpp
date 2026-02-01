#include <iostream>
#include <thread>
#include <string>
#include <iomanip>
#include <cmath>
#include <complex>

#include "sv/core/mac.h"
#include "sv/core/ptp.h"
#include "sv/model/IedModel.h"
#include "sv/model/IedServer.h"
#include "sv/model/LogicalNode.h"
#include "sv/model/SampledValueControlBlock.h"
#include "sv/sim/Breaker.h"
#include "sv/protection/Protection.h"


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
    svcb->setSmpSynch(sv::SmpSynch::Global);
    svcb->setSamplesPerPeriod(sv::SamplesPerPeriod::SPP_80);
    svcb->setSignalFrequency(sv::SignalFrequency::FREQ_50_HZ);
    svcb->setVlanId(0);
    svcb->setUserPriority(4);
    svcb->setSimulate(false);
    svcb->setDataType(sv::DataType::INT32);
    svcb->setCurrentScaling(sv::ScalingFactors::CURRENT_DEFAULT);
    svcb->setVoltageScaling(sv::ScalingFactors::VOLTAGE_DEFAULT);

    /*std::array<uint8_t, 8> gmIdentity = {0x00, 0x1B, 0x21, 0xFF, 0xFE, 0x12, 0x34, 0x56};
    svcb->setGrandmasterIdentity(gmIdentity);*/

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

    sv::sim::BreakerDefinition breakerDef;
    breakerDef.maxCurrentA = 500.0;
    breakerDef.voltageRatingV = 400.0;
    breakerDef.openTimeSec = 0.050;
    breakerDef.closeTimeSec = 0.100;
    auto breaker = sv::sim::BreakerModel::create(breakerDef);

    sv::DistanceProtectionSettings distSettings;
    distSettings.zone1.reachOhm = 0.8;
    distSettings.zone1.delay = std::chrono::milliseconds(0);
    distSettings.zone2.reachOhm = 1.5;
    distSettings.zone2.delay = std::chrono::milliseconds(300);
    distSettings.zone3.enabled = false;
    distSettings.voltageThresholdV = 50.0;
    distSettings.currentThresholdA = 50.0;
    auto distanceProtection = sv::DistanceProtection::create(distSettings);

    distanceProtection->onTrip([&breaker](const sv::DistanceProtectionResult& result)
    {
        std::cout << "\n*** DISTANCE PROTECTION TRIP ***" << std::endl;
        std::cout << "  Zone 1: " << (result.zone1Trip ? "TRIP" : "OK") << std::endl;
        std::cout << "  Zone 2: " << (result.zone2Trip ? "TRIP" : "OK") << std::endl;
        std::cout << "  Zone 3: " << (result.zone3Trip ? "TRIP" : "OK") << std::endl;
        std::cout << "  Measured Z: " << std::fixed << std::setprecision(3)
                  << result.measuredImpedanceOhm << " Ω" << std::endl;
        breaker->trip();
    });

    breaker->onStateChange([](sv::sim::BreakerState oldState, sv::sim::BreakerState newState)
    {
        std::cout << "*** Breaker: " << sv::sim::toString(oldState)
                  << " -> " << sv::sim::toString(newState) << std::endl;
    });

    std::cout << "\n=== Closing breaker ===" << std::endl;
    breaker->close();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    std::cout << "Starting server..." << std::endl;
    server->start();

    std::vector<sv::AnalogValue> values(sv::VALUES_PER_ASDU);
    std::cout << "\nSending sampled value frames with protection simulation..." << std::endl;
    std::cout << "Simulating normal load, then fault at frame 40..." << std::endl;

    for (int frame = 0; frame < 80; ++frame)
    {
        const auto samplesPerPeriod = static_cast<double>(svcb->getSamplesPerPeriod());
        const double frequency = static_cast<double>(svcb->getSignalFrequency()) / 10.0;
        const double time = frame / (samplesPerPeriod * frequency);
        const double omega = 2.0 * std::numbers::pi * frequency;

        double currentAmplitude = 100.0;
        constexpr double voltageAmplitude = 230.0;

        if (frame >= 40 && breaker->isClosed())
        {
            currentAmplitude = 450.0;
            std::cout << "\n>>> Fault injected at frame " << frame << " <<<" << std::endl;
        }

        const double ia = currentAmplitude * std::sin(omega * time);
        const double ib = currentAmplitude * std::sin(omega * time - (2.0 * std::numbers::pi / 3.0));
        const double ic = currentAmplitude * std::sin(omega * time + (2.0 * std::numbers::pi / 3.0));

        const double va = voltageAmplitude * std::sin(omega * time);
        const double vb = voltageAmplitude * std::sin(omega * time - (2.0 * std::numbers::pi / 3.0));
        const double vc = voltageAmplitude * std::sin(omega * time + (2.0 * std::numbers::pi / 3.0));

        if (breaker->isClosed())
        {
            breaker->setCurrent(ia);

            const double actualBreakerCurrent = breaker->getCurrent();
            constexpr double minCurrentForProtection = 10.0;
            if (std::abs(actualBreakerCurrent) > minCurrentForProtection)
            {
                const std::complex<double> currentPhasor(actualBreakerCurrent, 0.0);
                const std::complex<double> voltagePhasor(va, 0.0);

                const auto protResult = distanceProtection->update(voltagePhasor, currentPhasor);

                if (protResult.zone1Trip || protResult.zone2Trip || protResult.zone3Trip)
                {
                    std::cout << "  Protection operated!" << std::endl;
                }
            }
        }

        const double breakerResistance = breaker->getResistance();
        const double actualCurrent = breaker->isClosed() ? ia : 0.0;

        values[0].value = static_cast<int32_t>(actualCurrent * sv::ScalingFactors::CURRENT_DEFAULT);
        values[1].value = static_cast<int32_t>((breaker->isClosed() ? ib : 0.0) * sv::ScalingFactors::CURRENT_DEFAULT);
        values[2].value = static_cast<int32_t>((breaker->isClosed() ? ic : 0.0) * sv::ScalingFactors::CURRENT_DEFAULT);
        values[3].value = 0;

        values[4].value = static_cast<int32_t>(va * sv::ScalingFactors::VOLTAGE_DEFAULT);
        values[5].value = static_cast<int32_t>(vb * sv::ScalingFactors::VOLTAGE_DEFAULT);
        values[6].value = static_cast<int32_t>(vc * sv::ScalingFactors::VOLTAGE_DEFAULT);
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

        if (frame % 10 == 0 || frame >= 40)
        {
            const int32_t currentVal = values[0].getScaledInt();
            const int32_t voltageVal = values[4].getScaledInt();
            const double actualI = static_cast<double>(currentVal) / svcb->getCurrentScaling();
            const double actualV = static_cast<double>(voltageVal) / svcb->getVoltageScaling();

            std::cout << "Frame " << std::setw(3) << (frame + 1) << ": "
                      << "Ia=" << std::setw(7) << std::fixed << std::setprecision(1) << actualI << "A, "
                      << "Va=" << std::setw(7) << std::fixed << std::setprecision(1) << actualV << "V, "
                      << "Breaker: " << sv::sim::toString(breaker->getState());

            if (breaker->isClosed() && std::abs(actualI) > 1.0)
            {
                const double impedance = actualV / actualI;
                std::cout << ", Z=" << std::setw(6) << std::fixed << std::setprecision(2) << impedance << "Ohm";
            }

            std::cout << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    server->stop();
    std::cout << "Server stopped." << std::endl;

    return 0;
}