#pragma once

#include "GameTypes.h"
#include "ItemCatalog.h"

#include <QColor>
#include <QPainter>
#include <QString>

#include <random>

class ScoreSystem {
public:
    void reset();
    void addPoint(int winner);

    bool isMatchOver() const;
    int matchWinner() const;
    int gamesFor(int playerId) const;
    QString pointTextFor(int playerId) const;
    QString scoreNoteText() const;
    QString pointText() const;
    QString gamesText() const;
    QString matchSummary() const;

private:
    void winGame(int playerId);
    static QString pointName(int point);

    int p1Point_ = 0;
    int p2Point_ = 0;
    int advantage_ = 0;
    int p1Games_ = 0;
    int p2Games_ = 0;
    int matchWinner_ = 0;
};

struct Player {
    int id = 1;
    Gender gender = Gender::Male;
    QString name;
    Vec2 pos;
    OutfitItem outfit;
    RacketItem racket;
    bool hitOpportunityActive = false;
    double hitOpportunityStart = 0.0;
    double hitPoseTimer = 0.0;
};

struct Ball {
    Vec3 pos;
    Vec3 vel;
    double previousY = 0.0;
    bool active = false;
    int lastHitPlayer = 0;
    int expectedReceiver = 0;
    int bounceCount = 0;
};

class MatchController {
public:
    MatchController();

    void reset(const OutfitItem& p1Outfit, const RacketItem& p1Racket,
               const OutfitItem& p2Outfit, const RacketItem& p2Racket,
               PlayMode playMode = PlayMode::DoublePlayer,
               AiDifficulty aiDifficulty = AiDifficulty::Medium);
    void update(const InputState& input, double dt);
    void draw(QPainter& painter, const QSize& size) const;

    bool isMatchOver() const;
    int matchWinner() const;
    QString matchSummary() const;

private:
    enum class ServeFaultPlan {
        None,
        Net,
        Out
    };

    QPointF courtToScreen(double x, double y, double z, const QRectF& courtRect) const;
    QRectF courtRectFor(const QSize& size) const;

    void updatePlayers(const InputState& input, double dt);
    void updateBall(double dt);
    void updateHitOpportunities();
    void handleHitInput(const InputState& input);
    void applyAiMovement(InputState& input, double dt);
    void updateAiHitDecision(InputState& input);

    void resetForNextPoint();
    void startServe(int serverId);
    void tryServe(int playerId);
    void tryHit(Player& player, const InputState& input);
    bool canHit(const Player& player) const;
    double hitProbability(double dt) const;
    Vec3 chooseTarget(const Player& player, const InputState& input);
    Vec3 chooseAiTarget(const Player& player);
    Vec3 chooseServeTarget(int serverId);
    ServeFaultPlan chooseServeFaultPlan(int serverId);
    void launchBallTo(const Vec3& target, int hitterId, double flightTime);
    Vec2 predictBallLanding() const;
    double aiHitBonus() const;

    bool isLegalServeBounce(const Vec3& pos) const;
    void registerServeFault(const QString& reason);
    void handleBounce();
    void endPoint(int winner, const QString& reason);

    void drawCourt(QPainter& painter, const QRectF& courtRect) const;
    void drawPlayer(QPainter& painter, const Player& player, const QRectF& courtRect) const;
    void drawRacket(QPainter& painter, const Player& player, const QPointF& grip,
                    double racketDir, double scale) const;
    void drawBall(QPainter& painter, const QRectF& courtRect) const;
    void drawOverlay(QPainter& painter, const QSize& size) const;

    Player& playerById(int id);
    const Player& playerById(int id) const;
    double randomReal(double minValue, double maxValue);

    static constexpr double CourtHalfWidth = 4.115;
    static constexpr double CourtHalfLength = 11.885;
    static constexpr double NetHeight = 0.914;
    static constexpr double Gravity = 9.8;
    static constexpr double PlayerSpeed = 5.25;
    static constexpr double HitRadius = 1.45;
    static constexpr double MinHitHeight = 0.05;
    static constexpr double MaxHitHeight = 2.35;

    Player p1_;
    Player p2_;
    Ball ball_;
    ScoreSystem score_;
    MatchPhase phase_ = MatchPhase::ServeReady;
    PlayMode playMode_ = PlayMode::DoublePlayer;
    AiDifficulty aiDifficulty_ = AiDifficulty::Medium;
    int server_ = 1;
    int serveFaults_ = 0;
    double time_ = 0.0;
    double pointOverTimer_ = 0.0;
    double aiServeTimer_ = 0.0;
    bool serveInFlight_ = false;
    bool aiOpportunityWasActive_ = false;
    ServeFaultPlan plannedServeFault_ = ServeFaultPlan::None;
    double aiReactionDelay_ = 0.35;
    Vec2 aiMoveTarget_;
    int aiPlannedLastHitPlayer_ = 0;
    int aiPlannedBounceCount_ = -1;
    QString feedback_;
    double feedbackTimer_ = 0.0;
    int courtTheme_ = 0;
    mutable QRectF lastCourtRect_;
    std::mt19937 rng_;
};
