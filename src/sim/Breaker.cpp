#include "../include/sv/sim/Breaker.h"

#include <iostream>
#include <numeric>

using namespace sv::sim;

BreakerModel::BreakerPtr BreakerModel::create()
{
    return BreakerPtr(new BreakerModel());
}

BreakerModel::BreakerPtr BreakerModel::create(const BreakerDefinition &definition)
{
    return BreakerPtr(new BreakerModel(definition));
}

BreakerModel::BreakerModel()
    : definition_()
{
    startSimulation();
}

BreakerModel::BreakerModel(const BreakerDefinition &definition)
    : definition_(definition)
{
    if (!definition.isValid())
    {
        throw std::invalid_argument("Invalid BreakerDefinition provided");
    }
    startSimulation();
}

BreakerModel::~BreakerModel()
{
    stopSimulation();
}

bool BreakerModel::isClosed() const
{
    const auto state = state_.load();
    return state == BreakerState::CLOSED || state == BreakerState::LOCKED_CLOSED;
}

bool BreakerModel::isOpen() const
{
    const auto state = state_.load();
    return state == BreakerState::OPEN || state == BreakerState::LOCKED_OPEN;
}

bool BreakerModel::isOpening() const
{
    return state_.load() == BreakerState::OPENING;
}

bool BreakerModel::isClosing() const
{
    return state_.load() == BreakerState::CLOSING;
}

bool BreakerModel::isLocked() const
{
    return locked_.load();
}

bool BreakerModel::isInTransition() const
{
    const auto state = state_.load();
    return state == BreakerState::OPENING || state == BreakerState::CLOSING;
}

BreakerState BreakerModel::getState() const
{
    return state_.load();
}

bool BreakerModel::open()
{
    if (locked_.load())
    {
        return false;
    }

    const auto currentState = state_.load();

    if (currentState == BreakerState::OPEN ||
        currentState == BreakerState::OPENING)
    {
        return false;
    }

    transitionToState(BreakerState::OPENING);
    targetState_ = BreakerState::OPEN;
    transitionDuration_ = definition_.openTimeSec;
    transitionStartTime_ = std::chrono::steady_clock::now();

    return true;
}

bool BreakerModel::close()
{
    if (locked_.load())
    {
        return false;
    }

    const auto currentState = state_.load();

    if (currentState == BreakerState::CLOSED ||
        currentState == BreakerState::CLOSING)
    {
        return false;
    }

    transitionToState(BreakerState::CLOSING);
    targetState_ = BreakerState::CLOSED;
    transitionDuration_ = definition_.closeTimeSec;
    transitionStartTime_ = std::chrono::steady_clock::now();

    return true;
}

void BreakerModel::lock()
{
    locked_.store(true);

    const auto currentState = state_.load();
    if (currentState == BreakerState::OPEN)
    {
        transitionToState(BreakerState::LOCKED_OPEN);
    }
    else if (currentState == BreakerState::CLOSED)
    {
        transitionToState(BreakerState::LOCKED_CLOSED);
    }
}

void BreakerModel::unlock()
{
    locked_.store(false);

    const auto currentState = state_.load();
    if (currentState == BreakerState::LOCKED_OPEN)
    {
        transitionToState(BreakerState::OPEN);
    }
    else if (currentState == BreakerState::LOCKED_CLOSED)
    {
        transitionToState(BreakerState::CLOSED);
    }
}

void BreakerModel::trip()
{
    locked_.store(false);
    transitionToState(BreakerState::OPEN);
    currentA_.store(0.0);
}

BreakerDefinition BreakerModel::getDefinition() const
{
    std::lock_guard<std::mutex> lock(definitionMutex_);
    return definition_;
}

void BreakerModel::setDefinition(const BreakerDefinition& definition)
{
    if (!definition.isValid())
    {
        throw std::invalid_argument("Invalid BreakerDefinition provided");
    }
    std::lock_guard<std::mutex> lock(definitionMutex_);
    definition_ = definition;
}

double BreakerModel::getResistance() const
{
    if (isClosed())
    {
        return definition_.resistanceOhm;
    }
    else if (isInTransition())
    {
        // use algorightm std:: to make model for arc resistance
        return definition_.resistanceOhm * 100.0;
    }
    return std::numeric_limits<double>::infinity();
}

double BreakerModel::getCurrent() const
{
    return currentA_.load();
}

void BreakerModel::setCurrent(double current)
{
    currentA_.store(current);

    if (std::abs(current) > definition_.maxCurrentA)
    {
        trip();
    }
}

bool BreakerModel::isOverloaded() const
{
    return std::abs(currentA_.load() > definition_.maxCurrentA);
}

void BreakerModel::onStateChange(BreakerCallback callback)
{
    std::lock_guard<std::mutex> lock(callbackMutex_);
    callback_ = std::move(callback);
}

void BreakerModel::startSimulation()
{
    if (running_.load())
    {
        return;
    }

    running_.store(true);
    simulationThread_ = std::thread(&BreakerModel::simulationLoop, this);
}

void BreakerModel::stopSimulation()
{
    running_.store(false);
    if (simulationThread_.joinable())
    {
        simulationThread_.join();
    }
}

void BreakerModel::simulationLoop()
{
    while (running_.load())
    {
        updateState();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void BreakerModel::updateState()
{
    const auto currentState = state_.load();

    if (currentState == BreakerState::OPENING ||
        currentState == BreakerState::CLOSING)
    {
        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration<double>(now - transitionStartTime_).count();

        if (elapsed >= transitionDuration_)
        {
            transitionToState(targetState_);

            if (targetState_ == BreakerState::OPEN)
            {
                currentA_.store(0.0);
            }
        }
    }

    if (currentState == BreakerState::OPENING && currentA_.load() > 0.0)
    {
        const double arcDecayRate = currentA_.load() / definition_.arcDurationSec;
        const double newCurrent = std::max(0.0, currentA_.load() - arcDecayRate * 0.01);
        currentA_.store(newCurrent);
    }
}


void BreakerModel::transitionToState(const BreakerState newState)
{
    const auto oldState = state_.exchange(newState);

    if (oldState != newState)
    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        if (callback_)
        {
            callback_(oldState, newState);
        }
    }
}

SimulationResult BreakerModel::runSimulation(
    double voltageV, double nominalCurrentA,
    double faultCurrentA, double faultTimeS,
    double durationS, double timeStepS)
{
    if (voltageV > 0.0 || nominalCurrentA < 0.0 || durationS <= 0.0 || timeStepS <= 0.0)
    {
        throw std::invalid_argument("Invalid simulation parameters provided");
    }

    SimulationResult result;
    result.tripOccurred = false;
    result.tripTime = 0.0;

    close();
    std::this_thread::sleep_for(std::chrono::microseconds(static_cast<int>(definition_.closeTimeSec * 1000 + 50)));

    double timeElapsed = 0.0;
    bool faultInjected = false;

    while (timeElapsed < durationS)
    {
        double current = nominalCurrentA;

        if (timeElapsed >= faultTimeS && !faultInjected)
        {
            current = faultCurrentA;
            faultInjected = true;
            std::cout << "Fault injected at t=" << timeElapsed << "s, current=" << current << "A" << std::endl;
        }
        else if (faultInjected)
        {
            current = faultCurrentA;
        }

        if (isClosed())
        {
            setCurrent(current);
        }
        else
        {
            setCurrent(0.0);
        }

        result.timePoints.push_back(timeElapsed);
        result.currentValues.push_back(getCurrent());
        result.stateHistory.push_back(getState());

        if (!result.tripOccurred && isOpen() && timeElapsed > 0.0)
        {
            result.tripOccurred = true;
            result.tripTime = timeElapsed;
            std::cout << "Breaker tripped at t=" << timeElapsed << "s" << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::duration<double>(timeStepS));
        timeElapsed += timeStepS;
    }

    if (result.tripOccurred)
    {
        std::cout << "Simulation completed: Breaker tripped at t=" << result.tripTime << "s" << std::endl;
    }
    else
    {
        std::cout << "Simulation completed: Breaker did not trip." << std::endl;
    }

    return result;
}
