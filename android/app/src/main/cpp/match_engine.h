#pragma once
#include <string>
#include "simulation.h"

namespace fcempire {

class MatchEngine {
public:
    MatchEngine();
    std::string startMatch(const std::string& matchJson);
    std::string tick(double deltaMinutes);
    std::string pause();
    std::string resume();
    std::string setSpeed(double speed);
    std::string updateTactics(const std::string& teamId, const std::string& tacticsJson);
    std::string getMatchState() const;
    std::string stopMatch();
private:
    Simulation sim;
};

}
