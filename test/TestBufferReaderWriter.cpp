#include <gtest/gtest.h>
#include "sv/model/IedModel.h"
#include "sv/model/IedServer.h"
#include "sv/model/IedClient.h"
#include "sv/model/LogicalNode.h"
#include "sv/model/SampledValueControlBlock.h"
#include "sv/core/mac.h"
#include "sv/core/buffer.h"


TEST(BufferWriterTest, WriteUint8)
{
    sv::BufferWriter writer;
    writer.writeUint8(0x42);
    EXPECT_EQ(writer.size(), 1);
    EXPECT_EQ(writer.data()[0], 0x42);
}

TEST(BufferWriterTest, WriteUint16)
{
    sv::BufferWriter writer;
    writer.writeUint16(0x1234);
    EXPECT_EQ(writer.size(), 2);
    EXPECT_EQ(writer.data()[0], 0x12);
    EXPECT_EQ(writer.data()[1], 0x34);
}

TEST(BufferWriterTest, WriteUint32)
{
    sv::BufferWriter writer;
    writer.writeUint32(0x12345678);
    EXPECT_EQ(writer.size(), 4);
    EXPECT_EQ(writer.data()[0], 0x12);
    EXPECT_EQ(writer.data()[1], 0x34);
    EXPECT_EQ(writer.data()[2], 0x56);
    EXPECT_EQ(writer.data()[3], 0x78);
}

TEST(BufferWriterTest, WriteUint64)
{
    sv::BufferWriter writer;
    writer.writeUint64(0x123456789ABCDEF0);
    EXPECT_EQ(writer.size(), 8);
    EXPECT_EQ(writer.data()[0], 0x12);
    EXPECT_EQ(writer.data()[7], 0xF0);
}

TEST(BufferWriterTest, WriteInt16)
{
    sv::BufferWriter writer;
    writer.writeInt16(-1234);
    EXPECT_EQ(writer.size(), 2);
}

TEST(BufferWriterTest, WriteFloat)
{
    sv::BufferWriter writer;
    writer.writeFloat(3.14159f);
    EXPECT_EQ(writer.size(), 4);
}

TEST(BufferWriterTest, WriteBytes)
{
    sv::BufferWriter writer;
    uint8_t data[] = {0x01, 0x02, 0x03};
    writer.writeBytes(data, 3);
    EXPECT_EQ(writer.size(), 3);
    EXPECT_EQ(writer.data()[0], 0x01);
    EXPECT_EQ(writer.data()[2], 0x03);
}

TEST(BufferWriterTest, WriteFixedString)
{
    sv::BufferWriter writer;
    writer.writeFixedString("Hello", 10);
    EXPECT_EQ(writer.size(), 10);
    EXPECT_EQ(writer.data()[0], 'H');
    EXPECT_EQ(writer.data()[4], 'o');
    EXPECT_EQ(writer.data()[5], 0); // Padding
}

TEST(BufferWriterTest, WriteUint16At)
{
    sv::BufferWriter writer;
    writer.writeUint16(0x0000);
    writer.writeUint16At(0, 0xABCD);
    EXPECT_EQ(writer.data()[0], 0xAB);
    EXPECT_EQ(writer.data()[1], 0xCD);
}

TEST(BufferWriterTest, WriteUint16AtOutOfRange)
{
    sv::BufferWriter writer;
    writer.writeUint8(0x01);
    EXPECT_THROW(writer.writeUint16At(10, 0x1234), std::out_of_range);
}

TEST(BufferWriterTest, Reserve)
{
    sv::BufferWriter writer;
    const size_t pos = writer.reserve(10);
    EXPECT_EQ(pos, 0);
    EXPECT_EQ(writer.size(), 10);
}

TEST(BufferWriterTest, Clear)
{
    sv::BufferWriter writer;
    writer.writeUint32(0x12345678);
    EXPECT_EQ(writer.size(), 4);
    writer.clear();
    EXPECT_EQ(writer.size(), 0);
}

TEST(BufferReaderTest, ReadUint8)
{
    uint8_t data[] = {0x42};
    sv::BufferReader reader(data, 1);
    EXPECT_EQ(reader.readUint8(), 0x42);
}

TEST(BufferReaderTest, ReadUint16)
{
    uint8_t data[] = {0x12, 0x34};
    sv::BufferReader reader(data, 2);
    EXPECT_EQ(reader.readUint16(), 0x1234);
}

TEST(BufferReaderTest, ReadUint32)
{
    uint8_t data[] = {0x12, 0x34, 0x56, 0x78};
    sv::BufferReader reader(data, 4);
    EXPECT_EQ(reader.readUint32(), 0x12345678);
}

TEST(BufferReaderTest, ReadUint64)
{
    uint8_t data[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
    sv::BufferReader reader(data, 8);
    EXPECT_EQ(reader.readUint64(), 0x123456789ABCDEF0);
}

TEST(BufferReaderTest, ReadInt16)
{
    uint8_t data[] = {0xFF, 0xFF};
    sv::BufferReader reader(data, 2);
    EXPECT_EQ(reader.readInt16(), -1);
}

TEST(BufferReaderTest, ReadFloat)
{
    sv::BufferWriter writer;
    writer.writeFloat(3.14159f);
    sv::BufferReader reader(writer.data(), writer.size());
    EXPECT_NEAR(reader.readFloat(), 3.14159f, 0.00001f);
}

TEST(BufferReaderTest, ReadFixedString)
{
    sv::BufferWriter writer;
    writer.writeFixedString("Hello", 10);
    sv::BufferReader reader(writer.data(), writer.size());
    EXPECT_EQ(reader.readFixedString(10), "Hello");
}

TEST(BufferReaderTest, Skip)
{
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    sv::BufferReader reader(data, 4);
    reader.skip(2);
    EXPECT_EQ(reader.readUint8(), 0x03);
}

TEST(BufferReaderTest, Position)
{
    uint8_t data[] = {0x01, 0x02, 0x03};
    sv::BufferReader reader(data, 3);
    EXPECT_EQ(reader.position(), 0);
    reader.readUint8();
    EXPECT_EQ(reader.position(), 1);
}

TEST(BufferReaderTest, Remaining)
{
    uint8_t data[] = {0x01, 0x02, 0x03};
    sv::BufferReader reader(data, 3);
    EXPECT_EQ(reader.remaining(), 3);
    reader.readUint8();
    EXPECT_EQ(reader.remaining(), 2);
}

TEST(BufferReaderTest, HasMore)
{
    uint8_t data[] = {0x01};
    sv::BufferReader reader(data, 1);
    EXPECT_TRUE(reader.hasMore());
    reader.readUint8();
    EXPECT_FALSE(reader.hasMore());
}

TEST(BufferReaderTest, Reset)
{
    uint8_t data[] = {0x01, 0x02, 0x03};
    sv::BufferReader reader(data, 3);
    reader.readUint8();
    reader.reset();
    EXPECT_EQ(reader.position(), 0);
}

TEST(BufferReaderTest, Seek)
{
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    sv::BufferReader reader(data, 4);
    reader.seek(2);
    EXPECT_EQ(reader.position(), 2);
    EXPECT_EQ(reader.readUint8(), 0x03);
}

TEST(BufferReaderTest, SeekOutOfRange)
{
    uint8_t data[] = {0x01};
    sv::BufferReader reader(data, 1);
    EXPECT_THROW(reader.seek(10), std::out_of_range);
}

TEST(BufferReaderTest, ReadBeyondEnd)
{
    uint8_t data[] = {0x01};
    sv::BufferReader reader(data, 1);
    reader.readUint8();
    EXPECT_EQ(reader.readUint8(), 0);
}