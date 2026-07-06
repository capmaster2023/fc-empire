#include "player.h"

namespace fcempire {

static bool has(const std::string& s, const char* t) { return s.find(t) != std::string::npos; }

bool Player::isGoalkeeper() const { return has(role, "GK") || has(role, "G"); }
bool Player::isDefender() const { return has(role, "CB") || has(role, "LB") || has(role, "RB") || has(role, "DF"); }
bool Player::isMidfielder() const { return has(role, "CM") || has(role, "DM") || has(role, "AM") || has(role, "MF") || has(role, "CDM") || has(role, "CAM"); }
bool Player::isAttacker() const { return has(role, "ST") || has(role, "CF") || has(role, "LW") || has(role, "RW") || has(role, "AT"); }
double Player::fatigueMul() const { return 0.56 + 0.44 * (stamina / 100.0); }

}
