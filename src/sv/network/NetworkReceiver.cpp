#include "sv/network/NetworkReceiver.h"
#include "sv/core/logging.h"
#include "sv/core/buffer.h"

#include <array>
#include <vector>
#include <sstream>
#include <cstring>
#include <iomanip>
#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <ifaddrs.h>

using namespace sv;

std::string sv::getFirstEthernetInterface()
{
    ifaddrs *ifaddr = nullptr;

    if (getifaddrs(&ifaddr) == -1)
    {
        LOG_ERROR("getifaddrs failed" + std::string(strerror(errno)));
        return "";
    }

    /// @brief RAII guard for ifaddrs \struct IfAddrGuard
    struct IfAddrGuard
    {
        ifaddrs* addr;
        explicit IfAddrGuard(ifaddrs* a) : addr(a) {}
        ~IfAddrGuard() { if (addr) freeifaddrs(addr); }
        IfAddrGuard(const IfAddrGuard&) = delete;
        IfAddrGuard& operator=(const IfAddrGuard&) = delete;
    } guard(ifaddr);

    for (const ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == nullptr) continue;
        if (ifa->ifa_addr->sa_family != AF_PACKET) continue;  // eth

        const std::string ifName(ifa->ifa_name);
        if (ifName == "lo") continue;  // Skip loopback

        //check if iss up and not loopback
        const int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) continue;

        struct ifreq ifr{};
        const size_t copyLen = std::min(ifName.size(), static_cast<size_t>(IFNAMSIZ - 1));
        std::copy_n(ifName.begin(), copyLen, ifr.ifr_name);
        ifr.ifr_name[IFNAMSIZ - 1] = '\0';

        bool isUp = false;
        if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0)
        {
            if (ifr.ifr_flags & IFF_UP)
            {
                isUp = true;
            }
        }
        close(sock);

        if (isUp)
        {
            return ifName;
        }
    }

    return "";
}

std::unique_ptr<EthernetNetworkReceiver> EthernetNetworkReceiver::create(const std::string& interface)
{
    return std::unique_ptr<EthernetNetworkReceiver>(new EthernetNetworkReceiver(interface));
}

EthernetNetworkReceiver::EthernetNetworkReceiver(std::string interface)
    : interface_(std::move(interface))
    , socket_(socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL)))
    , ifIndex_(0)
    , running_(false)
{
    if (socket_.get() < 0)
    {
        throw std::runtime_error("Failed to create raw socket: " + std::string(strerror(errno)));
    }

    try
    {
        ifIndex_ = getInterfaceIndex();
        enablePromiscuousMode();

        sockaddr_ll addr{};
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

EthernetNetworkReceiver::~EthernetNetworkReceiver()
{
    stop();
}

int EthernetNetworkReceiver::getInterfaceIndex() const
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

void EthernetNetworkReceiver::enablePromiscuousMode() const
{
    struct ifreq ifr{};

    const size_t copyLen = std::min(interface_.size(), static_cast<size_t>(IFNAMSIZ - 1));
    std::copy_n(interface_.begin(), copyLen, ifr.ifr_name);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    if (ioctl(socket_.get(), SIOCGIFFLAGS, &ifr) >= 0)
    {
        ifr.ifr_flags |= IFF_PROMISC;
        if (ioctl(socket_.get(), SIOCSIFFLAGS, &ifr) < 0)
        {
            LOG_ERROR("Failed to set promiscuous mode on interface: " + interface_ + ": " + std::string(strerror(errno)));
        }
    }
    else
    {
        LOG_ERROR("Failed to get interface flags for: " + interface_ + ": " + std::string(strerror(errno)));
    }
}

std::string EthernetNetworkReceiver::formatMacAddress(const std::array<uint8_t, 6> &mac)
{
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < mac.size(); ++i)
    {
        if (i > 0) oss << ":";
        oss << std::setw(2) << static_cast<int>(mac[i]);
    }
    return oss.str();
}

std::string smpSynchToString(const SmpSynch synch)
{
    switch (synch)
    {
        case SmpSynch::None: return "None";
        case SmpSynch::Local: return "Local";
        case SmpSynch::Global: return "Global";
        default: return "Unknown";
    }
}

std::optional<ASDU> EthernetNetworkReceiver::parseASDU(const std::vector<uint8_t> &buffer, const size_t length)
{
    constexpr size_t MIN_SV_FRAME_SIZE = 14 + 8; // Ethernet header + minimum SV size

    if (length < MIN_SV_FRAME_SIZE)
    {
        LOG_ERROR("Frame too short for SV: " + std::to_string(length) + " bytes");
        return std::nullopt;
    }

    try
    {
        BufferReader reader(buffer);
        reader.skip(12);

        uint16_t etherType = reader.readUint16();
        if (etherType == sv::VLAN_TAG_TPID)
        {
            reader.skip(2);
            etherType = reader.readUint16();
        }

        if (etherType != sv::SV_ETHER_TYPE)
        {
            LOG_ERROR("Not an SV frame, etherType=0x" + [](const uint16_t et)
            {
                std::ostringstream oss;
                oss << std::hex << std::setw(4) << std::setfill('0') << et;
                return oss.str();
            }(etherType));
            return std::nullopt;
        }

        [[maybe_unused]] const uint16_t appId = reader.readUint16();
        [[maybe_unused]] const uint16_t svLength = reader.readUint16();

        reader.skip(4);

        const uint8_t numASDUs = reader.readUint8();
        if (numASDUs == 0 || numASDUs > MAX_ASDUS_PER_MESSAGE)
        {
            LOG_ERROR("Invalid number of ASDUs: " + std::to_string(numASDUs));
            return std::nullopt;
        }

        ASDU asdu;

        // Read svID (64 bytes fixed length, null-padded)
        asdu.svID = reader.readFixedString(64);

        const size_t nullPos = asdu.svID.find('\0');
        if (nullPos != std::string::npos)
        {
            asdu.svID.erase(nullPos);
        }

        while (!asdu.svID.empty() && asdu.svID.back() == ' ')
        {
            asdu.svID.pop_back();
        }

        asdu.smpCnt = reader.readUint16();
        asdu.confRev = reader.readUint32();

        const uint8_t synchValue = reader.readUint8();

        switch (synchValue)
        {
            case 0: asdu.smpSynch = SmpSynch::None; break;
            case 1: asdu.smpSynch = SmpSynch::Local; break;
            case 2: asdu.smpSynch = SmpSynch::Global; break;
            default:
                LOG_ERROR("Invalid SmpSynch value: " + std::to_string(synchValue));
                asdu.smpSynch = SmpSynch::None; break;
        }

        constexpr size_t EXPECTED_VALUES = VALUES_PER_ASDU;
        for (size_t i = 0; i < EXPECTED_VALUES && reader.remaining() >= 8; ++i)
        {
            AnalogValue analogValue;
            const int32_t value = reader.readInt32();
            analogValue.value = value;

            const uint32_t qualityRaw = reader.readUint32();
            analogValue.quality = Quality(qualityRaw);

            asdu.dataSet.push_back(analogValue);
        }

        if (asdu.dataSet.size() != EXPECTED_VALUES)
        {
            LOG_ERROR("Invalid number of values in ASDU: " + std::to_string(asdu.dataSet.size()) + ", expected: " + std::to_string(EXPECTED_VALUES));
            return std::nullopt;
        }

        if (reader.remaining() >= 8)
        {
            const uint64_t ts = reader.readUint64();
            asdu.timestamp = Timestamp(std::chrono::nanoseconds(ts));
        }
        else
        {
            asdu.timestamp = std::chrono::system_clock::now();
            LOG_ERROR("Timestamp missing in ASDU, using current time");
        }

        if (!asdu.isValid())
        {
            LOG_ERROR("Parsed ASDU is not valid: svID='" + asdu.svID + "' (length=" + std::to_string(asdu.svID.length()) + ")");
            return std::nullopt;
        }

        LOG_INFO("Parsed ASDU: svID=" + asdu.svID + ", smpCnt=" + std::to_string(asdu.smpCnt) +
                 ", confRev=" + std::to_string(asdu.confRev) + ", smpSynch=" + smpSynchToString(asdu.smpSynch) +
                 ", values=" + std::to_string(asdu.dataSet.size()));

        return asdu;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception while parsing ASDU: " + std::string(e.what()));
        return std::nullopt;
    }
}

void EthernetNetworkReceiver::start(Callback callback)
{
    try
    {
        ASSERT(callback, "Callback is null");

        if (running_.load())
        {
            LOG_ERROR("Receiver already running");
            return;
        }

        running_.store(true);

        receiveThread_ = std::thread([this, callback = std::move(callback)]()
        {
            constexpr size_t BUFFER_SIZE = 1500;
            std::vector<uint8_t> buffer(BUFFER_SIZE);

            while (running_.load())
            {
                const ssize_t len = recv(socket_.get(), buffer.data(), buffer.size(), 0);

                if (len < 0)
                {
                    if (errno != EAGAIN && errno != EWOULDBLOCK)
                    {
                        LOG_ERROR("Receive error: " + std::string(strerror(errno)));
                    }
                    continue;
                }

                const auto lenSize = static_cast<size_t>(len);

                if (lenSize < 14)
                {
                    LOG_ERROR("Frame too short: " + std::to_string(lenSize) + " bytes");
                    continue;
                }

                // Extract and log destination MAC
                std::array<uint8_t, 6> destMac{};
                std::copy_n(buffer.begin(), 6, destMac.begin());

                const uint16_t etherType = (static_cast<uint16_t>(buffer[12]) << 8) | buffer[13];

                LOG_INFO("RX frame: dstMAC=" + formatMacAddress(destMac) + " etherType=0x" + [](const uint16_t et)
                {
                    std::ostringstream oss;
                    oss << std::hex << std::setw(4) << std::setfill('0') << et;
                    return oss.str();
                }(etherType) + " len=" + std::to_string(lenSize));

                // Parse ASDU
                auto asduOpt = parseASDU(buffer, lenSize);
                if (asduOpt.has_value())
                {
                    callback(asduOpt.value());
                }
            }
        });
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Exception in start: " + std::string(e.what()));
        running_.store(false);
        throw;
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