#include "../include/sv/visualize/SVVisualizer.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <algorithm>

using namespace sv;

std::unique_ptr<SVVisualizer> SVVisualizer::create(const Mode mode, const std::string &csvFile)
{
    return std::unique_ptr<SVVisualizer>(new SVVisualizer(mode, csvFile));
}

SVVisualizer::~SVVisualizer()
{
    close();
}

void SVVisualizer::update(const ASDU &asdu)
{
    switch (mode_)
    {
        case Mode::RealTime: updateRealTime(asdu); break;
        case Mode::Statistics: updateStatistics(asdu); break;
        case Mode::Table: updateTable(asdu); break;
        case Mode::CSV: updateCSV(asdu); break;
    }

    if (csvFile_.is_open())
    {
        updateCSV(asdu);
    }

    frameCount_++;
    if (frameCount_ > 1)
    {
        const uint16_t expected = (lastSmpCnt_ + 1) & 0xFFFF;
        if (asdu.smpCnt != expected)
        {
            missingFrames_++;
        }
    }
    lastSmpCnt_ = asdu.smpCnt;
}

void SVVisualizer::clear()
{
    std::cout << "\033[2J\033[H";
}

void SVVisualizer::printStatistics() const
{
    const auto now = std::chrono::steady_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime_).count();

    std::cout << "\n|--------------------------------------------------------------------------|\n";
    std::cout << "|                          Statistics Summary                                |\n";
    std::cout << "|----------------------------------------------------------------------------|\n";
    std::cout << "Total Frames:    " << frameCount_ << "\n";
    std::cout << "Missing Frames:  " << missingFrames_ << "\n";
    std::cout << "Duration:        " << duration / 1000.0 << " seconds\n";
    std::cout << "Average Rate:    " << (frameCount_ * 1000.0 / duration) << " frames/sec\n";
    std::cout << "Last smpCnt:     " << lastSmpCnt_ << "\n";
}

void SVVisualizer::close()
{
    if (csvFile_.is_open())
    {
        csvFile_.close();
    }
}

SVVisualizer::SVVisualizer(Mode mode, const std::string &csvFile)
    : mode_(mode)
      , startTime_(std::chrono::steady_clock::now())
{
    if (!csvFile.empty())
    {
        csvFile_.open(csvFile);
        if (csvFile_.is_open())
        {
            csvFile_ << "Timestamp,smpCnt,confRev,smpSynch,"
                    << "Ia,Ib,Ic,In,Va,Vb,Vc,Vn,"
                    << "Ia_quality,Ib_quality,Ic_quality,In_quality,"
                    << "Va_quality,Vb_quality,Vc_quality,Vn_quality\n";
        }
    }

    currentHistory_.reserve(HISTORY_SIZE);
    voltageHistory_.reserve(HISTORY_SIZE);
}

void SVVisualizer::updateRealTime(const ASDU& asdu)
{
    std::cout << "\033[2J\033[H";

    std::cout << "|----------------------------------------------------------------------------|\n";
    std::cout << "|           IEC 61850 Sampled Values - Real-Time Visualization               |\n";
    std::cout << "|____________________________________________________________________________|\n\n";

    std::cout << "SV ID: " << std::setw(20) << std::left << asdu.svID
            << "  Sample Count: " << std::setw(6) << asdu.smpCnt
            << "  Conf Rev: " << asdu.confRev << "\n";
    std::cout << "Sync: " << std::setw(8) << smpSynchToString(asdu.smpSynch)
            << "  Frames: " << frameCount_
            << "  Missing: " << missingFrames_ << "\n\n";

    if (asdu.dataSet.size() != VALUES_PER_ASDU)
    {
        std::cout << "ERROR: Invalid dataset size!\n";
        return;
    }

    const float ia = asdu.dataSet[0].getScaledInt() / static_cast<float>(ScalingFactors::CURRENT_DEFAULT);
    const float ib = asdu.dataSet[1].getScaledInt() / static_cast<float>(ScalingFactors::CURRENT_DEFAULT);
    const float ic = asdu.dataSet[2].getScaledInt() / static_cast<float>(ScalingFactors::CURRENT_DEFAULT);
    const float in = asdu.dataSet[3].getScaledInt() / static_cast<float>(ScalingFactors::CURRENT_DEFAULT);

    const float va = asdu.dataSet[4].getScaledInt() / static_cast<float>(ScalingFactors::VOLTAGE_DEFAULT);
    const float vb = asdu.dataSet[5].getScaledInt() / static_cast<float>(ScalingFactors::VOLTAGE_DEFAULT);
    const float vc = asdu.dataSet[6].getScaledInt() / static_cast<float>(ScalingFactors::VOLTAGE_DEFAULT);
    const float vn = asdu.dataSet[7].getScaledInt() / static_cast<float>(ScalingFactors::VOLTAGE_DEFAULT);

    std::cout << "|- CURRENTS (A) -------------------------------------------------------------|\n";
    std::cout << "| Ia: " << std::setw(8) << std::fixed << std::setprecision(2) << ia
            << " " << createBar(ia, -150.0f, 150.0f, 50)
            << (asdu.dataSet[0].quality.isGood() ? " [GOOD]" : " [BAD]") << "\n";
    std::cout << "| Ib: " << std::setw(8) << ib
            << " " << createBar(ib, -150.0f, 150.0f, 50)
            << (asdu.dataSet[1].quality.isGood() ? " [GOOD]" : " [BAD]") << "\n";
    std::cout << "| Ic: " << std::setw(8) << ic
            << " " << createBar(ic, -150.0f, 150.0f, 50)
            << (asdu.dataSet[2].quality.isGood() ? " [GOOD]" : " [BAD]") << "\n";
    std::cout << "| In: " << std::setw(8) << in
            << " " << createBar(in, -150.0f, 150.0f, 50)
            << (asdu.dataSet[3].quality.isGood() ? " [GOOD]" : " [BAD]") << "\n";
    std::cout << "|____________________________________________________________________________|\n\n";

    std::cout << "|- VOLTAGES (V) -------------------------------------------------------------|\n";
    std::cout << "| Va: " << std::setw(8) << va
            << " " << createBar(va, -400.0f, 400.0f, 50)
            << (asdu.dataSet[4].quality.isGood() ? " [GOOD]" : " [BAD]") << "\n";
    std::cout << "| Vb: " << std::setw(8) << vb
            << " " << createBar(vb, -400.0f, 400.0f, 50)
            << (asdu.dataSet[5].quality.isGood() ? " [GOOD]" : " [BAD]") << "\n";
    std::cout << "| Vc: " << std::setw(8) << vc
            << " " << createBar(vc, -400.0f, 400.0f, 50)
            << (asdu.dataSet[6].quality.isGood() ? " [GOOD]" : " [BAD]") << "\n";
    std::cout << "| Vn: " << std::setw(8) << vn
            << " " << createBar(vn, -400.0f, 400.0f, 50)
            << (asdu.dataSet[7].quality.isGood() ? " [GOOD]" : " [BAD]") << "\n";
    std::cout << "|____________________________________________________________________________|\n\n";

    currentHistory_.push_back(ia);
    voltageHistory_.push_back(va);
    if (currentHistory_.size() > HISTORY_SIZE)
    {
        currentHistory_.erase(currentHistory_.begin());
        voltageHistory_.erase(voltageHistory_.begin());
    }

    if (currentHistory_.size() > 10)
    {
        std::cout << "|- WAVEFORM (Ia - last " << currentHistory_.size() << " samples) --------------------------------------|\n";
        std::cout << "| " << createWaveform(currentHistory_.back(), -150.0f, 150.0f, 70) << " |\n";
        std::cout << "|______________________________________________________________________________________________________|\n";
    }

    std::cout << "\nPress Ctrl+C to stop...\n";
    std::cout.flush();
}

void SVVisualizer::updateStatistics(const ASDU &asdu)
{
    // TODO AdrGos update counters...
}

void SVVisualizer::updateTable(const ASDU &asdu) const
{
    if (frameCount_ == 1)
    {
        std::cout << std::setw(8) << "smpCnt"
                << std::setw(10) << "Ia(A)" << std::setw(10) << "Ib(A)"
                << std::setw(10) << "Ic(A)" << std::setw(10) << "In(A)"
                << std::setw(10) << "Va(V)" << std::setw(10) << "Vb(V)"
                << std::setw(10) << "Vc(V)" << std::setw(10) << "Vn(V)" << "\n";
        std::cout << std::string(88, '-') << "\n";
    }

    std::cout << std::setw(8) << asdu.smpCnt;
    for (size_t i = 0; i < 8 && i < asdu.dataSet.size(); ++i)
    {
        const float val = asdu.dataSet[i].getScaledInt() / (i < 4 ? ScalingFactors::CURRENT_DEFAULT : ScalingFactors::VOLTAGE_DEFAULT);
        std::cout << std::setw(10) << std::fixed << std::setprecision(2) << val;
    }
    std::cout << "\n";
}

void SVVisualizer::updateCSV(const ASDU& asdu)
{
    if (!csvFile_.is_open()) return;

    const auto now = std::chrono::system_clock::now();
    const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();

    csvFile_ << ns << "," << asdu.smpCnt << "," << asdu.confRev << "," << static_cast<int>(asdu.smpSynch);

    for (size_t i = 0; i < 8 && i < asdu.dataSet.size(); ++i)
    {
        const float val = asdu.dataSet[i].getScaledInt() / (i < 4 ? ScalingFactors::CURRENT_DEFAULT : ScalingFactors::VOLTAGE_DEFAULT);
        csvFile_ << "," << val;
    }

    for (size_t i = 0; i < 8 && i < asdu.dataSet.size(); ++i)
    {
        csvFile_ << "," << (asdu.dataSet[i].quality.isGood() ? "1" : "0");
    }

    csvFile_ << "\n";
    csvFile_.flush();
}


std::string SVVisualizer::createWaveform(const float value, const float min, const float max, const int width)
{
    const float normalized = (value - min) / (max - min);
    const int pos = static_cast<int>(normalized * width);

    std::string wave(width, '-');

    if (pos >= 0 && pos < width)
    {
        wave[pos] = '*';
    }

    return wave;
}

std::string SVVisualizer::createBar(const float value, const float min, const float max, const int width)
{
    const float normalized = (value - min) / (max - min);
    const int barWidth = static_cast<int>(normalized * width);
    const int centerPos = static_cast<int>((0.0f - min) / (max - min) * width);

    std::string bar(width, ' ');

    if (barWidth > centerPos)
    {
        std::fill(bar.begin() + centerPos, bar.begin() + std::min(barWidth, width), '|');
    }
    else
    {
        std::fill(bar.begin() + std::max(0, barWidth), bar.begin() + centerPos, '|');
    }

    if (centerPos >= 0 && centerPos < width)
    {
        bar[centerPos] = '|';
    }

    return "[" + bar + "]";
}
