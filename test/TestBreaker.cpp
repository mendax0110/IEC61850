#include <gtest/gtest.h>
#include "../include/sv/sim/Breaker.h"
#include <thread>
#include <chrono>
#include <cmath>

using namespace sv::sim;

class BreakerModelTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        breaker = BreakerModel::create();
    }

    void TearDown() override
    {
        breaker.reset();
    }

    BreakerModel::BreakerPtr breaker;
};

TEST_F(BreakerModelTest, CreateWithDefaultDefinition)
{
    ASSERT_NE(breaker, nullptr);
    EXPECT_EQ(breaker->getState(), BreakerState::OPEN);
    EXPECT_TRUE(breaker->isOpen());
    EXPECT_FALSE(breaker->isClosed());
}

TEST_F(BreakerModelTest, CreateWithCustomDefinition)
{
    BreakerDefinition def;
    def.maxCurrentA = 2000.0;
    def.voltageRatingV = 800.0;
    def.openTimeSec = 0.030;
    def.closeTimeSec = 0.080;

    const auto customBreaker = BreakerModel::create(def);
    ASSERT_NE(customBreaker, nullptr);

    const auto retrievedDef = customBreaker->getDefinition();
    EXPECT_DOUBLE_EQ(retrievedDef.maxCurrentA, 2000.0);
    EXPECT_DOUBLE_EQ(retrievedDef.voltageRatingV, 800.0);
    EXPECT_DOUBLE_EQ(retrievedDef.openTimeSec, 0.030);
    EXPECT_DOUBLE_EQ(retrievedDef.closeTimeSec, 0.080);
}

TEST_F(BreakerModelTest, InvalidDefinitionThrows)
{
    BreakerDefinition invalidDef;
    invalidDef.maxCurrentA = -100.0;

    EXPECT_THROW(BreakerModel::create(invalidDef), std::invalid_argument);
}

TEST_F(BreakerModelTest, CloseBreaker)
{
    ASSERT_TRUE(breaker->close());
    EXPECT_TRUE(breaker->isClosing());

    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    EXPECT_TRUE(breaker->isClosed());
    EXPECT_FALSE(breaker->isOpen());
    EXPECT_EQ(breaker->getState(), BreakerState::CLOSED);
}

TEST_F(BreakerModelTest, OpenBreaker)
{
    breaker->close();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    ASSERT_TRUE(breaker->open());
    EXPECT_TRUE(breaker->isOpening());

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_TRUE(breaker->isOpen());
    EXPECT_FALSE(breaker->isClosed());
    EXPECT_EQ(breaker->getState(), BreakerState::OPEN);
}

TEST_F(BreakerModelTest, CannotCloseAlreadyClosed)
{
    breaker->close();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    EXPECT_FALSE(breaker->close());
}

TEST_F(BreakerModelTest, CannotOpenAlreadyOpen)
{
    EXPECT_FALSE(breaker->open());
}

TEST_F(BreakerModelTest, LockInOpenPosition)
{
    breaker->lock();

    EXPECT_TRUE(breaker->isLocked());
    EXPECT_EQ(breaker->getState(), BreakerState::LOCKED_OPEN);

    EXPECT_FALSE(breaker->close());
}

TEST_F(BreakerModelTest, LockInClosedPosition)
{
    breaker->close();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    breaker->lock();

    EXPECT_TRUE(breaker->isLocked());
    EXPECT_EQ(breaker->getState(), BreakerState::LOCKED_CLOSED);

    EXPECT_FALSE(breaker->open());
}

TEST_F(BreakerModelTest, UnlockBreaker)
{
    breaker->lock();
    EXPECT_TRUE(breaker->isLocked());

    breaker->unlock();
    EXPECT_FALSE(breaker->isLocked());
    EXPECT_EQ(breaker->getState(), BreakerState::OPEN);
}

TEST_F(BreakerModelTest, EmergencyTrip)
{
    breaker->close();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    breaker->trip();

    EXPECT_TRUE(breaker->isOpen());
    EXPECT_EQ(breaker->getState(), BreakerState::OPEN);
    EXPECT_DOUBLE_EQ(breaker->getCurrent(), 0.0);
}

TEST_F(BreakerModelTest, TripUnlocksBreaker)
{
    breaker->close();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    breaker->lock();

    EXPECT_TRUE(breaker->isLocked());

    breaker->trip();

    EXPECT_FALSE(breaker->isLocked());
    EXPECT_TRUE(breaker->isOpen());
}

TEST_F(BreakerModelTest, ResistanceWhenOpen)
{
    const double resistance = breaker->getResistance();
    EXPECT_TRUE(std::isinf(resistance));
}

TEST_F(BreakerModelTest, ResistanceWhenClosed)
{
    breaker->close();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    const double resistance = breaker->getResistance();
    const auto def = breaker->getDefinition();

    EXPECT_DOUBLE_EQ(resistance, def.resistanceOhm);
}

TEST_F(BreakerModelTest, ResistanceDuringTransition)
{
    breaker->close();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    EXPECT_TRUE(breaker->isInTransition());

    const double resistance = breaker->getResistance();
    const auto def = breaker->getDefinition();

    EXPECT_GT(resistance, def.resistanceOhm);
    EXPECT_LT(resistance, def.arcResistanceOhm * 2.0);
}

TEST_F(BreakerModelTest, SetAndGetCurrent)
{
    breaker->close();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    breaker->setCurrent(500.0);
    EXPECT_DOUBLE_EQ(breaker->getCurrent(), 500.0);

    breaker->setCurrent(-300.0);
    EXPECT_DOUBLE_EQ(breaker->getCurrent(), -300.0);
}

TEST_F(BreakerModelTest, OverloadProtection)
{
    breaker->close();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    const auto def = breaker->getDefinition();
    breaker->setCurrent(def.maxCurrentA * 1.5);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    EXPECT_TRUE(breaker->isOpen());
}

TEST_F(BreakerModelTest, IsOverloaded)
{
    breaker->close();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    const auto def = breaker->getDefinition();

    breaker->setCurrent(def.maxCurrentA * 0.5);
    EXPECT_FALSE(breaker->isOverloaded());

    breaker->setCurrent(def.maxCurrentA * 0.9);
    EXPECT_FALSE(breaker->isOverloaded());

    const double overloadCurrent = def.maxCurrentA * 1.2;
    breaker->setCurrent(overloadCurrent);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_TRUE(breaker->isOpen());
}

TEST_F(BreakerModelTest, StateChangeCallback)
{
    int callbackCount = 0;
    auto lastOldState = BreakerState::OPEN;
    auto lastNewState = BreakerState::OPEN;

    breaker->onStateChange([&](const BreakerState oldState, const BreakerState newState)
    {
        ++callbackCount;
        lastOldState = oldState;
        lastNewState = newState;
    });

    breaker->close();
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    EXPECT_GT(callbackCount, 0);
    EXPECT_EQ(lastNewState, BreakerState::CLOSING);

    std::this_thread::sleep_for(std::chrono::milliseconds(150));

    EXPECT_GT(callbackCount, 1);
    EXPECT_EQ(lastNewState, BreakerState::CLOSED);
}

TEST_F(BreakerModelTest, ArcVoltageWhenOpen)
{
    const double arcVoltage = breaker->getArcVoltage();
    EXPECT_DOUBLE_EQ(arcVoltage, 0.0);
}

TEST_F(BreakerModelTest, ArcVoltageDuringOpening)
{
    breaker->close();
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    breaker->setCurrent(500.0);

    breaker->open();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    const double arcVoltage = breaker->getArcVoltage();
    EXPECT_GT(arcVoltage, 0.0);
}

TEST_F(BreakerModelTest, SimulationWithNominalCurrent)
{
    const auto result = breaker->runSimulation(
        400.0,
        100.0,
        0.0,
        10.0,
        0.5,
        0.01
    );

    EXPECT_FALSE(result.tripOccurred);
    EXPECT_EQ(result.tripTime, 0.0);
    EXPECT_GT(result.timePoints.size(), 0);
    EXPECT_EQ(result.timePoints.size(), result.currentValues.size());
    EXPECT_EQ(result.timePoints.size(), result.stateHistory.size());
}

TEST_F(BreakerModelTest, SimulationWithFault)
{
    const auto def = breaker->getDefinition();
    const auto result = breaker->runSimulation(
        400.0,
        100.0,
        def.maxCurrentA * 1.5,
        0.1,
        0.5,
        0.01
    );

    EXPECT_TRUE(result.tripOccurred);
    EXPECT_GT(result.tripTime, 0.1);
    EXPECT_LT(result.tripTime, 0.5);
}

TEST_F(BreakerModelTest, SimulationInvalidParametersThrows)
{
    EXPECT_THROW(
        breaker->runSimulation(-400.0, 100.0, 0.0, 0.1, 0.5, 0.01),
        std::invalid_argument
    );

    EXPECT_THROW(
        breaker->runSimulation(400.0, -100.0, 0.0, 0.1, 0.5, 0.01),
        std::invalid_argument
    );

    EXPECT_THROW(
        breaker->runSimulation(400.0, 100.0, 0.0, 0.1, -0.5, 0.01),
        std::invalid_argument
    );

    EXPECT_THROW(
        breaker->runSimulation(400.0, 100.0, 0.0, 0.1, 0.5, -0.01),
        std::invalid_argument
    );
}

TEST_F(BreakerModelTest, SetDefinitionValid)
{
    BreakerDefinition newDef;
    newDef.maxCurrentA = 3000.0;
    newDef.voltageRatingV = 1000.0;

    EXPECT_NO_THROW(breaker->setDefinition(newDef));

    const auto retrievedDef = breaker->getDefinition();
    EXPECT_DOUBLE_EQ(retrievedDef.maxCurrentA, 3000.0);
    EXPECT_DOUBLE_EQ(retrievedDef.voltageRatingV, 1000.0);
}

TEST_F(BreakerModelTest, SetDefinitionInvalidThrows)
{
    BreakerDefinition invalidDef;
    invalidDef.maxCurrentA = -1000.0;

    EXPECT_THROW(breaker->setDefinition(invalidDef), std::invalid_argument);
}

TEST_F(BreakerModelTest, BreakerDefinitionValidation)
{
    BreakerDefinition validDef;
    EXPECT_TRUE(validDef.isValid());

    BreakerDefinition invalidDef1;
    invalidDef1.openTimeSec = -0.05;
    EXPECT_FALSE(invalidDef1.isValid());

    BreakerDefinition invalidDef2;
    invalidDef2.maxCurrentA = -100.0;
    EXPECT_FALSE(invalidDef2.isValid());

    BreakerDefinition invalidDef3;
    invalidDef3.contactGapMm = 0.0;
    EXPECT_FALSE(invalidDef3.isValid());
}

TEST_F(BreakerModelTest, ConcurrentStateAccess)
{
    std::atomic<int> readCount{0};
    constexpr int iterations = 100;

    auto reader = [&]()
    {
        for (int i = 0; i < iterations; ++i)
        {
            [[maybe_unused]] const auto state = breaker->getState();
            [[maybe_unused]] const auto current = breaker->getCurrent();
            [[maybe_unused]] const auto resistance = breaker->getResistance();
            ++readCount;
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    };

    auto writer = [&]()
    {
        for (int i = 0; i < iterations / 10; ++i)
        {
            if (i % 2 == 0)
            {
                breaker->close();
            }
            else
            {
                breaker->open();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    };

    std::thread t1(reader);
    std::thread t2(reader);
    std::thread t3(writer);

    t1.join();
    t2.join();
    t3.join();

    EXPECT_EQ(readCount, iterations * 2);
}
