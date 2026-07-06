#include "match_engine.h"

namespace fcempire {

MatchEngine::MatchEngine() = default;
std::string MatchEngine::startMatch(const std::string& matchJson) { sim = Simulation(); sim.load(matchJson); return sim.toJson(); }
std::string MatchEngine::tick(double deltaMinutes) { sim.step(deltaMinutes); return sim.toJson(); }
std::string MatchEngine::pause() { sim.paused = true; return sim.toJson(); }
std::string MatchEngine::resume() { sim.paused = false; return sim.toJson(); }
std::string MatchEngine::setSpeed(double speed) { sim.speed = clamp(speed, 0.25, 8.0); return sim.toJson(); }
std::string MatchEngine::updateTactics(const std::string& teamId, const std::string& tacticsJson) { sim.updateTactics(teamId, tacticsJson); return sim.toJson(); }
std::string MatchEngine::getMatchState() const { return sim.toJson(); }
std::string MatchEngine::stopMatch() { sim.over = true; sim.paused = true; return sim.toJson(); }

}
