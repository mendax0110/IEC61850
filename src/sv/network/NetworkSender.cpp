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
        ASSERT(asdu.dataSet.size() == VALUES_PER_ASDU, "ASDU must contain exactly 8 values");

        BufferWriter writer(1500);

        // Ethernet Layer 2 Header....

        const auto destMac = parseMacAddress(svcb->getMulticastAddress());
        writer.writeBytes(destMac.data(), destMac.size());

        const auto srcMac = getSourceMacAddress();
        writer.writeBytes(srcMac.data(), srcMac.size());

        const uint16_t vlanId = svcb->getVlanId();
        const uint8_t userPriority = svcb->getUserPriority();

        if (vlanId > 0)
        {
            writer.writeUint16(VLAN_TAG_TPID);
            // TCI (Tag Control Information): 3 bits priority + 1 bit CFI + 12 bits VID
            const uint16_t tci = (static_cast<uint16_t>(userPriority) << 13) | (vlanId & 0x0FFF);
            writer.writeUint16(tci);
        }

        // Ethertype for SV protocol
        writer.writeUint16(SV_ETHER_TYPE);

        // SV Protocol Header....

        // APPID from SVCB
        writer.writeUint16(svcb->getAppId());

        // Length field - will be updated later
        const size_t lengthPos = writer.size();
        writer.writeUint16(0);  // Placeholder

        // Reserved 1 (2 bytes) - includes Simulate bit
        // Bit 15: Simulate flag
        const bool simulate = svcb->getSimulate();
        const uint16_t reserved1 = simulate ? 0x8000 : 0x0000;
        writer.writeUint16(reserved1);

        // Reserved 2 (2 bytes) - Reserved Security
        writer.writeUint16(0x0000);

        // APDU....

        // Number of ASDUs (typically 1)
        const uint8_t numASDUs = 1;
        writer.writeUint8(numASDUs);

        // ASDU....

        // svID (64 bytes fixed length, null-padded)
        writer.writeFixedString(asdu.svID, 64);

        // smpCnt
        writer.writeUint16(asdu.smpCnt);

        // confRev from SVCB
        writer.writeUint32(svcb->getConfRev());

        // smpSynch from SVCB (can be overridden by ASDU if needed)
        const SmpSynch synch = svcb->getSmpSynch();
        writer.writeUint8(static_cast<uint8_t>(synch));

        // gmIdentity (optional, 8 bytes) - from SVCB if configured
        const auto gmIdentity = svcb->getGrandmasterIdentity();
        if (gmIdentity.has_value())
        {
            writer.writeBytes(gmIdentity->data(), gmIdentity->size());
        }

        ASSERT(asdu.dataSet.size() == VALUES_PER_ASDU, "Invalid dataset size");

        for (const auto& analogValue : asdu.dataSet)
        {
            // write value based on configured data type...
            if (std::holds_alternative<int32_t>(analogValue.value))
            {
                const int32_t val = std::get<int32_t>(analogValue.value);
                writer.writeInt32(val);
            }
            else if (std::holds_alternative<uint32_t>(analogValue.value))
            {
                const uint32_t val = std::get<uint32_t>(analogValue.value);
                writer.writeUint32(val);
            }
            else if (std::holds_alternative<float>(analogValue.value))
            {
                const float val = std::get<float>(analogValue.value);
                writer.writeFloat(val);
            }
            else
            {
                throw std::runtime_error("Unsupported value type in ASDU dataset");
            }

            const uint32_t qualityRaw = analogValue.quality.toRaw();
            writer.writeUint32(qualityRaw);
        }

        const auto ts = std::chrono::duration_cast<std::chrono::nanoseconds>(asdu.timestamp.time_since_epoch()).count();
        writer.writeUint64(static_cast<uint64_t>(ts));

        const uint16_t length = static_cast<uint16_t>(writer.size() - lengthPos - 2);
        writer.writeUint16At(lengthPos, length);

        sendFrame(writer.data(), writer.size(), destMac);

        LOG_INFO("Sent SV frame: svID=" + asdu.svID +
                 ", smpCnt=" + std::to_string(asdu.smpCnt) +
                 ", confRev=" + std::to_string(svcb->getConfRev()) +
                 ", synch=" + smpSynchToString(synch) +
                 ", simulate=" + (simulate ? "true" : "false") +
                 ", size=" + std::to_string(writer.size()) + " bytes");
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in sendASDU: " + std::string(e.what()));
        throw;
    }
}