#pragma once

#include <memory>
#include <string>
#include "sv/core/types.h"
#include "sv/model/SampledValueControlBlock.h"

/// @brief sv namespace \namespace sv
namespace sv
{
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
        ~EthernetNetworkSender() override;

    public:
        /**
         * @brief Constructor is private. Use create() method.
         * @param interface The network interface name.
         */
        EthernetNetworkSender(std::string  interface);

        std::string interface_;
        int socket_;
    };
}