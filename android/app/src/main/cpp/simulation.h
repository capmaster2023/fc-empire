#pragma once
#include <string>
#include <vector>
#include <random>
#include "player.h"
#include "team.h"
#include "ball.h"

namespace fcempire {

double clamp(double v, double lo, double hi);

struct MatchEvent {
    double minute = 0;
    std::string type = "INFO";
    std::string text;
    std::string playerId;
};

class Simulation {
public:
    Team teams[2];
    std::vector<Player> players;
    Ball ball;
    double minute = 0.0;
    double speed = 1.0;
    bool paused = false;
    bool over = false;
    int tickCount = 0;
    MatchEvent lastEvent;
    std::vector<MatchEvent> events;

    Simulation();
    void load(const std::string& matchJson);
    void step(double deltaMinutes);
    void updateTactics(const std::string& teamId, const std::string& tacticsJson);
    std::string toJson() const;

private:
    std::mt19937 rng;
    double dist(double ax, double ay, double bx, double by) const;
    double rnd(double lo = 0.0, double hi = 1.0);
    int nearestPlayer(double x, double y, int teamFilter = -1, bool skipGk = false) const;
    int kickoffPlayer(int team) const;
    void claimBall(int playerIndex);
    void log(const std::string& type, const std::string& text, int playerIndex = -1);
    void setupFallbackPlayers();
    void updateTargets();
    void movePlayers(double dt);
    void updateBall(double dt);
    bool tryInterception(double dt);
    void decideWithBall(double dt);
    void passBall(int holderIndex, int targetIndex, bool throughBall);
    void shoot(int holderIndex, bool longShot);
    void resetKickoff(int concedingTeam);
    void resolvePressure();
    int choosePassTarget(int holderIndex, bool risky) const;
    std::string eventJson(const MatchEvent& e) const;
};

}
