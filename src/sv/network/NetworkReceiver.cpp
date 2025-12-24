#include "sv/network/NetworkReceiver.h"
#include <atomic>
#include <arpa/inet.h>
#include <cstring>
#include <endian.h>
#include <errno.h>
#include <iostream>
#include <utility>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ifaddrs.h>
#include "sv/core/logging.h"

using namespace sv;

std::string sv::getFirstEthernetInterface()
{
    struct ifaddrs *ifaddr, *ifa;
    std::string result;

    if (getifaddrs(&ifaddr) == -1)
    {
        LOG_ERROR("getifaddrs failed");
        return "";
    }

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == nullptr) continue;
        if (ifa->ifa_addr->sa_family != AF_PACKET) continue;  // eth
        if (strcmp(ifa->ifa_name, "lo") == 0) continue;  // Skip loopback

        //check if iss up and not loopback
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) continue;

        struct ifreq ifr{};
        std::strncpy(ifr.ifr_name, ifa->ifa_name, IFNAMSIZ);
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0)
        {
            if (ifr.ifr_flags & IFF_UP)
            {
                result = ifa->ifa_name;
                close(sock);
                break;
            }
        }
        close(sock);
    }

    freeifaddrs(ifaddr);
    return result;
}

std::unique_ptr<EthernetNetworkReceiver> EthernetNetworkReceiver::create(const std::string& interface)
{
    return std::make_unique<EthernetNetworkReceiver>(interface);
}

EthernetNetworkReceiver::EthernetNetworkReceiver(std::string  interface)
    : interface_(std::move(interface))
    , socket_(-1)
    , running_(false)
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

EthernetNetworkReceiver::~EthernetNetworkReceiver()
{
    stop();
    if (socket_ >= 0)
    {
        close(socket_);
    }
}

void EthernetNetworkReceiver::start(Callback callback)
{
    try
    {
        ASSERT(callback, "Callback is null");

        if (running_.load())
        {
            return;
        }
        running_.store(true);
        receiveThread_ = std::thread([this, callback]()
        {
            uint8_t buffer[1500];
            while (running_.load())
            {
                ssize_t len = recv(socket_, buffer, sizeof(buffer), 0);
                if (len < 0)
                {
                    if (errno != EAGAIN)
                    {
                        LOG_ERROR("Receive error: " + std::string(strerror(errno)));
                    }
                    continue;
                }
                size_t len_size = static_cast<size_t>(len);
                if (len_size < 14 + 8) // Minimum size
                {
                    continue;
                }
                if (len_size < 14)
                {
                    LOG_ERROR("Frame too short");
                    continue;
                }
                uint16_t etherType = ntohs(*reinterpret_cast<uint16_t*>(buffer + 12));
                if (etherType != SV_ETHER_TYPE)
                {
                    continue;
                }
                // Parse ASDU
                size_t offset = 14; // After Ethernet header
                // AppID
                offset += 2;
                // Length
                offset += 2;
                // Reserved
                offset += 2;

                ASDU asdu;
                // svID
                char svId[65] = {0};
                std::memcpy(svId, buffer + offset, 64);
                asdu.svID = svId;
                offset += 64;

                // smpCnt
                asdu.smpCnt = ntohs(*reinterpret_cast<uint16_t*>(buffer + offset));
                offset += 2;

                // confRev
                asdu.confRev = ntohl(*reinterpret_cast<uint32_t*>(buffer + offset));
                offset += 4;

                // smpSynch
                asdu.smpSynch = buffer[offset++] != 0;

                // DataSet (assuem 8 values)
                for (int i = 0; i < 8 && offset + 3 < len_size; ++i)
                {
                    AnalogValue val;
                    // Value (assume int16)
                    val.value = static_cast<int16_t>(ntohs(*reinterpret_cast<uint16_t*>(buffer + offset)));
                    offset += 2;
                    // Quality
                    uint8_t quality = buffer[offset++];
                    val.quality.validity = quality & 0x80;
                    val.quality.overflow = quality & 0x40;
                    asdu.dataSet.push_back(val);
                }

                // Timestamp
                if (offset + 8 <= len_size)
                {
                    uint64_t ts = be64toh(*reinterpret_cast<uint64_t*>(buffer + offset));
                    asdu.timestamp = Timestamp(std::chrono::nanoseconds(ts));
                }
                else
                {
                    asdu.timestamp = std::chrono::system_clock::now();
                }

                callback(asdu);
            }
        });
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in start: " + std::string(e.what()));
    }
}

void EthernetNetworkReceiver::stop()
{
    if (!running_)
    {
        return;
    }
    running_ = false;
    if (receiveThread_.joinable())
    {
        receiveThread_.join();
    }
}