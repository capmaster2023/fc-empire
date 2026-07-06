#pragma once
#include <string>

namespace fcempire {

struct Tactics {
    std::string mentality = "balanced";
    std::string pressing = "medium";
    std::string passStyle = "mixed";
    double defensiveLine = 50.0;
    double width = 62.0;
    double tempo = 1.0;
    double aggression = 1.0;
    double attackBias = 1.0;
    double passingRisk = 0.50;

    static Tactics fromText(const std::string& text);
};

}
