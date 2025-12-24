#include "sv/network/NetworkSender.h"
#include <arpa/inet.h>
#include <cstring>
#include <endian.h>
#include <errno.h>
#include <iostream>
#include <linux/if_packet.h>
#include <utility>
#include <net/ethernet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include "sv/core/logging.h"

using namespace sv;

std::unique_ptr<EthernetNetworkSender> EthernetNetworkSender::create(const std::string& interface)
{
    return std::make_unique<EthernetNetworkSender>(interface);
}

EthernetNetworkSender::EthernetNetworkSender(std::string  interface)
    : interface_(std::move(interface))
    , socket_(-1)
{
    socket_ = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (socket_ < 0)
    {
        throw std::runtime_error("Failed to create socket");
    }

    struct ifreq ifr{};
    std::strncpy(ifr.ifr_name, interface_.c_str(), IFNAMSIZ);
    if (ioctl(socket_, SIOCGIFINDEX, &ifr) < 0)
    {
        close(socket_);
        throw std::runtime_error("Failed to get interface index");
    }

    struct sockaddr_ll addr;
    addr.sll_family = AF_PACKET;
    addr.sll_protocol = htons(ETH_P_ALL);
    addr.sll_ifindex = ifr.ifr_ifindex;
    addr.sll_hatype = 0;
    addr.sll_pkttype = 0;
    addr.sll_halen = 0;

    if (bind(socket_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0)
    {
        close(socket_);
        throw std::runtime_error("Failed to bind socket");
    }
}

EthernetNetworkSender::~EthernetNetworkSender()
{
    if (socket_ >= 0)
    {
        close(socket_);
    }
}

void EthernetNetworkSender::sendASDU(std::shared_ptr<SampledValueControlBlock> svcb, const ASDU& asdu)
{
    try
    {
        ASSERT(svcb, "SVCB is null");
        ASSERT(!asdu.svID.empty(), "svID is empty");

        uint8_t frame[1500];
        size_t offset = 0;

        // Destination MAC from multicast address
        std::string macStr = svcb->getMulticastAddress();
        uint8_t destMac[6];
        if (sscanf(macStr.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                   &destMac[0], &destMac[1], &destMac[2], &destMac[3], &destMac[4], &destMac[5]) != 6)
        {
            LOG_ERROR("Invalid MAC address format: " + macStr);
            return;
        }
        std::memcpy(frame + offset, destMac, 6);
        offset += 6;

        // Source MAC (placeholder)
        uint8_t srcMac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        std::memcpy(frame + offset, srcMac, 6);
        offset += 6;

        // EtherType
        uint16_t etherType = SV_ETHER_TYPE;
        *reinterpret_cast<uint16_t*>(frame + offset) = htons(etherType);
        offset += 2;

        // AppID
        uint16_t appId = svcb->getAppId();
        *reinterpret_cast<uint16_t*>(frame + offset) = htons(appId);
        offset += 2;

        // Length (placeholder, calculate later)
        uint16_t lengthPos = offset;
        offset += 2;

        // Reserved
        offset += 2;

        // PDU: ASDU
        // svID (string, assume up to 64 chars)
        size_t svIdLen = std::min(asdu.svID.size(), size_t(64));
        std::memcpy(frame + offset, asdu.svID.c_str(), svIdLen);
        offset += 64; // Fixed size in IEC?

        // smpCnt
        *reinterpret_cast<uint16_t*>(frame + offset) = htons(asdu.smpCnt);
        offset += 2;

        // confRev
        *reinterpret_cast<uint32_t*>(frame + offset) = htonl(asdu.confRev);
        offset += 4;

        // smpSynch
        frame[offset++] = asdu.smpSynch ? 1 : 0;

        // DataSet
        for (const auto& val : asdu.dataSet)
        {
            if (std::holds_alternative<int16_t>(val.value))
            {
                int16_t v = std::get<int16_t>(val.value);
                *reinterpret_cast<uint16_t*>(frame + offset) = htons(v);
                offset += 2;
            }
            else if (std::holds_alternative<float>(val.value))
            {
                float v = std::get<float>(val.value);
                uint32_t fv = *reinterpret_cast<uint32_t*>(&v);
                *reinterpret_cast<uint32_t*>(frame + offset) = htonl(fv);
                offset += 4;
            }
            // Quality
            frame[offset++] = (val.quality.validity ? 0x80 : 0) | (val.quality.overflow ? 0x40 : 0);
        }

        // Timestam
        auto ts = std::chrono::duration_cast<std::chrono::nanoseconds>(asdu.timestamp.time_since_epoch()).count();
        *reinterpret_cast<uint64_t*>(frame + offset) = htobe64(ts);
        offset += 8;

        // Set length
        uint16_t length = offset - 14; // Eth header
        *reinterpret_cast<uint16_t*>(frame + lengthPos) = htons(length);

        // Send
        struct sockaddr_ll addr = {};
        addr.sll_ifindex = if_nametoindex(interface_.c_str());
        if (addr.sll_ifindex == 0)
        {
            LOG_ERROR("Invalid interface: " + interface_);
            return;
        }
        addr.sll_halen = ETH_ALEN;
        std::memcpy(addr.sll_addr, destMac, ETH_ALEN);

        ssize_t sent = sendto(socket_, frame, offset, 0, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
        if (sent < 0)
        {
            LOG_ERROR("Send failed: " + std::string(strerror(errno)));
        }
        else
        {
            LOG_INFO("Sent ASDU frame of " + std::to_string(sent) + " bytes");
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in sendASDU: " + std::string(e.what()));
    }
}