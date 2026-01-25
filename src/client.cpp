#include "sv/model/IedClient.h"
#include "sv/model/IedModel.h"
#include "sv/visualize/SVVisualizer.h"
#include <iostream>
#include <string>
#include <thread>

int main(int argc, char* argv[])
{
    std::string interface;
    if (argc > 1)
    {
        interface = argv[1];
    }

    std::cout << "IEC61850 SV Client Demo" << std::endl;
    std::cout << "Interface: " << (interface.empty() ? "(auto-detect)" : interface) << std::endl;

    const auto model = sv::IedModel::create("ClientModel");
    const auto client = sv::IedClient::create(model, interface);
    if (!client)
    {
        std::cerr << "Error: Failed to create client. Check interface name." << std::endl;
        return 1;
    }

    //auto viz = sv::SVVisualizer::create(sv::SVVisualizer::Mode::RealTime, "sv_data.csv");

    std::cout << "Starting client, listening for 10 seconds..." << std::endl;
    client->start();
    /*client->start([&viz](const sv::ASDU& asdu)
    {
        viz->update(asdu);
    });*/
    std::cout << "Receiving sampled value frames..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(10));
    std::cout << "Stopping client..." << std::endl;
    client->stop();
    //viz->close();

    const auto received = client->receiveSampledValues();
    std::cout << "Received " << received.size() << " ASDU frames total." << std::endl;

    if (received.empty())
    {
        std::cout << "No ASDU frames received." << std::endl;
    }
    else
    {
        std::cout << "\n=== Received ASDU Details ===" << std::endl;
        std::cout << "Total frames: " << received.size() << std::endl;
        std::cout << "First smpCnt: " << received.front().smpCnt << std::endl;
        std::cout << "Last smpCnt: " << received.back().smpCnt << std::endl;

        int missing = 0;
        for (size_t i = 1; i < received.size(); ++i)
        {
            uint16_t expected = (received[i-1].smpCnt + 1) & 0xFFFF;
            if (received[i].smpCnt != expected)
            {
                missing++;
            }
        }
        std::cout << "Missing frames (based on smpCnt): " << missing << std::endl;
        std::cout << "==============================" << std::endl;

        const size_t framesToShow = std::min(received.size(), static_cast<size_t>(3));
        for (size_t i = 0; i <framesToShow; ++i)
        {
            const auto& asdu = received[i];
            std::cout << "\n=== ASDU Frame " << (i + 1) << " ===" << std::endl;
            std::cout << "SV ID: " << asdu.svID << std::endl;
            std::cout << "Sample Count: " << asdu.smpCnt << std::endl;
            std::cout << "Configuration Revision: " << asdu.confRev << std::endl;

            std::string synchStr;
            switch (asdu.smpSynch)
            {
                case sv::SmpSynch::None: synchStr = "None"; break;
                case sv::SmpSynch::Local: synchStr = "Local"; break;
                case sv::SmpSynch::Global: synchStr = "Global"; break;
                default: synchStr = "Unknown"; break;
            }
            std::cout << "Synchronization: " << synchStr << std::endl;

            std::cout << "Dataset (" << asdu.dataSet.size() << " values):" << std::endl;

            if (asdu.dataSet.size() >= 4)
            {
                std::cout << "  Currents (scaled by " << sv::ScalingFactors::CURRENT_DEFAULT << "):" << std::endl;
                for (size_t j = 0; j < 4; ++j)
                {
                    const auto& av = asdu.dataSet[j];
                    const int32_t val = av.getScaledInt();
                    const float actual = static_cast<float>(val) / sv::ScalingFactors::CURRENT_DEFAULT;

                    std::cout << "      I" << j << std::setw(10) << val
                                << " (actual: " << std::fixed << std::setprecision(3) << actual << " A)"
                                << " [Quality: " << (av.quality.isGood() ? "Good" : "Bad") << "]" << std::endl;

                    if (!av.quality.isGood())
                    {
                        std::cout << "        Quality Flags: ";
                        const uint32_t qRaw = av.quality.toRaw();
                        for (int bit = 0; bit < 32; ++bit)
                        {
                            if (qRaw & (1 << bit))
                            {
                                std::cout << "Bit" << bit << " ";
                            }
                        }
                    }
                }
            }

            if (asdu.dataSet.size() >= 8)
            {
                std::cout << "  Voltages (scaled by " << sv::ScalingFactors::VOLTAGE_DEFAULT << "):" << std::endl;
                for (size_t j = 4; j < 8; ++j)
                {
                    const auto& av = asdu.dataSet[j];
                    const int32_t val = av.getScaledInt();
                    const float actual = static_cast<float>(val) / sv::ScalingFactors::VOLTAGE_DEFAULT;

                    std::cout << "      U" << (j - 4) << std::setw(10) << val
                                << " (actual: " << std::fixed << std::setprecision(3) << actual << " V)"
                                << " [Quality: " << (av.quality.isGood() ? "Good" : "Bad") << "]" << std::endl;

                    if (!av.quality.isGood())
                    {
                        std::cout << "        Quality Flags: ";
                        const uint32_t qRaw = av.quality.toRaw();
                        for (int bit = 0; bit < 32; ++bit)
                        {
                            if (qRaw & (1 << bit))
                            {
                                std::cout << "Bit" << bit << " ";
                            }
                        }
                    }
                }
            }

            if (asdu.gmIdentity.has_value())
            {
                std::cout << "Grandmaster Identity: ";
                for (const auto byte : asdu.gmIdentity.value())
                {
                    std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
                }
                std::cout << std::dec << std::endl;
            }

            const auto duration = asdu.timestamp.time_since_epoch();
            const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
            std::cout << "Timestamp (ns since epoch): " << ns << std::endl;
            std::cout << "==========================" << std::endl;
        }

        if (received.size() > framesToShow)
        {
            std::cout << "\n... (only first " << framesToShow << " frames shown)" << std::endl;
        }
    }

    return 0;
}