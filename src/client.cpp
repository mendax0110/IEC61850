#include "sv/model/IedClient.h"
#include "sv/model/IedModel.h"
#include <iostream>
#include <thread>

int main()
{
    auto model = sv::IedModel::create("ClientModel");
    auto client = sv::IedClient::create(model, "veth1");
    client->start();
    std::this_thread::sleep_for(std::chrono::seconds(10));
    client->stop();
    return 0;
}