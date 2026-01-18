#include <iostream>
#include <thread>
#include <string>

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
    //svcb->setMulticastAddress("01:0C:CD:01:00:01");
    svcb->setMulticastAddress("ff:ff:ff:ff:ff:ff");
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

    std::vector<sv::AnalogValue> values(8);
    for (int i = 0; i < 8; ++i)
    {
        values[i].value = static_cast<int16_t>(i * 100);
        values[i].quality.validity = true;
        values[i].quality.overflow = false;
    }

    std::cout << "Sending 10 sampled value frames..." << std::endl;
    for (int i = 0; i < 10; ++i)
    {
        server->updateSampledValue(svcb, values);
        std::cout << "  Frame " << (i + 1) << "/10 sent" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    server->stop();
    std::cout << "Server stopped." << std::endl;

    return 0;
}