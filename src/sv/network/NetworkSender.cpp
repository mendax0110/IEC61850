#include "sv/network/NetworkSender.h"
#include "sv/core/logging.h"
#include "sv/core/buffer.h"

#include <cstring>
#include <array>
#include <sstream>
#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

using namespace sv;

std::unique_ptr<EthernetNetworkSender> EthernetNetworkSender::create(const std::string& interface)
{
    return std::unique_ptr<EthernetNetworkSender>(new EthernetNetworkSender(interface));
}


EthernetNetworkSender::EthernetNetworkSender(std::string interface)
    : interface_(std::move(interface))
    , socket_(socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL)))
    , ifIndex_(0)
{
    if (socket_.get() < 0)
    {
        throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
    }

    try
    {
        ifIndex_ = getInterfaceIndex();

        struct sockaddr_ll addr{};
        addr.sll_family = AF_PACKET;
        addr.sll_protocol = htons(ETH_P_ALL);
        addr.sll_ifindex = ifIndex_;
        addr.sll_hatype = 0;
        addr.sll_pkttype = 0;
        addr.sll_halen = 0;

        if (bind(socket_.get(), reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0)
        {
            throw std::runtime_error("Failed to bind socket to interface " + interface_ + ": " + std::string(strerror(errno)));
        }
    }
    catch (...)
    {
        throw;
    }
}

std::array<uint8_t, 6> EthernetNetworkSender::parseMacAddress(const std::string& macStr)
{
    std::array<uint8_t, 6> mac{};
    std::istringstream iss(macStr);
    std::string segment;
    size_t index = 0;

    while (std::getline(iss, segment, ':') && index < 6)
    {
        try
        {
            const unsigned long value = std::stoul(segment, nullptr, 16);
            if (value > 0xFF)
            {
                throw std::invalid_argument("Invalid MAC address segment: " + segment);
            }
            mac[index++] = static_cast<uint8_t>(value);
        }
        catch (const std::invalid_argument&)
        {
            throw std::invalid_argument("Invalid MAC address format: " + macStr);
        }
        catch (const std::out_of_range&)
        {
            throw std::invalid_argument("MAC address segment out of range: " + segment);
        }
    }

    if (index != 6)
    {
        throw std::invalid_argument("Incomplete MAC address: " + macStr);
    }

    return mac;
}

std::array<uint8_t, 6> EthernetNetworkSender::getSourceMacAddress() const
{
    struct ifreq ifr{};

    const size_t copyLen = std::min(interface_.size(), static_cast<size_t>(IFNAMSIZ - 1));
    std::copy_n(interface_.begin(), copyLen, ifr.ifr_name);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    if (ioctl(socket_.get(), SIOCGIFHWADDR, &ifr) < 0)
    {
        LOG_ERROR("Failed to get source MAC address for interface: " + interface_ + ": " + std::string(strerror(errno)));
        return std::array<uint8_t, 6>{};
    }

    std::array<uint8_t, 6> mac{};
    std::copy_n(reinterpret_cast<const uint8_t*>(ifr.ifr_hwaddr.sa_data), 6, mac.data());
    return mac;
}

int EthernetNetworkSender::getInterfaceIndex() const
{
    struct ifreq ifr{};

    const size_t copyLen = std::min(interface_.size(), static_cast<size_t>(IFNAMSIZ - 1));
    std::copy_n(interface_.begin(), copyLen, ifr.ifr_name);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    if (ioctl(socket_.get(), SIOCGIFINDEX, &ifr) < 0)
    {
        throw std::runtime_error("Failed to get interface index for " + interface_ + ": " + std::string(strerror(errno)));
    }

    return ifr.ifr_ifindex;
}

void EthernetNetworkSender::sendFrame(const uint8_t* data, const size_t size, const std::array<uint8_t, 6>& destMac) const
{
    struct sockaddr_ll addr{};
    addr.sll_family = AF_PACKET;
    addr.sll_ifindex = ifIndex_;
    addr.sll_halen = ETH_ALEN;
    std::copy(destMac.begin(), destMac.end(), addr.sll_addr);

    const ssize_t sent = sendto(socket_.get(), data, size, 0, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));

    if (sent < 0)
    {
        const std::string errorMsg = "Send failed: " + std::string(strerror(errno));
        LOG_ERROR(errorMsg);
        throw std::runtime_error(errorMsg);
    }

    if (static_cast<size_t>(sent) != size)
    {
        LOG_ERROR("Partial send: sent " + std::to_string(sent) + " of " + std::to_string(size) + " bytes");
    }
    else
    {
        LOG_INFO("Sent frame of " + std::to_string(sent) + " bytes");
    }
}

void EthernetNetworkSender::sendASDU(const std::shared_ptr<SampledValueControlBlock> svcb, const ASDU& asdu)
{
    try
    {
        ASSERT(svcb, "SVCB is null");
        ASSERT(!asdu.svID.empty(), "ASDU svID is empty");

        BufferWriter writer(1500);

        // dest mac address from multicast address
        const auto destMac = parseMacAddress(svcb->getMulticastAddress());
        writer.writeBytes(destMac.data(), destMac.size());

        // source mac
        const auto srcMac = getSourceMacAddress();
        writer.writeBytes(srcMac.data(), srcMac.size());

        // ehternet type
        writer.writeUint16(SV_ETHER_TYPE);

        // appID
        writer.writeUint16(svcb->getAppId());

        const size_t lengthPos = writer.size();
        writer.writeUint16(0);

        // reversed (2 bytes)
        writer.writeUint16(0);

        // PDU: ASDU, svID 64 bytes
        writer.writeFixedString(asdu.svID, 64);

        // smpCnt
        writer.writeUint16(asdu.smpCnt);

        // confRev
        writer.writeUint32(asdu.confRev);

        // smpSynch
        writer.writeUint8(asdu.smpSynch ? 1 : 0);

        // dataSet values
        for (const auto& [value, quality] : asdu.dataSet)
        {
            if (std::holds_alternative<int16_t>(value))
            {
                writer.writeInt16(std::get<int16_t>(value));
            }
            else if (std::holds_alternative<float>(value))
            {
                writer.writeFloat(std::get<float>(value));
            }

            const uint8_t qualityByte = (quality.validity ? 0x80 : 0) | (quality.overflow ? 0x40 : 0);
            writer.writeUint8(qualityByte);
        }

        const auto ts = std::chrono::duration_cast<std::chrono::nanoseconds>(asdu.timestamp.time_since_epoch()).count();
        writer.writeUint64(static_cast<uint64_t>(ts));

        const uint16_t length = static_cast<uint16_t>(writer.size() - 14);
        writer.writeUint16At(lengthPos, length);

        sendFrame(writer.data(), writer.size(), destMac);
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in sendASDU: " + std::string(e.what()));
        throw;
    }
}