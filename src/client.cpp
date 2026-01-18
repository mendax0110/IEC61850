#include "sv/model/IedClient.h"
#include "sv/model/IedModel.h"
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

    std::cout << "Starting client, listening for 10 seconds..." << std::endl;
    client->start();
    std::cout << "Receiving sampled value frames..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(10));
    std::cout << "Stopping client..." << std::endl;
    client->stop();

    const auto received = client->receiveSampledValues();
    std::cout << "Received " << received.size() << " ASDU frames total." << std::endl;

    if (received.empty())
    {
        std::cout << "No ASDU frames received." << std::endl;
    }
    else
    {
        for (const auto& asdu : received)
        {
            std::cout << "Received ASDU: " << asdu.svID << " with " << asdu.dataSet.size() << " values" << std::endl;
        }
    }

    return 0;
}