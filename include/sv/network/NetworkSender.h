#pragma once

#include <memory>
#include <string>
#include <unistd.h>
#include "sv/core/types.h"
#include "sv/model/SampledValueControlBlock.h"

/// @brief sv namespace \namespace sv
namespace sv
{
    /// @brief RAII wrapper for socket file descriptor \class SocketGuard
    class SocketGuard
    {
    public:
        /**
         * @brief Constructor that takes a file descriptor.
         * @param fd The file descriptor to manage.
         */
        explicit SocketGuard(const int fd) : fd_(fd) {}

        /**
         * @brief Destructor that closes the file descriptor if valid.
         */
        ~SocketGuard()
        {
            if (fd_ >= 0)
            {
                close(fd_);
            }
        }

        /// @brief Disable: copy constructor and assignment
        SocketGuard(const SocketGuard&) = delete;
        SocketGuard& operator=(const SocketGuard&) = delete;

        /// @brief Enable: move constructor and assignment
        SocketGuard(SocketGuard&& other) noexcept : fd_(other.fd_) { other.fd_ = -1;}
        SocketGuard& operator=(SocketGuard&& other) noexcept
        {
            if (this != &other)
            {
                if (fd_ >= 0) { close(fd_); }
                fd_ = other.fd_;
                other.fd_ = -1;
            }
            return *this;
        }

        /**
         * @brief Gets the managed file descriptor.
         * @return The file descriptor.
         */
        [[nodiscard]] int get() const { return fd_; }

        /**
         * @brief Releases ownership of the file descriptor.
         * @return The file descriptor.
         */
        int release() { const int temp = fd_; fd_ = -1; return temp; }
    private:
        int fd_;
    };

    /// @brief Interface for network sending of SV frames. \class NetworkSender
    class NetworkSender
    {
    public:
        /**
         * @brief Virtual destructor.
         */
        virtual ~NetworkSender() = default;

        /**
         * @brief Sends an ASDU.
         * @param svcb The control block.
         * @param asdu The ASDU to send.
         */
        virtual void sendASDU(std::shared_ptr<SampledValueControlBlock> svcb, const ASDU& asdu) = 0;
    };

    /// @brief Ethernet-based network sender for SV. \class EthernetNetworkSender
    class EthernetNetworkSender : public NetworkSender
    {
    public:
        /**
        * @brief Creates a new EthernetNetworkSender.
        * @param interface The network interface name.
        * @return A unique pointer to the sender.
        */
        static std::unique_ptr<EthernetNetworkSender> create(const std::string& interface);

        /**
         * @brief Sends an ASDU.
         * @param svcb The control block.
         * @param asdu The ASDU to send.
         */
        void sendASDU(std::shared_ptr<SampledValueControlBlock> svcb, const ASDU& asdu) override;

        /**
         * @brief Destructor override.
         */
        ~EthernetNetworkSender() override = default;

    private:
        /**
         * @brief Constructor is private. Use create() method.
         * @param interface The network interface name.
         */
        explicit EthernetNetworkSender(std::string interface);

        /**
         * @brief Parses a MAC address from string.
         * @param macStr The MAC address string.
         * @return The MAC address as an array of bytes.
         */
        [[nodiscard]] std::array<uint8_t, 6> parseMacAddress(const std::string& macStr) const;

        /**
         * @brief Gets the source MAC address of the interface.
         * @return The source MAC address as an array of bytes.
         */
        [[nodiscard]] std::array<uint8_t, 6> getSourceMacAddress() const;

        /**
         * @brief Gets the interface index.
         * @return The interface index.
         */
        [[nodiscard]] int getInterfaceIndex() const;

        /**
         * @brief Sends a raw Ethernet frame.
         * @param data Pointer to the frame data.
         * @param size Size of the frame data.
         * @param destMac Destination MAC address.
         */
        void sendFrame(const uint8_t* data, size_t size, const std::array<uint8_t, 6>& destMac) const;

        std::string interface_;
        SocketGuard socket_;
        int ifIndex_;
    };
}