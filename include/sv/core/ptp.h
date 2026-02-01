#pragma once

#include <chrono>
#include <cstdint>
#include <array>
#include <optional>

/// @brief sv namespace \namespace sv
namespace sv
{
    /// @brief PTP Timestamp representation \class PtpTimestamp
    class PtpTimestamp
    {
    public:
        /**
         * @brief Constructor with seconds and nanoseconds.
         */
        constexpr PtpTimestamp() noexcept
            : seconds_(0)
            , nanoseconds_(0)
            , valid_(false)
        {
        }

        /**
         * @brief Constructor with seconds and nanoseconds.
         * @param seconds The seconds part of the timestamp.
         * @param nanoseconds The nanoseconds part of the timestamp.
         */
        constexpr PtpTimestamp(const uint64_t seconds, const uint32_t nanoseconds) noexcept
            : seconds_(seconds)
            , nanoseconds_(nanoseconds)
            , valid_(nanoseconds < 1'000'000'000)
        {
        }

        /**
         * @brief Getter for the current system time as PtpTimestamp.
         * @return A PtpTimestamp representing the current time.
         */
        [[nodiscard]] static PtpTimestamp now() noexcept
        {
            const auto now = std::chrono::system_clock::now();
            const auto duration = now.time_since_epoch();
            const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
            const auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(duration - seconds);

            return PtpTimestamp(
                static_cast<uint64_t>(seconds.count()),
                static_cast<uint32_t>(nanoseconds.count())
            );
        }

        /**
         * @brief Converts TAI byte array to PtpTimestamp.
         * @param data The 8-byte array representing the TAI timestamp.
         * @return Optional containing PtpTimestamp if conversion was successful, std::nullopt otherwise.
         */
        [[nodiscard]] static std::optional<PtpTimestamp> fromTai(const std::array<uint8_t, 8>& data) noexcept
        {
            uint64_t seconds = 0;
            for (size_t i = 0; i < 4; ++i)
            {
                seconds = (seconds << 8) | data[i];
            }

            uint32_t fraction = 0;
            for (size_t i = 4; i < 8; ++i)
            {
                fraction = (fraction << 8) | data[i];
            }

            constexpr uint64_t fractionToNano = 1'000'000'000ULL;
            const uint32_t nanoseconds = static_cast<uint32_t>((static_cast<uint64_t>(fraction) * fractionToNano) >> 32);

            if (nanoseconds >= 1'000'000'000)
            {
                return std::nullopt;
            }

            return PtpTimestamp(seconds, nanoseconds);
        }

        /**
         * @brief Converts PtpTimestamp to TAI byte array.
         * @return An 8-byte array representing the TAI timestamp.
         */
        [[nodiscard]] constexpr std::array<uint8_t, 8> toTai() const noexcept
        {
            std::array<uint8_t, 8> data{};

            data[0] = static_cast<uint8_t>((seconds_ >> 24) & 0xFF);
            data[1] = static_cast<uint8_t>((seconds_ >> 16) & 0xFF);
            data[2] = static_cast<uint8_t>((seconds_ >> 8) & 0xFF);
            data[3] = static_cast<uint8_t>(seconds_ & 0xFF);

            constexpr uint64_t nanoToFraction = (1ULL << 32);
            const uint32_t fraction = static_cast<uint32_t>((static_cast<uint64_t>(nanoseconds_) * nanoToFraction) / 1'000'000'000ULL);

            data[4] = static_cast<uint8_t>((fraction >> 24) & 0xFF);
            data[5] = static_cast<uint8_t>((fraction >> 16) & 0xFF);
            data[6] = static_cast<uint8_t>((fraction >> 8) & 0xFF);
            data[7] = static_cast<uint8_t>(fraction & 0xFF);

            return data;
        }

        /**
         * @brief Getter for the seconds part of the timestamp.
         * @return A uint64_t representing the seconds.
         */
        [[nodiscard]] constexpr uint64_t getSeconds() const noexcept { return seconds_; }

        /**
         * @brief Getter for the nanoseconds part of the timestamp.
         * @return A uint32_t representing the nanoseconds.
         */
        [[nodiscard]] constexpr uint32_t getNanoseconds() const noexcept { return nanoseconds_; }

        /**
         * @brief Checks if the timestamp is valid.
         * @return True if valid, false otherwise.
         */
        [[nodiscard]] constexpr bool isValid() const noexcept { return valid_; }

        /**
         * @brief Converts PtpTimestamp to std::chrono::system_clock::time_point.
         * @return A time_point representing the timestamp.
         */
        [[nodiscard]] std::chrono::system_clock::time_point toTimePoint() const noexcept
        {
            const auto duration = std::chrono::seconds(seconds_) + std::chrono::nanoseconds(nanoseconds_);
            return std::chrono::system_clock::time_point(std::chrono::duration_cast<std::chrono::system_clock::duration>(duration));
        }

        /**
         * @brief Equality comparison operator.
         * @param other The other PtpTimestamp to compare.
         * @return A bool indicating whether the timestamps are equal.
         */
        constexpr bool operator==(const PtpTimestamp& other) const noexcept
        {
            return seconds_ == other.seconds_ && nanoseconds_ == other.nanoseconds_;
        }

        /**
         * @brief Inequality comparison operator.
         * @param other The other PtpTimestamp to compare.
         * @return A bool indicating whether the timestamps are not equal.
         */
        constexpr bool operator!=(const PtpTimestamp& other) const noexcept
        {
            return !(*this == other);
        }

        /**
         * @brief Less-than comparison operator.
         * @param other The other PtpTimestamp to compare.
         * @return A bool indicating whether this timestamp is less than the other.
         */
        constexpr bool operator<(const PtpTimestamp& other) const noexcept
        {
            if (seconds_ < other.seconds_) return true;
            if (seconds_ > other.seconds_) return false;
            return nanoseconds_ < other.nanoseconds_;
        }

        /**
         * @brief Greater-than comparison operator.
         * @param other The other PtpTimestamp to compare.
         * @return A bool indicating whether this timestamp is greater than the other.
         */
        constexpr bool operator>(const PtpTimestamp& other) const noexcept
        {
            return other < *this;
        }

        /**
         * @brief Less-than-or-equal comparison operator.
         * @param other The other PtpTimestamp to compare.
         * @return A bool indicating whether this timestamp is less than or equal to the other.
         */
        constexpr bool operator<=(const PtpTimestamp& other) const noexcept
        {
            return !(other < *this);
        }

        /**
         * @brief Greater-than-or-equal comparison operator.
         * @param other The other PtpTimestamp to compare.
         * @return A bool indicating whether this timestamp is greater than or equal to the other.
         */
        constexpr bool operator>=(const PtpTimestamp& other) const noexcept
        {
            return !(*this < other);
        }

    private:
        uint64_t seconds_;
        uint32_t nanoseconds_;
        bool valid_;
    };
}