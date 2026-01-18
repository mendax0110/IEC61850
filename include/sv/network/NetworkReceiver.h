#pragma once

#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <atomic>
#include "sv/core/types.h"

/// @brief sv namespace \namespace sv
namespace sv
{
    /// @brief RAII wrapper for socket file descriptor \class ReceiverSocketGuard
    class ReceiverSocketGuard
    {
    public:
        /**
         * @brief Constructor that takes a file descriptor.
         * @param fd The file descriptor to manage.
         */
        explicit ReceiverSocketGuard(const int fd) : fd_(fd) {}

        /**
         * @brief Destructor that closes the file descriptor if valid.
         */
        ~ReceiverSocketGuard()
        {
            if (fd_ >= 0)
            {
                close(fd_);
            }
        }

        /// @brief Disable: copy constructor and assignment
        ReceiverSocketGuard(const ReceiverSocketGuard&) = delete;
        ReceiverSocketGuard& operator=(const ReceiverSocketGuard&) = delete;

        /// @brief Enable: move constructor and assignment
        ReceiverSocketGuard(ReceiverSocketGuard&& other) noexcept : fd_(other.fd_) { other.fd_ = -1; }
        ReceiverSocketGuard& operator=(ReceiverSocketGuard&& other) noexcept
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

    /// @brief Interface for network receiving of SV frames. \class NetworkReceiver
    class NetworkReceiver
    {
    public:
        /**
         * @brief Callback type for received ASDUs.
         */
        using Callback = std::function<void(const ASDU&)>;

        /**
         * @brief Virtual destructor.
         */
        virtual ~NetworkReceiver() = default;

        /**
         * @brief Starts receiving frames.
         * @param callback Function to call when an ASDU is received.
         */
        virtual void start(Callback callback) = 0;

        /**
         * @brief Stops receiving.
         */
        virtual void stop() = 0;
    };

    /**
     * @brief Gets the first available ethernet interface.
     * @return Interface name or empty string if none found.
     */
    std::string getFirstEthernetInterface();

    /// @brief Ethernet-based network receiver for SV. \class EthernetNetworkReceiver
    class EthernetNetworkReceiver : public NetworkReceiver
    {
    public:
        /**
        * @brief Creates a new EthernetNetworkReceiver.
        * @param interface The network interface name.
        * @return A unique pointer to the receiver.
        */
        static std::unique_ptr<EthernetNetworkReceiver> create(const std::string& interface);

        /**
         * @brief Starts receiving frames.
         * @param callback Function to call when an ASDU is received.
         */
        void start(Callback callback) override;

        /**
         * @brief Stops receiving.
         */
        void stop() override;

        /**
         * @brief Destructor override.
         */
        ~EthernetNetworkReceiver() override;

    private:
        /**
         * @brief Constructor is private. Use create() method.
         * @param interface The network interface.
         */
        explicit EthernetNetworkReceiver(std::string interface);

        /**
         * @brief Gets the interface index.
         * @return The interface index.
         */
        [[nodiscard]] int getInterfaceIndex() const;

        /**
         * @brief Enables promiscuous mode on the interface.
         */
        void enablePromiscuousMode() const;

        /**
         * @brief Formats a MAC address as a string.
         * @param mac The MAC address bytes.
         * @return The formatted MAC address string.
         */
        [[nodiscard]] static std::string formatMacAddress(const std::array<uint8_t, 6>& mac);

        /**
         * @brief Parses an ASDU from a buffer.
         * @param buffer The received data buffer.
         * @param length The length of the data.
         * @return An optional ASDU if parsing was successful.
         */
        [[nodiscard]] static std::optional<ASDU> parseASDU(const std::vector<uint8_t>& buffer, size_t length);

        std::string interface_;
        ReceiverSocketGuard socket_;
        int ifIndex_;
        std::atomic<bool> running_;
        std::thread receiveThread_;
    };
}