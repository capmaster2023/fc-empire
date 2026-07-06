#pragma once
#include <string>
#include <vector>
#include "tactics.h"

namespace fcempire {

struct Team {
    std::string id;
    std::string name;
    int side = 0; // 0 home attacks right, 1 away attacks left
    int goals = 0;
    int shots = 0;
    int onTarget = 0;
    double possessionTicks = 0;
    std::vector<int> players;
    Tactics tactics;

    double attackDir() const { return side == 0 ? 1.0 : -1.0; }
    double attackingGoalX() const { return side == 0 ? 100.0 : 0.0; }
    double ownGoalX() const { return side == 0 ? 0.0 : 100.0; }
};

}
