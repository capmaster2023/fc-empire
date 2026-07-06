#pragma once

namespace fcempire {

enum class BallState { Owned, Pass, Loose, Shot };

struct Ball {
    BallState state = BallState::Owned;
    double x = 50, y = 50;
    double vx = 0, vy = 0;
    int owner = -1;
    int target = -1;
    int lastOwner = -1;
    double quality = 70;
    double age = 0;
    double cooldown = 0.35;
};

}
