#include "tactics.h"
#include <algorithm>

namespace fcempire {

static bool contains(const std::string& s, const std::string& token) {
    return s.find(token) != std::string::npos;
}

Tactics Tactics::fromText(const std::string& text) {
    Tactics t;
    if (contains(text, "offensive") || contains(text, "attack")) {
        t.mentality = "offensive"; t.attackBias = 1.18; t.defensiveLine = 61; t.passingRisk = 0.62;
    } else if (contains(text, "defensive") || contains(text, "bloc_bas")) {
        t.mentality = "defensive"; t.attackBias = 0.86; t.defensiveLine = 36; t.passingRisk = 0.34;
    }
    if (contains(text, "pressing_haut") || contains(text, "high")) {
        t.pressing = "high"; t.defensiveLine = std::max(t.defensiveLine, 66.0); t.aggression = 1.18; t.tempo = 1.16;
    } else if (contains(text, "low") || contains(text, "bas")) {
        t.pressing = "low"; t.defensiveLine = std::min(t.defensiveLine, 34.0); t.aggression = 0.92;
    }
    if (contains(text, "direct")) { t.passStyle = "direct"; t.passingRisk = 0.72; t.tempo = 1.22; }
    if (contains(text, "possession")) { t.passStyle = "short"; t.passingRisk = 0.30; t.tempo = 0.86; }
    if (contains(text, "ailes") || contains(text, "wide")) { t.width = 78.0; }
    return t;
}

}
