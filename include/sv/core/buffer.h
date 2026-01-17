#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <arpa/inet.h>
#include <endian.h>

/// @brief sv namespace \namespace sv
namespace sv
{
    /// @brief Buffer writer for serialization \class BufferWriter
    class BufferWriter
    {
    public:
        /**
         * @brief Constructor with optional initial capacity.
         * @param capacity The initial capacity of the buffer.
         */
        explicit BufferWriter(const size_t capacity = 1500)
        {
            buffer_.reserve(capacity);
        }

        /**
         * @brief Writes a uint8_t to the buffer.
         * @param value The uint8_t value to write.
         */
        void writeUint8(const uint8_t value)
        {
            buffer_.push_back(value);
        }

        /**
         * @brief Writes a uint16_t to the buffer in big-endian order.
         * @param value The uint16_t value to write.
         */
        void writeUint16(const uint16_t value)
        {
            const uint16_t net_value = htobe16(value);
            const auto bytes = reinterpret_cast<const uint8_t*>(&net_value);
            buffer_.insert(buffer_.end(), bytes, bytes + sizeof(net_value));
        }

        /**
         * @brief Writes a uint32_t to the buffer in big-endian order.
         * @param value The uint32_t value to write.
         */
        void writeUint32(const uint32_t value)
        {
            const uint32_t net_value = htobe32(value);
            const auto bytes = reinterpret_cast<const uint8_t*>(&net_value);
            buffer_.insert(buffer_.end(), bytes, bytes + sizeof(net_value));
        }

        /**
         * @brief Writes a uint64_t to the buffer in big-endian order.
         * @param value The uint64_t value to write.
         */
        void writeUint64(const uint64_t value)
        {
            const uint64_t net_value = htobe64(value);
            const auto bytes = reinterpret_cast<const uint8_t*>(&net_value);
            buffer_.insert(buffer_.end(), bytes, bytes + sizeof(net_value));
        }

        /**
         * @brief Writes an int16_t to the buffer in big-endian order.
         * @param value The int16_t value to write.
         */
        void writeInt16(const int16_t value)
        {
            writeUint16(static_cast<uint16_t>(value));
        }

        /**
         * @brief Writes a float to the buffer in IEEE 754 format.
         * @param value The float value to write.
         */
        void writeFloat(const float value)
        {
            uint32_t asInt;
            std::memcpy(&asInt, &value, sizeof(float));
            writeUint32(asInt);
        }

        /**
         * @brief Writes raw bytes to the buffer.
         * @param data The pointer to the byte data.
         * @param length The number of bytes to write.
         */
        void writeBytes(const uint8_t* data, const size_t length)
        {
            buffer_.insert(buffer_.end(), data, data + length);
        }

        /**
         * @brief Writes a fixed-size string to the buffer, padding with null bytes if necessary.
         * @param str The string to write.
         * @param fixedSize The fixed size of the string field.
         */
        void writeFixedString(const std::string& str, const size_t fixedSize)
        {
            const size_t copyLen = std::min(str.size(), fixedSize);
            buffer_.insert(buffer_.end(), str.begin(), str.begin() + copyLen);
            for (size_t i = copyLen; i < fixedSize; ++i)
            {
                buffer_.push_back(0);
            }
        }

        /**
         * @brief Writes a uint16_t at a specific position in the buffer in big-endian order.
         * @param pos The position to write the uint16_t.
         * @param value The uint16_t value to write.
         */
        void writeUint16At(const size_t pos, const uint16_t value)
        {
            if (pos + 2 <= buffer_.size())
            {
                const uint16_t net_value = htobe16(value);
                std::memcpy(&buffer_[pos], &net_value, sizeof(net_value));
            }
        }

        /**
         * @brief Reserves space in the buffer and returns the starting index.
         * @param bytes The number of bytes to reserve.
         * @return A size_t index where the reserved space starts.
         */
        size_t reserve(const size_t bytes)
        {
            const size_t currentSize = buffer_.size();
            buffer_.resize(currentSize + bytes);
            return currentSize;
        }

        /**
         * @brief Getter for the size of the buffer.
         * @return A size_t representing the current size of the buffer.
         */
        size_t size() const
        {
            return buffer_.size();
        }

        /**
         * @brief Getter for the raw data pointer of the buffer.
         * @return A pointer to the buffer's data.
         */
        const uint8_t* data() const
        {
            return buffer_.data();
        }

        /**
         * @brief Getter for the raw data pointer of the buffer (non-const).
         * @return A pointer to the buffer's data.
         */
        uint8_t* date()
        {
            return buffer_.data();
        }

        /**
         * @brief Clears the buffer.
         */
        void clear()
        {
            buffer_.clear();
        }

    private:
        std::vector<uint8_t> buffer_;
    };

    /// @brief Buffer reader for deserialization \class BufferReader
    class BufferReader
    {
    public:
        /**
         * @brief Constructs a BufferReader with given data and length.
         * @param data The pointer to the byte data.
         * @param length The length of the data.
         */
        BufferReader(const uint8_t* data, size_t length)
            : data_(data)
            , length_(length)
            , pos_(0)
        {

        }

        /**
         * @brief Reads a uint16_t from the buffer in big-endian order.
         * @return The uint16_t value read.
         */
        uint8_t readUint8()
        {
            if (pos_ >= length_)
            {
                return 0;
            }
            return data_[pos_++];
        }

        /**
         * @brief Reads a uint16_t from the buffer in big-endian order.
         * @return The uint16_t value read.
         */
        uint16_t readUint16()
        {
            if (pos_ + 2 > length_)
            {
                return 0;
            }
            uint16_t net_value;
            std::memcpy(&net_value, &data_[pos_], sizeof(net_value));
            pos_ += 2;
            return be16toh(net_value);
        }

        /**
         * @brief Reads a uint32_t from the buffer in big-endian order.
         * @return The uint32_t value read.
         */
        uint32_t readUint32()
        {
            if (pos_ + 4 > length_)
            {
                return 0;
            }
            uint32_t net_value;
            std::memcpy(&net_value, &data_[pos_], sizeof(net_value));
            pos_ += 4;
            return be32toh(net_value);
        }

        /**
         * @brief Reads a uint64_t from the buffer in big-endian order.
         * @return The uint64_t value read.
         */
        uint64_t readUint64()
        {
            if (pos_ + 8 > length_)
            {
                return 0;
            }
            uint64_t net_value;
            std::memcpy(&net_value, &data_[pos_], sizeof(net_value));
            pos_ += 8;
            return be64toh(net_value);
        }

        /**
         * @brief Reads an int16_t from the buffer in big-endian order.
         * @return A int16_t value read.
         */
        int16_t readInt16()
        {
            return static_cast<int16_t>(readUint16());
        }

        /**
         * @brief Reads a 32-bit float in network byte order.
         * @return Value read.
         */
        float readFloat()
        {
            const uint32_t bits = readUint32();
            float value;
            std::memcpy(&value, &bits, sizeof(value));
            return value;
        }

        /**
         * @brief Reads a fixed-size string from the buffer, trimming at the first null byte.
         * @param fixedSize The fixed size of the string field.
         * @return The string read.
         */
        std::string readFixedString(size_t fixedSize)
        {
            if (pos_ + fixedSize > length_)
            {
                fixedSize = length_ - pos_;
            }
            std::string str(reinterpret_cast<const char*>(&data_[pos_]), fixedSize);
            pos_ += fixedSize;

            const size_t null_pos = str.find('\0');
            if (null_pos != std::string::npos)
            {
                str.resize(null_pos);
            }

            return str;
        }

        /**
         * @brief Skips a number of bytes in the buffer.
         * @param bytes The number of bytes to skip.
         */
        void skip(const size_t bytes)
        {
            pos_ = std::min(pos_ + bytes, length_);
        }

        /**
         * @brief Getter for the current position in the buffer.
         * @return A size_t representing the current position.
         */
        size_t position() const
        {
            return pos_;
        }

        /**
         * @brief Getter for the remaining bytes in the buffer.
         * @return A size_t representing the number of remaining bytes.
         */
        size_t remaining() const
        {
            return length_ > pos_ ? length_ - pos_ : 0;
        }

        /**
         * @brief Checks if there are more bytes to read in the buffer.
         * @return true if more bytes are available, false otherwise.
         */
        bool hasMore() const
        {
            return pos_ < length_;
        }

    private:
        const uint8_t* data_;
        size_t length_;
        size_t pos_;
    };
}