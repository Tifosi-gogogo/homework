#include "MatchController.h"

#include <QFont>
#include <QLinearGradient>
#include <QPen>
#include <QStaticText>

#include <algorithm>

void ScoreSystem::reset() {
    p1Point_ = 0;
    p2Point_ = 0;
    advantage_ = 0;
    p1Games_ = 0;
    p2Games_ = 0;
    matchWinner_ = 0;
}

void ScoreSystem::addPoint(int winner) {
    if (matchWinner_ != 0) {
        return;
    }

    if (advantage_ == winner) {
        winGame(winner);
        return;
    }

    if (advantage_ != 0 && advantage_ != winner) {
        advantage_ = 0;
        return;
    }

    int& winnerPoint = winner == 1 ? p1Point_ : p2Point_;
    int& loserPoint = winner == 1 ? p2Point_ : p1Point_;

    if (winnerPoint < 3) {
        ++winnerPoint;
        return;
    }

    if (winnerPoint == 3 && loserPoint < 3) {
        winGame(winner);
        return;
    }

    advantage_ = winner;
}

bool ScoreSystem::isMatchOver() const {
    return matchWinner_ != 0;
}

int ScoreSystem::matchWinner() const {
    return matchWinner_;
}

int ScoreSystem::gamesFor(int playerId) const {
    return playerId == 1 ? p1Games_ : p2Games_;
}

QString ScoreSystem::pointText() const {
    if (advantage_ == 1) {
        return QStringLiteral("P1 Advantage");
    }
    if (advantage_ == 2) {
        return QStringLiteral("P2 Advantage");
    }
    if (p1Point_ == 3 && p2Point_ == 3) {
        return QStringLiteral("Deuce");
    }
    return pointName(p1Point_) + QStringLiteral(" - ") + pointName(p2Point_);
}

QString ScoreSystem::gamesText() const {
    return QStringLiteral("Games  P1 %1 : %2 P2").arg(p1Games_).arg(p2Games_);
}

QString ScoreSystem::matchSummary() const {
    return QStringLiteral("P1 games %1, P2 games %2").arg(p1Games_).arg(p2Games_);
}

void ScoreSystem::winGame(int playerId) {
    if (playerId == 1) {
        ++p1Games_;
    } else {
        ++p2Games_;
    }

    p1Point_ = 0;
    p2Point_ = 0;
    advantage_ = 0;

    if (p1Games_ >= 2) {
        matchWinner_ = 1;
    } else if (p2Games_ >= 2) {
        matchWinner_ = 2;
    }
}

QString ScoreSystem::pointName(int point) {
    switch (point) {
    case 0:
        return QStringLiteral("0");
    case 1:
        return QStringLiteral("15");
    case 2:
        return QStringLiteral("30");
    default:
        return QStringLiteral("40");
    }
}

MatchController::MatchController()
    : rng_(std::random_device{}()) {
    reset(outfitByGenderIndex(Gender::Male, 0), racketByIndex(0),
          outfitByGenderIndex(Gender::Female, 0), racketByIndex(0));
}

void MatchController::reset(const OutfitItem& p1Outfit, const RacketItem& p1Racket,
                            const OutfitItem& p2Outfit, const RacketItem& p2Racket) {
    score_.reset();
    p1_ = Player{
        1,
        p1Outfit.gender,
        QStringLiteral("P1"),
        {0.0, 8.6},
        p1Outfit,
        p1Racket,
        false,
        0.0};
    p2_ = Player{
        2,
        p2Outfit.gender,
        QStringLiteral("P2"),
        {0.0, -8.6},
        p2Outfit,
        p2Racket,
        false,
        0.0};

    server_ = 1;
    phase_ = MatchPhase::ServeReady;
    time_ = 0.0;
    pointOverTimer_ = 0.0;
    feedback_ = QStringLiteral("P1 serve: press Space");
    feedbackTimer_ = 3.0;
    startServe(server_);
}

void MatchController::update(const InputState& input, double dt) {
    dt = clampDouble(dt, 0.0, 0.05);
    time_ += dt;
    feedbackTimer_ = std::max(0.0, feedbackTimer_ - dt);

    updatePlayers(input, dt);

    if (phase_ == MatchPhase::PointOver) {
        pointOverTimer_ -= dt;
        if (pointOverTimer_ <= 0.0) {
            if (score_.isMatchOver()) {
                phase_ = MatchPhase::MatchOver;
            } else {
                resetForNextPoint();
            }
        }
        return;
    }

    if (phase_ == MatchPhase::MatchOver) {
        return;
    }

    if (phase_ == MatchPhase::ServeReady) {
        const Player& serverPlayer = playerById(server_);
        ball_.pos = {serverPlayer.pos.x, serverPlayer.pos.y + (server_ == 1 ? -0.45 : 0.45), 1.05};

        if ((server_ == 1 && input.p1HitPressed) ||
            (server_ == 2 && input.p2HitPressed)) {
            tryServe(server_);
        }
        return;
    }

    updateBall(dt);
    updateHitOpportunities();
    handleHitInput(input);
}

void MatchController::draw(QPainter& painter, const QSize& size) const {
    painter.fillRect(QRect(QPoint(0, 0), size), QColor(22, 40, 48));
    const QRectF courtRect = courtRectFor(size);
    lastCourtRect_ = courtRect;

    drawCourt(painter, courtRect);

    if (p2_.pos.y < p1_.pos.y) {
        drawPlayer(painter, p2_, courtRect);
        drawPlayer(painter, p1_, courtRect);
    } else {
        drawPlayer(painter, p1_, courtRect);
        drawPlayer(painter, p2_, courtRect);
    }
    drawBall(painter, courtRect);
    drawOverlay(painter, size);
}

bool MatchController::isMatchOver() const {
    return phase_ == MatchPhase::MatchOver;
}

int MatchController::matchWinner() const {
    return score_.matchWinner();
}

QString MatchController::matchSummary() const {
    return score_.matchSummary();
}

QRectF MatchController::courtRectFor(const QSize& size) const {
    const double maxWidth = size.width() * 0.58;
    const double maxHeight = size.height() * 0.76;
    const double widthFromHeight = maxHeight * (CourtHalfWidth * 2.0) / (CourtHalfLength * 2.0) * 2.08;
    const double courtWidth = std::min(maxWidth, std::max(460.0, widthFromHeight));
    const double courtHeight = std::min(maxHeight, courtWidth * 1.45);
    const double x = (size.width() - courtWidth) * 0.5;
    const double y = 94.0;
    return QRectF(x, y, courtWidth, courtHeight);
}

QPointF MatchController::courtToScreen(double x, double y, double z, const QRectF& courtRect) const {
    const double sx = courtRect.width() / (CourtHalfWidth * 2.0);
    const double sy = courtRect.height() / (CourtHalfLength * 2.0);
    const double px = courtRect.center().x() + x * sx;
    const double py = courtRect.center().y() + y * sy - z * 26.0;
    return QPointF(px, py);
}

void MatchController::updatePlayers(const InputState& input, double dt) {
    auto movePlayer = [&](Player& player, bool up, bool down, bool left, bool right) {
        double dx = 0.0;
        double dy = 0.0;
        if (left) {
            dx -= 1.0;
        }
        if (right) {
            dx += 1.0;
        }
        if (up) {
            dy -= 1.0;
        }
        if (down) {
            dy += 1.0;
        }

        const double len = std::sqrt(dx * dx + dy * dy);
        if (len > 0.0) {
            dx /= len;
            dy /= len;
        }

        player.pos.x += dx * PlayerSpeed * dt;
        player.pos.y += dy * PlayerSpeed * dt;

        player.pos.x = clampDouble(player.pos.x, -CourtHalfWidth + 0.25, CourtHalfWidth - 0.25);
        if (player.id == 1) {
            player.pos.y = clampDouble(player.pos.y, 0.65, CourtHalfLength - 0.45);
        } else {
            player.pos.y = clampDouble(player.pos.y, -CourtHalfLength + 0.45, -0.65);
        }
    };

    movePlayer(p1_, input.p1Up, input.p1Down, input.p1Left, input.p1Right);
    movePlayer(p2_, input.p2Up, input.p2Down, input.p2Left, input.p2Right);
}

void MatchController::updateBall(double dt) {
    if (!ball_.active) {
        return;
    }

    const Vec3 previous = ball_.pos;
    ball_.previousY = ball_.pos.y;

    ball_.pos.x += ball_.vel.x * dt;
    ball_.pos.y += ball_.vel.y * dt;
    ball_.pos.z += ball_.vel.z * dt;
    ball_.vel.z -= Gravity * dt;

    const bool crossedNet = (previous.y < 0.0 && ball_.pos.y >= 0.0) ||
                            (previous.y > 0.0 && ball_.pos.y <= 0.0);
    if (crossedNet) {
        const double denom = ball_.pos.y - previous.y;
        const double t = std::abs(denom) < 0.0001 ? 0.0 : (0.0 - previous.y) / denom;
        const double zAtNet = previous.z + (ball_.pos.z - previous.z) * t;
        const double xAtNet = previous.x + (ball_.pos.x - previous.x) * t;
        if (std::abs(xAtNet) <= CourtHalfWidth + 0.35 && zAtNet < NetHeight) {
            endPoint(opponentOf(ball_.lastHitPlayer), QStringLiteral("Net"));
            return;
        }
    }

    if (ball_.pos.z <= 0.0 && ball_.vel.z < 0.0) {
        ball_.pos.z = 0.0;
        handleBounce();
    }
}

void MatchController::updateHitOpportunities() {
    for (Player* player : {&p1_, &p2_}) {
        if (canHit(*player)) {
            if (!player->hitOpportunityActive) {
                player->hitOpportunityActive = true;
                player->hitOpportunityStart = time_;
            }
        } else {
            player->hitOpportunityActive = false;
        }
    }
}

void MatchController::handleHitInput(const InputState& input) {
    if (phase_ != MatchPhase::Rally) {
        return;
    }

    if (input.p1HitPressed) {
        tryHit(p1_, input);
    }
    if (input.p2HitPressed) {
        tryHit(p2_, input);
    }
}

void MatchController::resetForNextPoint() {
    server_ = (score_.gamesFor(1) + score_.gamesFor(2)) % 2 == 0 ? 1 : 2;
    p1_.pos = {0.0, 8.6};
    p2_.pos = {0.0, -8.6};
    p1_.hitOpportunityActive = false;
    p2_.hitOpportunityActive = false;
    phase_ = MatchPhase::ServeReady;
    startServe(server_);
}

void MatchController::startServe(int serverId) {
    const Player& serverPlayer = playerById(serverId);
    ball_.active = false;
    ball_.lastHitPlayer = 0;
    ball_.expectedReceiver = 0;
    ball_.bounceCount = 0;
    ball_.vel = {0.0, 0.0, 0.0};
    ball_.pos = {serverPlayer.pos.x, serverPlayer.pos.y + (serverId == 1 ? -0.45 : 0.45), 1.05};
    feedback_ = serverId == 1 ? QStringLiteral("P1 serve: Space")
                              : QStringLiteral("P2 serve: J");
    feedbackTimer_ = 2.5;
}

void MatchController::tryServe(int playerId) {
    if (phase_ != MatchPhase::ServeReady || playerId != server_) {
        return;
    }

    const Vec3 target = chooseServeTarget(playerId);
    launchBallTo(target, playerId, 1.05);
    phase_ = MatchPhase::Rally;
    feedback_ = QStringLiteral("Serve");
    feedbackTimer_ = 0.75;
}

void MatchController::tryHit(Player& player, const InputState& input) {
    if (phase_ != MatchPhase::Rally) {
        return;
    }

    if (!canHit(player)) {
        feedback_ = player.id == 1 ? QStringLiteral("P1: too far") : QStringLiteral("P2: too far");
        feedbackTimer_ = 0.45;
        return;
    }

    const double dt = player.hitOpportunityActive ? time_ - player.hitOpportunityStart : 0.0;
    const double probability = std::min(1.0, hitProbability(dt) + player.racket.hitBonus);
    if (randomReal(0.0, 1.0) > probability) {
        endPoint(opponentOf(player.id), player.id == 1 ? QStringLiteral("P1 miss") : QStringLiteral("P2 miss"));
        return;
    }

    Vec3 target = chooseTarget(player, input);
    const double distance = std::sqrt((target.x - ball_.pos.x) * (target.x - ball_.pos.x) +
                                      (target.y - ball_.pos.y) * (target.y - ball_.pos.y));
    const double flightTime = clampDouble(distance / 10.4 + randomReal(-0.08, 0.16), 0.68, 1.40);
    launchBallTo(target, player.id, flightTime);

    if (dt <= 0.1) {
        feedback_ = QStringLiteral("Perfect");
    } else if (dt <= 0.5) {
        feedback_ = QStringLiteral("Good");
    } else {
        feedback_ = QStringLiteral("Late");
    }
    feedbackTimer_ = 0.75;
}

bool MatchController::canHit(const Player& player) const {
    if (phase_ != MatchPhase::Rally || !ball_.active || ball_.expectedReceiver != player.id) {
        return false;
    }
    const double dist = length2D(player.pos, ball_.pos);
    return dist <= HitRadius + player.racket.rangeBonus &&
           ball_.pos.z >= MinHitHeight && ball_.pos.z <= MaxHitHeight;
}

double MatchController::hitProbability(double dt) const {
    if (dt <= 0.1) {
        return 1.00;
    }
    if (dt <= 0.2) {
        return 0.90;
    }
    if (dt <= 0.5) {
        return 0.80;
    }
    if (dt <= 1.0) {
        return 0.50;
    }
    if (dt <= 1.5) {
        return 0.10;
    }
    return 0.0;
}

Vec3 MatchController::chooseTarget(const Player& player, const InputState& input) {
    double x = randomReal(-2.35, 2.35);
    double y = player.id == 1 ? randomReal(-9.6, -4.4) : randomReal(4.4, 9.6);

    if (player.id == 1) {
        if (input.p1Left) {
            x -= 1.45;
        }
        if (input.p1Right) {
            x += 1.45;
        }
        if (input.p1Up) {
            y -= randomReal(0.8, 1.9);
        }
        if (input.p1Down) {
            y += randomReal(0.8, 1.8);
        }
    } else {
        if (input.p2Left) {
            x -= 1.45;
        }
        if (input.p2Right) {
            x += 1.45;
        }
        if (input.p2Down) {
            y += randomReal(0.8, 1.9);
        }
        if (input.p2Up) {
            y -= randomReal(0.8, 1.8);
        }
    }

    x += randomReal(-player.racket.controlError, player.racket.controlError);
    y += randomReal(-player.racket.controlError * 1.25, player.racket.controlError * 1.25);
    return {clampDouble(x, -CourtHalfWidth + 0.32, CourtHalfWidth - 0.32),
            clampDouble(y, player.id == 1 ? -CourtHalfLength + 0.45 : 0.8,
                        player.id == 1 ? -0.8 : CourtHalfLength - 0.45),
            0.0};
}

Vec3 MatchController::chooseServeTarget(int serverId) {
    const double x = randomReal(-1.8, 1.8);
    const double y = serverId == 1 ? randomReal(-7.8, -5.5) : randomReal(5.5, 7.8);
    return {x, y, 0.0};
}

void MatchController::launchBallTo(const Vec3& target, int hitterId, double flightTime) {
    ball_.active = true;
    ball_.lastHitPlayer = hitterId;
    ball_.expectedReceiver = opponentOf(hitterId);
    ball_.bounceCount = 0;
    ball_.previousY = ball_.pos.y;

    if (ball_.pos.z < 0.45) {
        ball_.pos.z = 0.45;
    }

    ball_.vel.x = (target.x - ball_.pos.x) / flightTime;
    ball_.vel.y = (target.y - ball_.pos.y) / flightTime;
    ball_.vel.z = (target.z - ball_.pos.z + 0.5 * Gravity * flightTime * flightTime) / flightTime;

    p1_.hitOpportunityActive = false;
    p2_.hitOpportunityActive = false;
}

void MatchController::handleBounce() {
    const bool inBounds = std::abs(ball_.pos.x) <= CourtHalfWidth &&
                          std::abs(ball_.pos.y) <= CourtHalfLength;
    const bool correctSide = (ball_.expectedReceiver == 1 && ball_.pos.y > 0.0) ||
                             (ball_.expectedReceiver == 2 && ball_.pos.y < 0.0);

    if (ball_.bounceCount == 0) {
        if (!inBounds || !correctSide) {
            endPoint(opponentOf(ball_.lastHitPlayer), QStringLiteral("Out"));
            return;
        }

        ball_.bounceCount = 1;
        ball_.vel.z = -ball_.vel.z * 0.54;
        ball_.vel.x *= 0.88;
        ball_.vel.y *= 0.88;
        feedback_ = QStringLiteral("Bounce");
        feedbackTimer_ = 0.35;
        return;
    }

    endPoint(ball_.lastHitPlayer, QStringLiteral("Double bounce"));
}

void MatchController::endPoint(int winner, const QString& reason) {
    if (phase_ == MatchPhase::PointOver || phase_ == MatchPhase::MatchOver) {
        return;
    }

    ball_.active = false;
    score_.addPoint(winner);
    phase_ = MatchPhase::PointOver;
    pointOverTimer_ = 1.25;
    feedback_ = QStringLiteral("%1 - point P%2").arg(reason, QString::number(winner));
    feedbackTimer_ = 1.25;

    if (score_.isMatchOver()) {
        pointOverTimer_ = 1.0;
    }
}

void MatchController::drawCourt(QPainter& painter, const QRectF& courtRect) const {
    painter.save();

    QLinearGradient bg(courtRect.topLeft(), courtRect.bottomRight());
    bg.setColorAt(0.0, QColor(42, 137, 96));
    bg.setColorAt(1.0, QColor(28, 112, 82));
    painter.setBrush(bg);
    painter.setPen(QPen(QColor(235, 245, 238), 3));
    painter.drawRoundedRect(courtRect, 8, 8);

    auto line = [&](double x1, double y1, double x2, double y2, int width = 2) {
        painter.setPen(QPen(QColor(244, 250, 246), width, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(courtToScreen(x1, y1, 0.0, courtRect), courtToScreen(x2, y2, 0.0, courtRect));
    };

    line(-CourtHalfWidth, 0.0, CourtHalfWidth, 0.0, 3);
    line(-CourtHalfWidth, -6.40, CourtHalfWidth, -6.40);
    line(-CourtHalfWidth, 6.40, CourtHalfWidth, 6.40);
    line(0.0, -6.40, 0.0, 6.40);
    line(-CourtHalfWidth, -CourtHalfLength, -CourtHalfWidth, CourtHalfLength, 3);
    line(CourtHalfWidth, -CourtHalfLength, CourtHalfWidth, CourtHalfLength, 3);
    line(-CourtHalfWidth, -CourtHalfLength, CourtHalfWidth, -CourtHalfLength, 3);
    line(-CourtHalfWidth, CourtHalfLength, CourtHalfWidth, CourtHalfLength, 3);

    const QPointF netLeft = courtToScreen(-CourtHalfWidth - 0.25, 0.0, 0.0, courtRect);
    const QPointF netRight = courtToScreen(CourtHalfWidth + 0.25, 0.0, 0.0, courtRect);
    painter.setPen(QPen(QColor(25, 43, 48, 210), 8, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(netLeft, netRight);
    painter.setPen(QPen(QColor(232, 235, 230), 2, Qt::DashLine, Qt::RoundCap));
    painter.drawLine(netLeft, netRight);

    painter.restore();
}

void MatchController::drawPlayer(QPainter& painter, const Player& player, const QRectF& courtRect) const {
    painter.save();
    const QPointF feet = courtToScreen(player.pos.x, player.pos.y, 0.0, courtRect);
    const double scale = courtRect.height() / 720.0;
    const double bodyW = 36.0 * scale;
    const double bodyH = 42.0 * scale;
    const double headR = 15.0 * scale;
    const OutfitItem& outfit = player.outfit;

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 70));
    painter.drawEllipse(QPointF(feet.x(), feet.y() + 8.0 * scale), 28.0 * scale, 9.0 * scale);

    QRectF body(feet.x() - bodyW / 2.0, feet.y() - bodyH - 24.0 * scale, bodyW, bodyH);
    const double legTop = body.bottom() - 2.0 * scale;

    painter.setPen(QPen(QColor(248, 248, 242), 5.0 * scale, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(body.center().x() - 8.0 * scale, legTop + 12.0 * scale),
                     QPointF(body.center().x() - 11.0 * scale, feet.y() + 3.0 * scale));
    painter.drawLine(QPointF(body.center().x() + 8.0 * scale, legTop + 12.0 * scale),
                     QPointF(body.center().x() + 11.0 * scale, feet.y() + 3.0 * scale));

    painter.setPen(Qt::NoPen);
    painter.setBrush(outfit.shoeColor);
    painter.drawRoundedRect(QRectF(body.center().x() - 22.0 * scale, feet.y() - 1.0 * scale,
                                   18.0 * scale, 8.0 * scale),
                            4.0 * scale, 4.0 * scale);
    painter.drawRoundedRect(QRectF(body.center().x() + 4.0 * scale, feet.y() - 1.0 * scale,
                                   18.0 * scale, 8.0 * scale),
                            4.0 * scale, 4.0 * scale);

    painter.setBrush(outfit.shirtMain);
    painter.drawRoundedRect(body, 9.0 * scale, 9.0 * scale);

    painter.setBrush(outfit.shirtAccent);
    painter.drawRoundedRect(QRectF(body.left() + 4.0 * scale, body.top() + 5.0 * scale,
                                   8.0 * scale, body.height() - 8.0 * scale),
                            4.0 * scale, 4.0 * scale);
    painter.drawRoundedRect(QRectF(body.right() - 12.0 * scale, body.top() + 5.0 * scale,
                                   8.0 * scale, body.height() - 8.0 * scale),
                            4.0 * scale, 4.0 * scale);

    painter.setBrush(outfit.collarColor);
    QPolygonF collar;
    collar << QPointF(body.center().x() - 8.0 * scale, body.top() + 2.0 * scale)
           << QPointF(body.center().x(), body.top() + 13.0 * scale)
           << QPointF(body.center().x() + 8.0 * scale, body.top() + 2.0 * scale);
    painter.drawPolygon(collar);

    painter.setBrush(outfit.sleeveColor);
    painter.drawEllipse(QPointF(body.left() + 1.0 * scale, body.top() + 14.0 * scale),
                        8.0 * scale, 11.0 * scale);
    painter.drawEllipse(QPointF(body.right() - 1.0 * scale, body.top() + 14.0 * scale),
                        8.0 * scale, 11.0 * scale);

    painter.setPen(QPen(outfit.stripeColor, 2.0 * scale, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(body.left() + 7.0 * scale, body.top() + 9.0 * scale),
                     QPointF(body.left() + 7.0 * scale, body.bottom() - 7.0 * scale));
    painter.drawLine(QPointF(body.right() - 7.0 * scale, body.top() + 9.0 * scale),
                     QPointF(body.right() - 7.0 * scale, body.bottom() - 7.0 * scale));

    painter.setFont(QFont(QStringLiteral("Segoe UI Symbol"), static_cast<int>(13 * scale), QFont::Bold));
    painter.setPen(outfit.stripeColor);
    painter.drawText(QRectF(body.center().x() - 12.0 * scale, body.top() + 16.0 * scale,
                            24.0 * scale, 18.0 * scale),
                     Qt::AlignCenter, outfit.chestMark);
    painter.setPen(Qt::NoPen);

    if (player.gender == Gender::Female) {
        QPolygonF skirt;
        skirt << QPointF(body.left() - 5.0 * scale, body.bottom() - 3.0 * scale)
              << QPointF(body.right() + 5.0 * scale, body.bottom() - 3.0 * scale)
              << QPointF(body.center().x() + 20.0 * scale, body.bottom() + 21.0 * scale)
              << QPointF(body.center().x() - 20.0 * scale, body.bottom() + 21.0 * scale);
        painter.setBrush(outfit.bottomMain);
        painter.drawPolygon(skirt);
        painter.setPen(QPen(outfit.bottomAccent, 2.0 * scale, Qt::SolidLine, Qt::RoundCap));
        for (int i = -2; i <= 2; ++i) {
            painter.drawLine(QPointF(body.center().x() + i * 7.0 * scale, body.bottom()),
                             QPointF(body.center().x() + i * 9.0 * scale, body.bottom() + 19.0 * scale));
        }
        painter.setPen(Qt::NoPen);
    } else {
        painter.setBrush(outfit.bottomMain);
        painter.drawRoundedRect(QRectF(body.left() + 1.0 * scale, body.bottom() - 2.0 * scale,
                                       body.width() / 2.0 - 3.0 * scale, 21.0 * scale),
                                4.0 * scale, 4.0 * scale);
        painter.drawRoundedRect(QRectF(body.center().x() + 2.0 * scale, body.bottom() - 2.0 * scale,
                                       body.width() / 2.0 - 3.0 * scale, 21.0 * scale),
                                4.0 * scale, 4.0 * scale);
        painter.setPen(QPen(outfit.bottomAccent, 2.0 * scale));
        painter.drawLine(QPointF(body.center().x(), body.bottom() + 2.0 * scale),
                         QPointF(body.center().x(), body.bottom() + 17.0 * scale));
        painter.setPen(Qt::NoPen);
    }

    painter.setPen(QPen(QColor(245, 202, 164), 5.0 * scale, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(body.left() + 3.0 * scale, body.top() + 20.0 * scale),
                     QPointF(body.left() - 8.0 * scale, body.top() + 34.0 * scale));
    painter.drawLine(QPointF(body.right() - 3.0 * scale, body.top() + 20.0 * scale),
                     QPointF(body.right() + 10.0 * scale, body.top() + 34.0 * scale));
    painter.setPen(Qt::NoPen);

    const QPointF headCenter(body.center().x(), body.top() - 18.0 * scale);
    if (player.gender == Gender::Female) {
        painter.setBrush(outfit.hairColor.darker(115));
        painter.drawEllipse(QPointF(headCenter.x() + 13.0 * scale, headCenter.y() + 1.0 * scale),
                            9.0 * scale, 13.0 * scale);
    }
    painter.setBrush(outfit.hairColor);
    painter.drawEllipse(QPointF(headCenter.x(), headCenter.y() - 2.0 * scale),
                        15.5 * scale, 14.0 * scale);
    painter.setBrush(QColor(245, 202, 164));
    painter.drawEllipse(headCenter, headR, headR);

    painter.setBrush(outfit.hairColor);
    painter.drawPie(QRectF(headCenter.x() - 14.0 * scale, headCenter.y() - 16.0 * scale,
                           28.0 * scale, 17.0 * scale),
                    0, 180 * 16);
    painter.drawEllipse(QPointF(headCenter.x() - 9.0 * scale, headCenter.y() - 7.0 * scale),
                        4.0 * scale, 5.0 * scale);
    painter.drawEllipse(QPointF(headCenter.x() + 6.0 * scale, headCenter.y() - 7.0 * scale),
                        4.0 * scale, 5.0 * scale);

    painter.setBrush(outfit.hatColor);
    painter.drawPie(QRectF(headCenter.x() - 17.0 * scale, headCenter.y() - 24.0 * scale,
                           34.0 * scale, 20.0 * scale),
                    0, 180 * 16);
    painter.setBrush(outfit.shirtAccent);
    painter.drawRoundedRect(QRectF(headCenter.x() + 6.0 * scale, headCenter.y() - 14.0 * scale,
                                   18.0 * scale, 5.0 * scale),
                            3.0 * scale, 3.0 * scale);

    if (player.gender == Gender::Female) {
        painter.setBrush(outfit.hatColor.lighter(112));
        painter.drawEllipse(QPointF(headCenter.x() - 11.0 * scale, headCenter.y() - 24.0 * scale),
                            4.0 * scale, 10.0 * scale);
        painter.drawEllipse(QPointF(headCenter.x() + 4.0 * scale, headCenter.y() - 25.0 * scale),
                            4.0 * scale, 10.0 * scale);
    }

    painter.setBrush(QColor(35, 41, 48));
    painter.drawEllipse(QPointF(headCenter.x() - 5.5 * scale, headCenter.y() - 2.0 * scale),
                        1.8 * scale, 2.6 * scale);
    painter.drawEllipse(QPointF(headCenter.x() + 5.5 * scale, headCenter.y() - 2.0 * scale),
                        1.8 * scale, 2.6 * scale);
    painter.setBrush(QColor(255, 145, 160, 135));
    painter.drawEllipse(QPointF(headCenter.x() - 10.0 * scale, headCenter.y() + 5.5 * scale),
                        3.0 * scale, 2.0 * scale);
    painter.drawEllipse(QPointF(headCenter.x() + 10.0 * scale, headCenter.y() + 5.5 * scale),
                        3.0 * scale, 2.0 * scale);
    painter.setPen(QPen(QColor(130, 65, 62), 1.4 * scale, Qt::SolidLine, Qt::RoundCap));
    painter.drawArc(QRectF(headCenter.x() - 5.0 * scale, headCenter.y() + 1.0 * scale,
                           10.0 * scale, 9.0 * scale),
                    210 * 16, 120 * 16);

    const double racketDir = player.id == 1 ? 1.0 : -1.0;
    const QPointF hand(body.right() + 2.0 * scale, body.center().y());
    const QPointF grip(hand.x() + racketDir * 22.0 * scale, hand.y() + 14.0 * scale);
    drawRacket(painter, player, grip, racketDir, scale);

    painter.setFont(QFont(QStringLiteral("Segoe UI"), static_cast<int>(11 * scale), QFont::Bold));
    painter.setPen(QColor(245, 248, 250));
    painter.drawText(QRectF(feet.x() - 28.0 * scale, feet.y() + 13.0 * scale,
                            56.0 * scale, 18.0 * scale),
                     Qt::AlignCenter, player.name);
    painter.restore();
}

void MatchController::drawRacket(QPainter& painter, const Player& player, const QPointF& grip,
                                 double racketDir, double scale) const {
    const RacketItem& racket = player.racket;
    const QPointF headCenter(grip.x() + racketDir * 15.0 * scale, grip.y() + 1.0 * scale);

    painter.save();
    painter.setPen(QPen(racket.gripColor, 5.0 * scale, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(grip.x() - racketDir * 18.0 * scale, grip.y() - 12.0 * scale), grip);

    painter.setPen(QPen(racket.frameColor, 4.0 * scale));
    painter.setBrush(QColor(255, 255, 255, 18));
    painter.drawEllipse(headCenter, 13.5 * scale, 18.0 * scale);

    painter.setPen(QPen(racket.accentColor, 2.2 * scale));
    painter.drawArc(QRectF(headCenter.x() - 12.0 * scale, headCenter.y() - 16.0 * scale,
                           24.0 * scale, 32.0 * scale),
                    35 * 16, 110 * 16);

    painter.setPen(QPen(racket.stringColor, 0.9 * scale));
    for (int i = -2; i <= 2; ++i) {
        painter.drawLine(QPointF(headCenter.x() + i * 4.0 * scale, headCenter.y() - 14.0 * scale),
                         QPointF(headCenter.x() + i * 4.0 * scale, headCenter.y() + 14.0 * scale));
    }
    for (int i = -2; i <= 2; ++i) {
        painter.drawLine(QPointF(headCenter.x() - 10.0 * scale, headCenter.y() + i * 5.0 * scale),
                         QPointF(headCenter.x() + 10.0 * scale, headCenter.y() + i * 5.0 * scale));
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(racket.accentColor);
    for (int i = 0; i < racket.starLevel; ++i) {
        painter.drawEllipse(QPointF(headCenter.x() - 8.0 * scale + i * 4.0 * scale,
                                    headCenter.y() - 22.0 * scale),
                            1.3 * scale, 1.3 * scale);
    }

    painter.restore();
}

void MatchController::drawBall(QPainter& painter, const QRectF& courtRect) const {
    painter.save();
    const QPointF shadow = courtToScreen(ball_.pos.x, ball_.pos.y, 0.0, courtRect);
    const QPointF center = courtToScreen(ball_.pos.x, ball_.pos.y, ball_.pos.z, courtRect);
    const double radius = 8.0 + ball_.pos.z * 1.8;

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 65));
    painter.drawEllipse(QPointF(shadow.x(), shadow.y() + 4.0), 9.0, 4.0);

    painter.setBrush(QColor(230, 237, 58));
    painter.setPen(QPen(QColor(245, 252, 145), 2));
    painter.drawEllipse(center, radius, radius);
    painter.setPen(QPen(QColor(245, 252, 145), 1.4));
    painter.drawArc(QRectF(center.x() - radius * 0.75, center.y() - radius,
                           radius * 1.5, radius * 2.0),
                    70 * 16, 110 * 16);
    painter.drawArc(QRectF(center.x() - radius * 0.75, center.y() - radius,
                           radius * 1.5, radius * 2.0),
                    250 * 16, 110 * 16);
    painter.restore();
}

void MatchController::drawOverlay(QPainter& painter, const QSize& size) const {
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    QRectF topPanel(28, 20, size.width() - 56, 58);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(13, 25, 31, 220));
    painter.drawRoundedRect(topPanel, 8, 8);

    painter.setPen(QColor(242, 247, 250));
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 14, QFont::Bold));
    painter.drawText(QRectF(topPanel.left() + 18, topPanel.top() + 8, 220, 24),
                     Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("Tennis Duel"));

    painter.setFont(QFont(QStringLiteral("Segoe UI"), 13, QFont::DemiBold));
    painter.drawText(QRectF(topPanel.center().x() - 170, topPanel.top() + 7, 340, 22),
                     Qt::AlignCenter, score_.pointText());
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 11));
    painter.setPen(QColor(205, 220, 224));
    painter.drawText(QRectF(topPanel.center().x() - 170, topPanel.top() + 31, 340, 18),
                     Qt::AlignCenter, score_.gamesText());

    painter.setPen(QColor(214, 225, 226));
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 10));
    painter.drawText(QRectF(topPanel.right() - 330, topPanel.top() + 10, 310, 38),
                     Qt::AlignRight | Qt::AlignVCenter,
                     QStringLiteral("P1 WASD + Space    P2 Arrows + J"));

    if (feedbackTimer_ > 0.0 || phase_ == MatchPhase::ServeReady || phase_ == MatchPhase::MatchOver) {
        QRectF box(size.width() / 2.0 - 180, size.height() - 82, 360, 44);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(12, 25, 30, 205));
        painter.drawRoundedRect(box, 8, 8);
        painter.setPen(QColor(250, 250, 246));
        painter.setFont(QFont(QStringLiteral("Segoe UI"), 15, QFont::Bold));
        QString text = feedback_;
        if (phase_ == MatchPhase::MatchOver) {
            text = QStringLiteral("P%1 wins the match").arg(score_.matchWinner());
        }
        painter.drawText(box, Qt::AlignCenter, text);
    }

    painter.restore();
}

Player& MatchController::playerById(int id) {
    return id == 1 ? p1_ : p2_;
}

const Player& MatchController::playerById(int id) const {
    return id == 1 ? p1_ : p2_;
}

double MatchController::randomReal(double minValue, double maxValue) {
    std::uniform_real_distribution<double> dist(minValue, maxValue);
    return dist(rng_);
}
