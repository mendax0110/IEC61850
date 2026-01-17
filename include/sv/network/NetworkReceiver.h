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

        std::string interface_;
        int socket_;
        std::atomic<bool> running_;
        std::thread receiveThread_;
    };
}