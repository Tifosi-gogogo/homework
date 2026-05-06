#pragma once

#include <algorithm>
#include <cmath>

enum class Gender {
    Male,
    Female
};

enum class MatchPhase {
    ServeReady,
    Rally,
    PointOver,
    MatchOver
};

enum class PlayMode {
    DoublePlayer,
    SinglePlayer
};

enum class AiDifficulty {
    Easy,
    Medium,
    Hard
};

struct Vec2 {
    double x = 0.0;
    double y = 0.0;
};

struct Vec3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct InputState {
    bool p1Up = false;
    bool p1Down = false;
    bool p1Left = false;
    bool p1Right = false;

    bool p2Up = false;
    bool p2Down = false;
    bool p2Left = false;
    bool p2Right = false;

    bool p1HitPressed = false;
    bool p2HitPressed = false;
    bool pausePressed = false;
};

inline double length2D(const Vec2& a, const Vec3& b) {
    const double dx = a.x - b.x;
    const double dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

inline double clampDouble(double value, double minValue, double maxValue) {
    return std::max(minValue, std::min(value, maxValue));
}

inline int opponentOf(int playerId) {
    return playerId == 1 ? 2 : 1;
}
