#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <mutex>
#include <chrono>
#include <stdexcept>
#include <memory>

/// @brief sv namespace \namespace sv
namespace sv
{
    /// @brief Pcap file global header structure \struct PcapGlobalHeader
    struct PcapGlobalHeader
    {
        uint32_t magicNumber{0xA1B2C3D4};
        uint16_t versionMajor{2};
        uint16_t versionMinor{4};
        int32_t thisZone{0};
        uint32_t sigFigs{0};
        uint32_t snapLen{65535};
        uint32_t network{1};
    };

    /// @brief Pcap file packet header structure \struct PcapPacketHeader
    struct PcapPacketHeader
    {
        uint32_t tsSec{0};
        uint32_t tsUsec{0};
        uint32_t inclLen{0};
        uint32_t origLen{0};
    };

    /// @brief PcapWriter class for writing pcap files \class PcapWriter
    class PcapWriter
    {
    public:
        /**
         * @brief Constructor is private. Use create() method.
         * @param filePath Path to the pcap file to write.
         * @return A unique pointer to the created PcapWriter.
         */
        static std::unique_ptr<PcapWriter> create(const std::string& filePath)
        {
            return std::unique_ptr<PcapWriter>(new PcapWriter(filePath));
        }

        ~PcapWriter()
        {
            close();
        }

        PcapWriter(const PcapWriter&) = delete;
        PcapWriter& operator=(const PcapWriter&) = delete;
        PcapWriter(PcapWriter&&) noexcept = delete;
        PcapWriter& operator=(PcapWriter&&) noexcept = delete;

        /**
         * @brief Writes frames to the pcap file.
         * @param data The raw frame data to write.
         * @param length The length of the frame data in bytes.
         */
        void writeFrame(const uint8_t* data, size_t length)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!file_.is_open())
            {
                return;
            }

            const auto now = std::chrono::system_clock::now();
            const auto duration = now.time_since_epoch();
            const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
            const auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration - seconds);

            PcapPacketHeader pktHeader;
            pktHeader.tsSec = static_cast<uint32_t>(seconds.count());
            pktHeader.tsUsec = static_cast<uint32_t>(microseconds.count());
            pktHeader.inclLen = static_cast<uint32_t>(length);
            pktHeader.origLen = static_cast<uint32_t>(length);

            file_.write(reinterpret_cast<const char*>(&pktHeader), sizeof(pktHeader));
            file_.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(length));
            packetCount_++;
        }

        /**
         * @brief Closes the pcap file.
         */
        void close()
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (file_.is_open())
            {
                file_.flush();
                file_.close();
            }
        }

        /**
         * @brief Getter for the number of packets written to the pcap file.
         * @return A size_t representing the number of packets written.
         */
        [[nodiscard]] size_t getPacketCount() const
        {
            return packetCount_;
        }

        /**
         * @brief Checks if the pcap file is currently open.
         * @return A bool indicating if the file is open.
         */
        [[nodiscard]] bool isOpen() const
        {
            return file_.is_open();
        }

    private:
        /**
         * @brief Constructor is private. Use create() method.
         * @param filePath The path to the pcap file to write.
         */
        explicit PcapWriter(const std::string& filePath)
            : file_(filePath, std::ios::binary | std::ios::trunc)
        {
            if (!file_.is_open())
            {
                throw std::runtime_error("Failed to open pcap file: " + filePath);
            }

            constexpr PcapGlobalHeader globalHeader;
            file_.write(reinterpret_cast<const char*>(&globalHeader), sizeof(globalHeader));
        }

        std::ofstream file_;
        std::mutex mutex_;
        size_t packetCount_{0};
    };
}