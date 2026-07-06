#pragma once
#include <string>

namespace fcempire {

struct Player {
    std::string id;
    std::string name;
    std::string role;
    int team = 0; // 0 home, 1 away
    int slot = 0;
    bool off = false;
    bool hasBall = false;
    double x = 50, y = 50;
    double vx = 0, vy = 0;
    double tx = 50, ty = 50;
    double baseX = 50, baseY = 50;
    double stamina = 100;
    double rating = 6.0;
    int goals = 0;
    int assists = 0;
    int lastTouchTick = 0;
    std::string action = "shape";

    double pace = 68;
    double pass = 68;
    double shooting = 65;
    double finishing = 65;
    double longshots = 60;
    double dribble = 68;
    double defense = 62;
    double tackling = 64;
    double vision = 66;
    double crossing = 62;
    double heading = 62;
    double composure = 66;
    double aggression = 62;
    double gk = 30;

    bool isGoalkeeper() const;
    bool isDefender() const;
    bool isMidfielder() const;
    bool isAttacker() const;
    double fatigueMul() const;
};

}
