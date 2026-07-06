#include "simulation.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <cctype>
#include <ctime>

namespace fcempire {

double clamp(double v, double lo, double hi) { return std::max(lo, std::min(hi, v)); }

static std::string jsonEscape(const std::string& in) {
    std::string out; out.reserve(in.size() + 8);
    for (char c : in) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) out += ' ';
                else out += c;
        }
    }
    return out;
}

static std::string extractString(const std::string& s, const std::string& key, const std::string& def = "") {
    const std::string needle = "\"" + key + "\"";
    size_t p = s.find(needle);
    if (p == std::string::npos) return def;
    p = s.find(':', p);
    if (p == std::string::npos) return def;
    p++;
    while (p < s.size() && std::isspace(static_cast<unsigned char>(s[p]))) p++;
    if (p >= s.size() || s[p] != '"') return def;
    p++;
    std::string out;
    bool esc = false;
    for (; p < s.size(); p++) {
        char c = s[p];
        if (esc) { out += c; esc = false; continue; }
        if (c == '\\') { esc = true; continue; }
        if (c == '"') break;
        out += c;
    }
    return out.empty() ? def : out;
}

static double extractNumber(const std::string& s, const std::string& key, double def = 0.0) {
    const std::string needle = "\"" + key + "\"";
    size_t p = s.find(needle);
    if (p == std::string::npos) return def;
    p = s.find(':', p);
    if (p == std::string::npos) return def;
    p++;
    while (p < s.size() && (std::isspace(static_cast<unsigned char>(s[p])) || s[p] == '"')) p++;
    size_t e = p;
    while (e < s.size() && (std::isdigit(static_cast<unsigned char>(s[e])) || s[e] == '-' || s[e] == '+' || s[e] == '.')) e++;
    if (e <= p) return def;
    try { return std::stod(s.substr(p, e - p)); } catch (...) { return def; }
}

static std::string extractObject(const std::string& s, const std::string& key) {
    const std::string needle = "\"" + key + "\"";
    size_t p = s.find(needle);
    if (p == std::string::npos) return "";
    p = s.find('{', p);
    if (p == std::string::npos) return "";
    int depth = 0; bool str = false, esc = false;
    for (size_t i = p; i < s.size(); i++) {
        char c = s[i];
        if (str) {
            if (esc) esc = false;
            else if (c == '\\') esc = true;
            else if (c == '"') str = false;
            continue;
        }
        if (c == '"') { str = true; continue; }
        if (c == '{') depth++;
        if (c == '}') {
            depth--;
            if (depth == 0) return s.substr(p, i - p + 1);
        }
    }
    return "";
}

static std::vector<std::string> extractObjectsFromArray(const std::string& s, const std::string& key) {
    std::vector<std::string> out;
    const std::string needle = "\"" + key + "\"";
    size_t p = s.find(needle);
    if (p == std::string::npos) return out;
    p = s.find('[', p);
    if (p == std::string::npos) return out;
    bool str = false, esc = false; int depth = 0; size_t start = std::string::npos;
    for (size_t i = p + 1; i < s.size(); i++) {
        char c = s[i];
        if (str) {
            if (esc) esc = false;
            else if (c == '\\') esc = true;
            else if (c == '"') str = false;
            continue;
        }
        if (c == '"') { str = true; continue; }
        if (c == '{') { if (depth == 0) start = i; depth++; }
        else if (c == '}') { depth--; if (depth == 0 && start != std::string::npos) out.push_back(s.substr(start, i - start + 1)); }
        else if (c == ']' && depth == 0) break;
    }
    return out;
}

Simulation::Simulation() : rng(static_cast<unsigned int>(std::time(nullptr))) {
    teams[0].side = 0; teams[0].id = "home"; teams[0].name = "Domicile";
    teams[1].side = 1; teams[1].id = "away"; teams[1].name = "Extérieur";
}

void Simulation::load(const std::string& matchJson) {
    minute = 0; speed = 1; paused = false; over = false; tickCount = 0;
    players.clear(); events.clear(); lastEvent = MatchEvent{};
    const std::string homeObj = extractObject(matchJson, "home");
    const std::string awayObj = extractObject(matchJson, "away");
    teams[0].id = extractString(homeObj, "id", "home");
    teams[0].name = extractString(homeObj, "name", "Domicile");
    teams[1].id = extractString(awayObj, "id", "away");
    teams[1].name = extractString(awayObj, "name", "Extérieur");
    teams[0].goals = teams[1].goals = 0;
    teams[0].shots = teams[1].shots = teams[0].onTarget = teams[1].onTarget = 0;
    teams[0].possessionTicks = teams[1].possessionTicks = 0;
    teams[0].players.clear(); teams[1].players.clear();
    teams[0].tactics = Tactics::fromText(homeObj + extractObject(matchJson, "tactics"));
    teams[1].tactics = Tactics::fromText(awayObj + extractObject(matchJson, "tactics"));

    const auto objs = extractObjectsFromArray(matchJson, "players");
    int slotHome = 0, slotAway = 0;
    for (const std::string& o : objs) {
        Player p;
        p.id = extractString(o, "id", std::to_string(players.size() + 1));
        p.name = extractString(o, "name", "Joueur");
        p.role = extractString(o, "role", extractString(o, "pos", "CM"));
        std::string team = extractString(o, "team", "home");
        p.team = (team == "away" || team == teams[1].id) ? 1 : 0;
        p.slot = p.team == 0 ? slotHome++ : slotAway++;
        p.x = extractNumber(o, "x", p.team == 0 ? 34 : 66);
        p.y = extractNumber(o, "y", 50);
        p.baseX = p.x; p.baseY = p.y; p.tx = p.x; p.ty = p.y;
        p.stamina = extractNumber(o, "stamina", 100);
        p.pace = extractNumber(o, "pace", 68);
        p.pass = extractNumber(o, "pass", extractNumber(o, "pas", 68));
        p.shooting = extractNumber(o, "shooting", extractNumber(o, "sho", 65));
        p.finishing = extractNumber(o, "finishing", p.shooting);
        p.longshots = extractNumber(o, "longshots", 60);
        p.dribble = extractNumber(o, "dribble", extractNumber(o, "dri", 68));
        p.defense = extractNumber(o, "defense", extractNumber(o, "def", 62));
        p.tackling = extractNumber(o, "tackling", p.defense);
        p.vision = extractNumber(o, "vision", p.pass);
        p.crossing = extractNumber(o, "crossing", p.pass);
        p.heading = extractNumber(o, "heading", 62);
        p.composure = extractNumber(o, "composure", 66);
        p.aggression = extractNumber(o, "aggression", 62);
        p.gk = std::max({extractNumber(o, "gk", 30), extractNumber(o, "gkRef", 30), extractNumber(o, "gkPos", 30)});
        const int idx = static_cast<int>(players.size());
        teams[p.team].players.push_back(idx);
        players.push_back(p);
    }
    if (players.size() < 22) setupFallbackPlayers();
    int ko = kickoffPlayer(0);
    claimBall(ko >= 0 ? ko : 0);
    ball.x = 50; ball.y = 50;
    log("KICKOFF", "Coup d'envoi. Les 22 joueurs sont placés, le ballon part du rond central.", ball.owner);
}

void Simulation::setupFallbackPlayers() {
    players.clear(); teams[0].players.clear(); teams[1].players.clear();
    const char* roles[11] = {"GK","LB","CB","CB","RB","CM","CM","CAM","LW","ST","RW"};
    const double xs[11] = {6,20,20,20,20,42,44,55,67,74,67};
    const double ys[11] = {50,18,39,61,82,34,66,50,22,50,78};
    for (int t=0;t<2;t++) for (int i=0;i<11;i++) {
        Player p; p.id = (t==0?"h":"a") + std::to_string(i+1); p.name = (t==0?"Home ":"Away ") + std::to_string(i+1);
        p.role = roles[i]; p.team=t; p.slot=i; p.x = t==0 ? xs[i] : 100-xs[i]; p.y=ys[i]; p.baseX=p.x; p.baseY=p.y; p.tx=p.x; p.ty=p.y;
        int idx=players.size(); teams[t].players.push_back(idx); players.push_back(p);
    }
}

double Simulation::rnd(double lo, double hi) { std::uniform_real_distribution<double> d(lo, hi); return d(rng); }
double Simulation::dist(double ax, double ay, double bx, double by) const { const double dx=ax-bx, dy=ay-by; return std::sqrt(dx*dx+dy*dy); }

int Simulation::nearestPlayer(double x, double y, int teamFilter, bool skipGk) const {
    int best = -1; double bd = 1e9;
    for (int i=0;i<(int)players.size();i++) {
        const Player& p=players[i]; if (p.off) continue; if (teamFilter>=0 && p.team!=teamFilter) continue; if (skipGk && p.isGoalkeeper()) continue;
        double d=dist(x,y,p.x,p.y); if (d<bd) { bd=d; best=i; }
    }
    return best;
}

int Simulation::kickoffPlayer(int team) const {
    int best = -1;
    for (int idx : teams[team].players) if (!players[idx].off && !players[idx].isGoalkeeper() && players[idx].isMidfielder()) { best=idx; break; }
    if (best < 0) for (int idx : teams[team].players) if (!players[idx].off && !players[idx].isGoalkeeper()) { best=idx; break; }
    return best;
}

void Simulation::claimBall(int playerIndex) {
    for (auto& p: players) p.hasBall = false;
    if (playerIndex < 0 || playerIndex >= (int)players.size()) return;
    ball.state = BallState::Owned; ball.owner = playerIndex; ball.target = -1; ball.lastOwner = playerIndex; ball.age = 0; ball.cooldown = 0.18;
    players[playerIndex].hasBall = true; players[playerIndex].lastTouchTick = tickCount;
    ball.x = players[playerIndex].x; ball.y = players[playerIndex].y; ball.vx = ball.vy = 0;
}

void Simulation::log(const std::string& type, const std::string& text, int playerIndex) {
    MatchEvent e; e.minute = minute; e.type = type; e.text = text;
    if (playerIndex >= 0 && playerIndex < (int)players.size()) e.playerId = players[playerIndex].id;
    lastEvent = e; events.push_back(e); if (events.size() > 80) events.erase(events.begin());
}

void Simulation::step(double deltaMinutes) {
    if (paused || over) return;
    double dt = clamp(deltaMinutes, 0.001, 0.25) * speed;
    minute += dt;
    tickCount++;
    if (minute >= 90.0) { minute = 90.0; over = true; paused = true; log("FULL_TIME", "Coup de sifflet final."); return; }
    updateTargets();
    movePlayers(dt);
    updateBall(dt);
    resolvePressure();
    decideWithBall(dt);
}

void Simulation::updateTargets() {
    const int possTeam = (ball.owner >= 0 && ball.owner < (int)players.size()) ? players[ball.owner].team : -1;
    for (int ti=0;ti<2;ti++) {
        Team& T = teams[ti]; Team& O = teams[1-ti];
        double dir = T.attackDir();
        bool hasBall = possTeam == ti;
        int nearest = nearestPlayer(ball.x, ball.y, ti, true);
        for (int idx : T.players) {
            Player& p = players[idx]; if (p.off) continue;
            double baseX = p.baseX, baseY = p.baseY;
            p.action = "shape";
            if (p.isGoalkeeper()) {
                p.tx = clamp(T.ownGoalX() + dir * 5.0, 3, 97);
                p.ty = clamp(50 + (ball.y - 50) * 0.22, 35, 65);
                continue;
            }
            if (hasBall) {
                if (idx == ball.owner) {
                    p.action = "carry";
                    p.tx = clamp(p.x + dir * (3.5 + T.tactics.tempo * 2.2), 2, 98);
                    p.ty = clamp(p.y + (50 - p.y) * 0.04 + rnd(-0.8, 0.8), 4, 96);
                } else {
                    double groupPush = p.isAttacker() ? 21 : p.isMidfielder() ? 11 : 4;
                    if ((p.role.find("LB") != std::string::npos || p.role.find("RB") != std::string::npos) && T.tactics.mentality != "defensive") groupPush += 7;
                    double wide = (baseY - 50) * (T.tactics.width / 70.0);
                    p.action = p.isAttacker() ? "run" : "support";
                    p.tx = clamp(baseX + dir * groupPush * T.tactics.attackBias + (ball.x - baseX) * (p.isDefender() ? 0.10 : 0.25), 2, 98);
                    p.ty = clamp(50 + wide + (ball.y - baseY) * 0.20, 4, 96);
                }
            } else {
                bool pressZone = ti == 0 ? ball.x < T.tactics.defensiveLine + 22 : ball.x > 100 - (T.tactics.defensiveLine + 22);
                if (idx == nearest && pressZone) {
                    p.action = "press"; p.tx = clamp(ball.x - dir * 1.3, 2, 98); p.ty = clamp(ball.y, 4, 96);
                } else {
                    double compactY = baseY + (ball.y - baseY) * 0.48;
                    double lineAnchor = T.ownGoalX() + dir * T.tactics.defensiveLine;
                    p.action = p.isDefender() ? "cover" : "repli";
                    p.tx = clamp(baseX * 0.54 + ball.x * 0.30 + lineAnchor * 0.16, 2, 98);
                    p.ty = clamp(compactY, 4, 96);
                }
            }
        }
    }
}

void Simulation::movePlayers(double dt) {
    for (auto& p : players) {
        if (p.off) continue;
        double dx = p.tx - p.x, dy = p.ty - p.y;
        double d = std::sqrt(dx*dx + dy*dy);
        if (d < 0.001) { p.vx = p.vy = 0; continue; }
        double actionBoost = p.action == "press" ? 1.18 : p.action == "run" ? 1.10 : p.action == "carry" ? 0.92 : 1.0;
        double units = (6.5 + p.pace / 8.0) * p.fatigueMul() * actionBoost * (p.isGoalkeeper() ? 0.44 : 1.0) * dt;
        double step = std::min(d, units);
        p.vx = dx / d * step; p.vy = dy / d * step;
        p.x = clamp(p.x + p.vx, 1.5, 98.5); p.y = clamp(p.y + p.vy, 3.0, 97.0);
        double drain = (p.action == "press" ? 0.10 : p.action == "run" ? 0.075 : 0.045) * dt;
        if (!p.isGoalkeeper()) p.stamina = clamp(p.stamina - drain, 5, 100);
    }
}

void Simulation::updateBall(double dt) {
    if (ball.state == BallState::Owned) {
        if (ball.owner >= 0 && ball.owner < (int)players.size()) { ball.x = players[ball.owner].x; ball.y = players[ball.owner].y; }
        ball.cooldown = std::max(0.0, ball.cooldown - dt);
        if (ball.owner >= 0) teams[players[ball.owner].team].possessionTicks += dt;
        return;
    }
    if (ball.state == BallState::Pass || ball.state == BallState::Shot) {
        ball.age += dt;
        ball.x = clamp(ball.x + ball.vx * dt, 1, 99); ball.y = clamp(ball.y + ball.vy * dt, 3, 97);
        if (ball.state == BallState::Pass && tryInterception(dt)) return;
        if (ball.state == BallState::Pass && ball.target >= 0 && ball.target < (int)players.size()) {
            if (dist(ball.x, ball.y, players[ball.target].x, players[ball.target].y) < 2.2 || ball.age > 0.80) {
                claimBall(ball.target); return;
            }
        }
        if (ball.age > 1.15) {
            ball.state = BallState::Loose; ball.owner = -1; ball.target = -1; ball.vx *= 0.42; ball.vy *= 0.42;
        }
        return;
    }
    if (ball.state == BallState::Loose) {
        ball.x = clamp(ball.x + ball.vx * dt, 1, 99); ball.y = clamp(ball.y + ball.vy * dt, 3, 97);
        ball.vx *= 0.88; ball.vy *= 0.88;
        int n = nearestPlayer(ball.x, ball.y, -1, false);
        if (n >= 0 && dist(ball.x, ball.y, players[n].x, players[n].y) < 2.6) claimBall(n);
    }
}

bool Simulation::tryInterception(double) {
    if (ball.lastOwner < 0 || ball.lastOwner >= (int)players.size()) return false;
    int atkTeam = players[ball.lastOwner].team;
    int defTeam = 1 - atkTeam;
    int n = nearestPlayer(ball.x, ball.y, defTeam, true);
    if (n < 0) return false;
    Player& d = players[n];
    double radius = 1.35 + d.tackling / 95.0 + d.pace / 180.0;
    if (dist(ball.x, ball.y, d.x, d.y) > radius) return false;
    double p = clamp(0.22 + (d.tackling - ball.quality) / 145.0 + (teams[defTeam].tactics.aggression - 1.0) * 0.10, 0.04, 0.56);
    if (rnd() > p) return false;
    claimBall(n);
    d.rating += 0.04;
    if (rnd() < 0.22) log("INTERCEPTION", "Interception de " + d.name + ".", n);
    return true;
}

void Simulation::resolvePressure() {
    if (ball.state != BallState::Owned || ball.owner < 0) return;
    Player& h = players[ball.owner]; if (h.isGoalkeeper() || ball.cooldown > 0) return;
    int n = nearestPlayer(h.x, h.y, 1 - h.team, true);
    if (n < 0 || dist(h.x,h.y,players[n].x,players[n].y) > 1.75) return;
    Player& d = players[n];
    double pWin = clamp(0.13 + (d.tackling * teams[d.team].tactics.aggression - (h.dribble + h.composure) / 2.0) / 170.0, 0.04, 0.42);
    if (rnd() < pWin) {
        d.rating += 0.04; h.rating -= 0.03; claimBall(n);
        if (rnd() < 0.18) log("TACKLE", "Tacle réussi de " + d.name + " sur " + h.name + ".", n);
    }
}

int Simulation::choosePassTarget(int holderIndex, bool risky) const {
    const Player& h = players[holderIndex]; const Team& T = teams[h.team];
    int best = -1; double bestScore = -1e9;
    for (int idx : T.players) {
        if (idx == holderIndex || players[idx].off || players[idx].isGoalkeeper()) continue;
        const Player& p = players[idx];
        double forward = (p.x - h.x) * T.attackDir();
        double d = dist(h.x, h.y, p.x, p.y);
        if (d < 3 || d > (risky ? 45 : 31)) continue;
        double score = forward * (risky ? 1.8 : 0.9) - d * 0.20 + p.vision * 0.03 + p.pace * 0.02;
        if (p.isAttacker()) score += 3.0;
        if (!risky && forward < -10) score += 4.0;
        if (score > bestScore) { bestScore = score; best = idx; }
    }
    return best;
}

void Simulation::decideWithBall(double dt) {
    if (ball.state != BallState::Owned || ball.owner < 0 || ball.cooldown > 0) return;
    Player& h = players[ball.owner]; Team& T = teams[h.team];
    double dg = std::abs(T.attackingGoalX() - h.x);
    bool central = std::abs(h.y - 50) < 24;
    bool finalThird = h.team == 0 ? h.x > 68 : h.x < 32;
    bool inBox = dg < 19 && central;
    if (inBox && rnd() < (0.30 + h.finishing / 260.0)) { shoot(ball.owner, false); return; }
    if (dg < 31 && central && rnd() < (0.06 + h.longshots / 760.0)) { shoot(ball.owner, true); return; }
    double pressure = 0.0; int near = nearestPlayer(h.x,h.y,1-h.team,true); if (near >= 0) pressure = std::max(0.0, 8.0 - dist(h.x,h.y,players[near].x,players[near].y));
    bool risky = finalThird || T.tactics.passStyle == "direct" || (rnd() < T.tactics.passingRisk);
    double passChance = clamp(0.34 + pressure * 0.06 + T.tactics.passingRisk * 0.18 + (h.pass + h.vision - 130) / 420.0, 0.18, 0.75);
    if (rnd() < passChance) {
        int tgt = choosePassTarget(ball.owner, risky);
        if (tgt >= 0) { passBall(ball.owner, tgt, risky && rnd() < 0.58); return; }
    }
    // Dribble: le porteur continue. On log rarement pour éviter un journal qui aboie à chaque pixel.
    if (rnd() < 0.025 + h.dribble / 2600.0) log("DRIBBLE", h.name + " porte le ballon entre les lignes.", ball.owner);
}

void Simulation::passBall(int holderIndex, int targetIndex, bool throughBall) {
    Player& h = players[holderIndex]; Player& t = players[targetIndex]; Team& T = teams[h.team];
    double lead = throughBall ? (6.0 + t.pace / 18.0) * T.attackDir() : 1.8 * T.attackDir();
    double tx = clamp(t.x + lead, 2, 98), ty = clamp(t.y + t.vy * 0.7, 4, 96);
    double dx = tx - h.x, dy = ty - h.y; double d = std::sqrt(dx*dx + dy*dy); if (d < 0.01) return;
    ball.state = BallState::Pass; ball.owner = -1; ball.lastOwner = holderIndex; ball.target = targetIndex; ball.age = 0;
    ball.x = h.x; ball.y = h.y; ball.quality = (h.pass * 0.68 + h.vision * 0.32) * h.fatigueMul() - (throughBall ? 5 : 0);
    double vel = (36.0 + ball.quality * 0.28 + T.tactics.tempo * 4.0);
    ball.vx = dx / d * vel; ball.vy = dy / d * vel;
    for (auto& p: players) p.hasBall = false;
    h.rating += 0.006; h.lastTouchTick = tickCount;
    log(throughBall ? "THROUGH_PASS" : "PASS", (throughBall ? "Passe en profondeur de " : "Passe de ") + h.name + " vers " + t.name + ".", holderIndex);
}

void Simulation::shoot(int holderIndex, bool longShot) {
    Player& h = players[holderIndex]; Team& T = teams[h.team]; Team& D = teams[1-h.team];
    T.shots++;
    int gkIdx = -1; for (int idx : D.players) if (players[idx].isGoalkeeper() && !players[idx].off) { gkIdx=idx; break; }
    Player* gk = gkIdx >= 0 ? &players[gkIdx] : nullptr;
    double dg = std::abs(T.attackingGoalX() - h.x);
    double angle = std::abs(h.y - 50) / 85.0;
    double gkQ = gk ? gk->gk * gk->fatigueMul() : 50;
    double finish = longShot ? h.longshots : h.finishing;
    double pGoal = clamp((longShot ? 0.055 : 0.20) + (finish - 68) / 245.0 + (22 - dg) / 135.0 - angle - (gkQ - 70) / 420.0 + (T.tactics.attackBias - 1) * 0.13, 0.015, 0.56) * h.fatigueMul();
    for (auto& p: players) p.hasBall = false;
    ball.state = BallState::Shot; ball.owner = -1; ball.lastOwner = holderIndex; ball.target = -1; ball.age = 0;
    double gx = T.attackingGoalX(), gy = clamp(50 + rnd(-8, 8), 38, 62);
    double dx = gx - h.x, dy = gy - h.y; double d = std::sqrt(dx*dx+dy*dy); if (d < 0.01) d=1;
    ball.x = h.x; ball.y = h.y; ball.vx = dx/d*68; ball.vy = dy/d*68;
    log("SHOT", "Tir de " + h.name + (longShot ? " de loin." : "."), holderIndex);
    if (rnd() < pGoal) {
        T.goals++; T.onTarget++; h.goals++; h.rating += 0.9;
        int assist = -1; int bestTouch=-1; for (int idx : T.players) if (idx != holderIndex && players[idx].lastTouchTick > bestTouch) { bestTouch=players[idx].lastTouchTick; assist=idx; }
        if (assist >= 0 && tickCount - players[assist].lastTouchTick < 18) { players[assist].assists++; players[assist].rating += 0.45; }
        log("GOAL", "BUUUT ! " + h.name + " marque pour " + T.name + ".", holderIndex);
        resetKickoff(1 - h.team);
    } else {
        bool saved = rnd() < clamp(0.54 + (gkQ - 70) / 270.0, 0.34, 0.78);
        if (saved) { T.onTarget++; if (gk) gk->rating += 0.08; log("SAVE", (gk ? gk->name : std::string("Le gardien")) + " détourne la frappe de " + h.name + ".", gkIdx); }
        else { log("MISS", h.name + " ne cadre pas sa frappe.", holderIndex); }
        int keeper = gkIdx >= 0 ? gkIdx : kickoffPlayer(1 - h.team);
        claimBall(keeper);
    }
}

void Simulation::resetKickoff(int concedingTeam) {
    for (auto& p : players) {
        p.x = p.baseX; p.y = p.baseY; p.tx = p.x; p.ty = p.y; p.vx = p.vy = 0; p.hasBall = false;
    }
    int ko = kickoffPlayer(concedingTeam);
    if (ko >= 0) { players[ko].x = 50; players[ko].y = 50; claimBall(ko); ball.cooldown = 0.42; }
}

void Simulation::updateTactics(const std::string& teamId, const std::string& tacticsJson) {
    for (int i=0;i<2;i++) {
        if (teams[i].id == teamId || (teamId == "home" && i==0) || (teamId == "away" && i==1)) {
            teams[i].tactics = Tactics::fromText(tacticsJson);
            log("TACTIC", teams[i].name + " change ses consignes en direct.");
        }
    }
}

std::string Simulation::eventJson(const MatchEvent& e) const {
    std::ostringstream os; os << "{\"type\":\"" << jsonEscape(e.type) << "\",\"text\":\"" << jsonEscape(e.text) << "\",\"time\":" << std::fixed << std::setprecision(2) << e.minute << ",\"playerId\":\"" << jsonEscape(e.playerId) << "\"}";
    return os.str();
}

std::string Simulation::toJson() const {
    std::ostringstream os;
    double possTot = teams[0].possessionTicks + teams[1].possessionTicks;
    int possH = possTot > 0 ? (int)std::round(teams[0].possessionTicks / possTot * 100.0) : 50;
    int possA = 100 - possH;
    os << std::fixed << std::setprecision(2);
    os << "{\"time\":" << minute << ",";
    os << "\"score\":{\"home\":" << teams[0].goals << ",\"away\":" << teams[1].goals << "},";
    os << "\"ball\":{\"x\":" << ball.x << ",\"y\":" << ball.y << ",\"ownerPlayerId\":\"";
    if (ball.owner >= 0 && ball.owner < (int)players.size()) os << jsonEscape(players[ball.owner].id);
    os << "\",\"state\":\"" << (ball.state == BallState::Owned ? "owned" : ball.state == BallState::Pass ? "pass" : ball.state == BallState::Shot ? "shot" : "loose") << "\"},";
    os << "\"players\":[";
    bool first = true;
    for (const auto& p : players) {
        if (p.off) continue;
        if (!first) os << ','; first = false;
        os << "{\"id\":\"" << jsonEscape(p.id) << "\",\"team\":\"" << (p.team == 0 ? "home" : "away") << "\",\"name\":\"" << jsonEscape(p.name) << "\",\"x\":" << p.x << ",\"y\":" << p.y << ",\"stamina\":" << p.stamina << ",\"hasBall\":" << (p.hasBall ? "true" : "false") << ",\"role\":\"" << jsonEscape(p.role) << "\",\"action\":\"" << jsonEscape(p.action) << "\",\"rating\":" << p.rating << ",\"goals\":" << p.goals << ",\"assists\":" << p.assists << "}";
    }
    os << "],";
    os << "\"event\":" << eventJson(lastEvent) << ",";
    os << "\"events\":[";
    for (size_t i=0;i<events.size();i++) { if (i) os << ','; os << eventJson(events[i]); }
    os << "],";
    os << "\"possession\":{\"home\":" << possH << ",\"away\":" << possA << "},";
    os << "\"status\":{\"paused\":" << (paused ? "true" : "false") << ",\"over\":" << (over ? "true" : "false") << ",\"speed\":" << speed << "},";
    os << "\"stats\":{\"homeShots\":" << teams[0].shots << ",\"awayShots\":" << teams[1].shots << ",\"homeOnTarget\":" << teams[0].onTarget << ",\"awayOnTarget\":" << teams[1].onTarget << "}";
    os << "}";
    return os.str();
}

}
