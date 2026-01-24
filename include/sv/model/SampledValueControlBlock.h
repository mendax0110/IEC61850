#pragma once

#include <memory>
#include <string>
#include <vector>
#include "sv/core/types.h"

/// @brief sv namespace \namespace sv
namespace sv
{

    /// @brief Represents a Sampled Value Control Block in IEC 61850. \class SampledValueControlBlock
    class SampledValueControlBlock
    {
    public:
        /**
         * @brief Shared pointer type for SampledValueControlBlock.
         */
        using Ptr = std::shared_ptr<SampledValueControlBlock>;

        /**
         * @brief Creates a new SampledValueControlBlock.
         * @param name The name of the control block.
         * @return A shared pointer to the created SVCB.
         */
        static Ptr create(const std::string& name);

        /**
         * @brief Sets the multicast address.
         * @param address The MAC address as string (e.g., "01:0C:CD:01:00:01").
         */
        void setMulticastAddress(const std::string& address);

        /**
         * @brief Gets the multicast address.
         * @return The MAC address.
         */
        [[nodiscard]] const std::string& getMulticastAddress() const;

        /**
         * @brief Sets the AppID.
         * @param appId The 16-bit AppID.
         */
        void setAppId(uint16_t appId = DEFAULT_APP_ID);

        /**
         * @brief Gets the AppID.
         * @return The AppID.
         */
        [[nodiscard]] uint16_t getAppId() const;

        /**
         * @brief Sets the sampling rate.
         * @param rate The sampling rate in Hz.
         */
        void setSmpRate(uint16_t rate);

        /**
         * @brief Gets the sampling rate.
         * @return The sampling rate.
         */
        [[nodiscard]] uint16_t getSmpRate() const;

        /**
         * @brief Sets the DataSet reference.
         * @param dataSet The name of the DataSet.
         */
        void setDataSet(const std::string& dataSet);

        /**
         * @brief Gets the DataSet reference.
         * @return The DataSet name.
         */
        [[nodiscard]] const std::string& getDataSet() const;

        /**
         * @brief Gets the name of the control block.
         * @return The name.
         */
        [[nodiscard]] const std::string& getName() const;

        /**
         * @brief Sets the configuration revision.
         * @param revision The configuration revision number.
         */
        void setConfRev(uint32_t revision);

        /**
         * @brief Gets the configuration revision.
         * @return The configuration revision number.
         */
        [[nodiscard]] uint32_t getConfRev() const;

        /**
         * @brief Sets the synchronization method.
         * @param synch The synchronization method.
         */
        void setSmpSynch(SmpSynch synch);

        /**
         * @brief Gets the synchronization method.
         * @return The synchronization method.
         */
        [[nodiscard]] SmpSynch getSmpSynch() const;

        /**
         * @brief Sets the VLAN ID.
         * @param vlanId The VLAN ID.
         */
        void setVlanId(uint16_t vlanId);

        /**
         * @brief Gets the VLAN ID.
         * @return The VLAN ID.
         */
        [[nodiscard]] uint16_t getVlanId() const;

        /**
         * @brief Sets the user priority.
         * @param priority The user priority (0-7).
         */
        void setUserPriority(uint8_t priority);

        /**
         * @brief Gets the user priority.
         * @return The user priority.
         */
        [[nodiscard]] uint8_t getUserPriority() const;

        /**
         * @brief Sets whether to simulate the SV transmission.
         * @param simulate True to simulate, false otherwise.
         */
        void setSimulate(bool simulate);

        /**
         * @brief Gets whether simulation is enabled.
         * @return True if simulation is enabled, false otherwise.
         */
        [[nodiscard]] bool getSimulate() const;

        /**
         * @brief Sets the samples per period.
         * @param spp The samples per period.
         */
        void setSamplesPerPeriod(SamplesPerPeriod spp);

        /**
         * @brief Gets the samples per period.
         * @return The samples per period.
         */
        [[nodiscard]] SamplesPerPeriod getSamplesPerPeriod() const;

        /**
         * @brief Sets the signal frequency.
         * @param freq The signal frequency.
         */
        void setSignalFrequency(SignalFrequency freq);

        /**
         * @brief Gets the signal frequency.
         * @return The signal frequency.
         */
        [[nodiscard]] SignalFrequency getSignalFrequency() const;

        /**
         * @brief Sets the grandmaster identity.
         * @param identity The 8-byte grandmaster identity.
         */
        void setGrandmasterIdentity(const std::array<uint8_t, 8>& identity);

        /**
         * @brief Gets the grandmaster identity.
         * @return The grandmaster identity.
         */
        [[nodiscard]] std::optional<std::array<uint8_t, 8>> getGrandmasterIdentity() const;

        /**
         * @brief Clears the grandmaster identity.
         */
        void clearGrandmasterIdentity();

        /**
         * @brief Sets the data type of the sampled values.
         * @param type The data type.
         */
        void setDataType(DataType type);

        /**
         * @brief Gets the data type of the sampled values.
         * @return The data type.
         */
        [[nodiscard]] DataType getDataType() const;

        /**
         * @brief Sets the current scaling factor.
         * @param factor The scaling factor.
         */
        void setCurrentScaling(int32_t factor);

        /**
         * @brief Gets the current scaling factor.
         * @return The scaling factor.
         */
        [[nodiscard]] int32_t getCurrentScaling() const;

        /**
         * @brief Sets the voltage scaling factor.
         * @param factor The scaling factor.
         */
        void setVoltageScaling(int32_t factor);

        /**
         * @brief Gets the voltage scaling factor.
         * @return The scaling factor.
         */
        [[nodiscard]] int32_t getVoltageScaling() const;

        /**
         * @brief Converts to PublisherConfig structure.
         * @return The PublisherConfig representation.
         */
        [[nodiscard]] PublisherConfig toPublisherConfig() const;

    private:
        /**
         * @brief Constructor is private. Use create() method.
         * @param name The name of the control block.
         */
        explicit SampledValueControlBlock(std::string name);

        std::string name_;
        std::string multicastAddress_;
        uint16_t appId_;
        uint16_t smpRate_;
        std::string dataSet_;

        uint32_t confRev_{1};
        SmpSynch smpSynch_{SmpSynch::None};
        uint16_t vlanId_{0};
        uint8_t userPriority_{4};
        bool simulate_{false};
        SamplesPerPeriod samplesPerPeriod_{SamplesPerPeriod::SPP_80};
        SignalFrequency signalFrequency_{SignalFrequency::FREQ_50_HZ};
        std::optional<std::array<uint8_t, 8>> gmIdentity_;
        DataType dataType_{DataType::INT32};
        int32_t currentScaling_{ScalingFactors::CURRENT_DEFAULT};
        int32_t voltageScaling_{ScalingFactors::VOLTAGE_DEFAULT};
    };
}