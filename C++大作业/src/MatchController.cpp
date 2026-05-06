#include "MatchController.h"

#include <QCoreApplication>
#include <QDir>
#include <QFont>
#include <QHash>
#include <QImage>
#include <QLinearGradient>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>

#include <algorithm>
#include <vector>

namespace {

struct AiProfile {
    double moveSpeedScale = 1.0;
    double planXError = 0.5;
    double planYError = 0.8;
    double reactionMin = 0.25;
    double reactionMax = 0.55;
    double hitBonus = 0.0;
    double targetVariety = 1.0;
    double controlScale = 1.0;
    double serveDelayMin = 0.7;
    double serveDelayMax = 1.3;
};

struct CourtTheme {
    QColor backdropTop;
    QColor backdropMid;
    QColor backdropBottom;
    QColor apron;
    QColor outerTop;
    QColor outerBottom;
    QColor serviceTop;
    QColor serviceBottom;
    QColor line;
    QColor lineShadow;
    QColor netBand;
    QColor netTop;
    QColor accent;
    QColor texture;
};

const CourtTheme& courtThemeAt(int index) {
    static const CourtTheme themes[] = {
        {
            QColor(11, 46, 42),
            QColor(35, 103, 65),
            QColor(74, 132, 58),
            QColor(28, 117, 63, 155),
            QColor(31, 132, 78),
            QColor(18, 94, 61),
            QColor(73, 177, 106),
            QColor(41, 146, 87),
            QColor(248, 255, 239),
            QColor(4, 43, 32, 95),
            QColor(18, 47, 43),
            QColor(247, 251, 219),
            QColor(245, 205, 82),
            QColor(189, 243, 154, 40),
        },
        {
            QColor(44, 31, 34),
            QColor(128, 58, 47),
            QColor(174, 91, 55),
            QColor(162, 75, 48, 155),
            QColor(187, 82, 49),
            QColor(132, 49, 38),
            QColor(217, 107, 64),
            QColor(172, 69, 45),
            QColor(255, 246, 225),
            QColor(82, 25, 25, 90),
            QColor(45, 31, 35),
            QColor(255, 248, 214),
            QColor(107, 184, 122),
            QColor(255, 222, 169, 46),
        },
        {
            QColor(38, 50, 52),
            QColor(134, 123, 48),
            QColor(211, 158, 55),
            QColor(229, 184, 70, 145),
            QColor(222, 183, 56),
            QColor(183, 130, 35),
            QColor(251, 210, 91),
            QColor(226, 171, 54),
            QColor(255, 255, 245),
            QColor(97, 72, 22, 86),
            QColor(41, 58, 61),
            QColor(255, 250, 219),
            QColor(64, 165, 154),
            QColor(255, 247, 184, 45),
        },
    };
    return themes[index % 3];
}

AiProfile profileFor(AiDifficulty difficulty) {
    switch (difficulty) {
    case AiDifficulty::Easy:
        return {0.78, 1.25, 1.65, 0.48, 0.95, -0.12, 0.55, 1.35, 1.05, 1.75};
    case AiDifficulty::Hard:
        return {1.18, 0.22, 0.32, 0.04, 0.22, 0.12, 1.45, 0.45, 0.45, 0.95};
    case AiDifficulty::Medium:
    default:
        return {0.98, 0.62, 0.85, 0.20, 0.52, 0.02, 1.00, 0.85, 0.70, 1.30};
    }
}

QString spritePathFor(Gender gender, bool hitting) {
    if (gender == Gender::Female) {
        return hitting ? QStringLiteral(":/assets/player_hit_female.png")
                       : QStringLiteral(":/assets/player_front_female.png");
    }
    return hitting ? QStringLiteral(":/assets/player_hit_male.png")
                   : QStringLiteral(":/assets/player_front_male.png");
}

bool isNearWhite(int r, int g, int b) {
    const int maxValue = std::max({r, g, b});
    const int minValue = std::min({r, g, b});
    return (r > 238 && g > 238 && b > 238) ||
           (minValue > 188 && maxValue - minValue < 42);
}

QColor shadedColor(const QColor& base, int luminance) {
    const double shade = std::clamp(luminance / 72.0, 0.48, 1.24);
    return QColor(std::clamp(static_cast<int>(base.red() * shade), 0, 255),
                  std::clamp(static_cast<int>(base.green() * shade), 0, 255),
                  std::clamp(static_cast<int>(base.blue() * shade), 0, 255));
}

QImage loadSpriteImage(const QString& path) {
    QImage image(path);
    if (!image.isNull()) {
        return image;
    }

    const int slash = path.lastIndexOf(QLatin1Char('/'));
    const QString fileName = slash >= 0 ? path.mid(slash + 1) : path;
    const QString current = QDir::current().absoluteFilePath(QStringLiteral("assets/") + fileName);
    image.load(current);
    if (!image.isNull()) {
        return image;
    }

    const QDir appDir(QCoreApplication::applicationDirPath());
    const QString besideExe = appDir.absoluteFilePath(QStringLiteral("assets/") + fileName);
    image.load(besideExe);
    if (!image.isNull()) {
        return image;
    }

    image.load(appDir.absoluteFilePath(QStringLiteral("../assets/") + fileName));
    return image;
}

QRect keepLargestAlphaComponent(QImage& image) {
    const int width = image.width();
    const int height = image.height();
    const int total = width * height;
    std::vector<unsigned char> visited(total, 0);
    std::vector<int> stack;
    std::vector<int> component;
    std::vector<int> bestComponent;
    stack.reserve(4096);
    component.reserve(4096);

    auto isVisible = [&](int index) {
        const int x = index % width;
        const int y = index / width;
        return qAlpha(image.pixel(x, y)) > 16;
    };

    for (int start = 0; start < total; ++start) {
        if (visited[start] || !isVisible(start)) {
            continue;
        }

        component.clear();
        stack.clear();
        stack.push_back(start);
        visited[start] = 1;

        while (!stack.empty()) {
            const int index = stack.back();
            stack.pop_back();
            component.push_back(index);

            const int x = index % width;
            const int y = index / width;
            const int neighbors[4] = {
                x > 0 ? index - 1 : -1,
                x + 1 < width ? index + 1 : -1,
                y > 0 ? index - width : -1,
                y + 1 < height ? index + width : -1,
            };

            for (int next : neighbors) {
                if (next >= 0 && !visited[next] && isVisible(next)) {
                    visited[next] = 1;
                    stack.push_back(next);
                }
            }
        }

        if (component.size() > bestComponent.size()) {
            bestComponent = component;
        }
    }

    if (bestComponent.empty()) {
        return image.rect();
    }

    std::vector<unsigned char> keep(total, 0);
    int left = width;
    int top = height;
    int right = 0;
    int bottom = 0;
    for (int index : bestComponent) {
        keep[index] = 1;
        const int x = index % width;
        const int y = index / width;
        left = std::min(left, x);
        top = std::min(top, y);
        right = std::max(right, x);
        bottom = std::max(bottom, y);
    }

    for (int y = 0; y < height; ++y) {
        QRgb* row = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < width; ++x) {
            const int index = y * width + x;
            if (!keep[index]) {
                row[x] = qRgba(0, 0, 0, 0);
            }
        }
    }

    return QRect(left, top, right - left + 1, bottom - top + 1)
        .adjusted(-8, -8, 8, 8)
        .intersected(image.rect());
}

QImage colorizedSpriteImage(const QString& path, const OutfitItem& outfit, const RacketItem& racket) {
    QImage image = loadSpriteImage(path);
    if (image.isNull()) {
        return {};
    }

    image = image.convertToFormat(QImage::Format_ARGB32);
    const int width = image.width();
    const int height = image.height();

    for (int y = 0; y < height; ++y) {
        QRgb* row = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < width; ++x) {
            const QRgb pixel = row[x];
            const int r = qRed(pixel);
            const int g = qGreen(pixel);
            const int b = qBlue(pixel);
            if (isNearWhite(r, g, b)) {
                row[x] = qRgba(0, 0, 0, 0);
            } else {
                row[x] = qRgba(r, g, b, 255);
            }
        }
    }

    const QRect crop = keepLargestAlphaComponent(image);

    for (int y = 0; y < height; ++y) {
        QRgb* row = reinterpret_cast<QRgb*>(image.scanLine(y));
        const double ny = static_cast<double>(y) / std::max(1, height - 1);
        for (int x = 0; x < width; ++x) {
            const QRgb pixel = row[x];
            if (qAlpha(pixel) == 0) {
                continue;
            }

            const int r = qRed(pixel);
            const int g = qGreen(pixel);
            const int b = qBlue(pixel);
            const int maxValue = std::max({r, g, b});
            const int minValue = std::min({r, g, b});
            const int saturation = maxValue - minValue;
            const int luminance = qGray(pixel);
            const double nx = static_cast<double>(x) / std::max(1, width - 1);

            const bool darkCloth = ny > 0.42 && luminance >= 30 && luminance < 118 && saturation < 76;
            const bool headBand = ny < 0.36 && nx > 0.16 && nx < 0.80 &&
                                  g > 105 && b > 105 && r < 185 && std::max(g, b) - r > 24;
            const bool shoeArea = ny > 0.79 && g > 80 && b > 95 && r < 190 &&
                                  std::max(g, b) - r > 22;
            const bool racketFrame = !headBand && !shoeArea &&
                                      g > 95 && b > 95 && r < 190 && std::max(g, b) - r > 24;

            if (darkCloth) {
                const QColor base = ny < 0.74 ? outfit.shirtAccent : outfit.bottomMain;
                const QColor color = shadedColor(base, luminance);
                row[x] = qRgba(color.red(), color.green(), color.blue(), qAlpha(pixel));
            } else if (headBand) {
                const QColor color = shadedColor(outfit.hatColor, luminance);
                row[x] = qRgba(color.red(), color.green(), color.blue(), qAlpha(pixel));
            } else if (shoeArea) {
                const QColor color = shadedColor(outfit.shoeColor, luminance);
                row[x] = qRgba(color.red(), color.green(), color.blue(), qAlpha(pixel));
            } else if (racketFrame) {
                const QColor color = shadedColor(racket.frameColor, luminance);
                row[x] = qRgba(color.red(), color.green(), color.blue(), qAlpha(pixel));
            }
        }
    }

    return image.copy(crop);
}

const QPixmap& playerSpritePixmap(const Player& player, bool hitting) {
    static QHash<QString, QPixmap> cache;
    const RacketItem& racket = player.racket;
    const QString path = spritePathFor(player.gender, hitting);
    const QString key = path + QStringLiteral("|") + player.outfit.id +
                        QStringLiteral("|") + racket.id +
                        QStringLiteral("|") + QString::number(racket.frameColor.rgba(), 16) +
                        QStringLiteral("|") + QString::number(racket.accentColor.rgba(), 16) +
                        QStringLiteral("|") + QString::number(racket.gripColor.rgba(), 16);

    auto it = cache.constFind(key);
    if (it != cache.constEnd()) {
        return it.value();
    }

    QPixmap pixmap = QPixmap::fromImage(colorizedSpriteImage(path, player.outfit, player.racket));
    auto inserted = cache.insert(key, pixmap);
    return inserted.value();
}

} // namespace

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

QString ScoreSystem::pointTextFor(int playerId) const {
    if (advantage_ == playerId) {
        return QStringLiteral("AD");
    }
    if (advantage_ != 0) {
        return QStringLiteral("40");
    }
    return pointName(playerId == 1 ? p1Point_ : p2Point_);
}

QString ScoreSystem::scoreNoteText() const {
    if (advantage_ == 1) {
        return QStringLiteral("P1 ADV");
    }
    if (advantage_ == 2) {
        return QStringLiteral("P2 ADV");
    }
    if (p1Point_ == 3 && p2Point_ == 3) {
        return QStringLiteral("DEUCE");
    }
    return QStringLiteral("POINT");
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
                            const OutfitItem& p2Outfit, const RacketItem& p2Racket,
                            PlayMode playMode, AiDifficulty aiDifficulty) {
    playMode_ = playMode;
    aiDifficulty_ = aiDifficulty;
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
        playMode_ == PlayMode::SinglePlayer ? QStringLiteral("CPU") : QStringLiteral("P2"),
        {0.0, -8.6},
        p2Outfit,
        p2Racket,
        false,
        0.0};

    server_ = 1;
    serveFaults_ = 0;
    phase_ = MatchPhase::ServeReady;
    time_ = 0.0;
    pointOverTimer_ = 0.0;
    aiServeTimer_ = 0.0;
    serveInFlight_ = false;
    aiOpportunityWasActive_ = false;
    plannedServeFault_ = ServeFaultPlan::None;
    aiReactionDelay_ = 0.35;
    aiMoveTarget_ = {0.0, -8.6};
    aiPlannedLastHitPlayer_ = 0;
    aiPlannedBounceCount_ = -1;
    std::uniform_int_distribution<int> courtThemeDist(0, 2);
    courtTheme_ = courtThemeDist(rng_);
    feedback_ = QStringLiteral("P1 serve: Space");
    feedbackTimer_ = 3.0;
    startServe(server_);
}

void MatchController::update(const InputState& input, double dt) {
    dt = clampDouble(dt, 0.0, 0.05);
    time_ += dt;
    feedbackTimer_ = std::max(0.0, feedbackTimer_ - dt);
    p1_.hitPoseTimer = std::max(0.0, p1_.hitPoseTimer - dt);
    p2_.hitPoseTimer = std::max(0.0, p2_.hitPoseTimer - dt);

    InputState activeInput = input;
    if (playMode_ == PlayMode::SinglePlayer) {
        applyAiMovement(activeInput, dt);
    }

    updatePlayers(activeInput, dt);

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

        if (playMode_ == PlayMode::SinglePlayer && server_ == 2) {
            aiServeTimer_ -= dt;
            if (aiServeTimer_ <= 0.0) {
                activeInput.p2HitPressed = true;
            }
        }

        if ((server_ == 1 && activeInput.p1HitPressed) ||
            (server_ == 2 && activeInput.p2HitPressed)) {
            tryServe(server_);
        }
        return;
    }

    updateBall(dt);
    updateHitOpportunities();
    if (playMode_ == PlayMode::SinglePlayer) {
        updateAiHitDecision(activeInput);
    }
    handleHitInput(activeInput);
}

void MatchController::draw(QPainter& painter, const QSize& size) const {
    const CourtTheme& theme = courtThemeAt(courtTheme_);
    QLinearGradient arena(0, 0, 0, size.height());
    arena.setColorAt(0.0, theme.backdropTop);
    arena.setColorAt(0.52, theme.backdropMid);
    arena.setColorAt(1.0, theme.backdropBottom);
    painter.fillRect(QRect(QPoint(0, 0), size), arena);

    const QRectF courtRect = courtRectFor(size);
    lastCourtRect_ = courtRect;

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255, 18));
    painter.drawRect(QRectF(0, 0, size.width(), 90));
    painter.setBrush(theme.apron);
    painter.drawRect(QRectF(0, courtRect.bottom() + 16, size.width(), size.height() - courtRect.bottom() - 16));

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
    auto movePlayer = [&](Player& player, bool up, bool down, bool left, bool right, double speedScale) {
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

        player.pos.x += dx * PlayerSpeed * speedScale * dt;
        player.pos.y += dy * PlayerSpeed * speedScale * dt;

        player.pos.x = clampDouble(player.pos.x, -CourtHalfWidth + 0.25, CourtHalfWidth - 0.25);
        if (player.id == 1) {
            player.pos.y = clampDouble(player.pos.y, 0.65, CourtHalfLength - 0.45);
        } else {
            player.pos.y = clampDouble(player.pos.y, -CourtHalfLength + 0.45, -0.65);
        }
    };

    const double p2SpeedScale = playMode_ == PlayMode::SinglePlayer
                                    ? profileFor(aiDifficulty_).moveSpeedScale
                                    : 1.0;
    movePlayer(p1_, input.p1Up, input.p1Down, input.p1Left, input.p1Right, 1.0);
    movePlayer(p2_, input.p2Up, input.p2Down, input.p2Left, input.p2Right, p2SpeedScale);
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
            if (serveInFlight_) {
                registerServeFault(QStringLiteral("Fault: net"));
            } else {
                endPoint(opponentOf(ball_.lastHitPlayer), QStringLiteral("Net"));
            }
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

void MatchController::applyAiMovement(InputState& input, double) {
    input.p2Up = false;
    input.p2Down = false;
    input.p2Left = false;
    input.p2Right = false;

    const AiProfile profile = profileFor(aiDifficulty_);
    Vec2 target{0.0, -8.6};

    if (phase_ == MatchPhase::Rally && ball_.active && ball_.expectedReceiver == 2) {
        const bool needsNewPlan = aiPlannedLastHitPlayer_ != ball_.lastHitPlayer ||
                                  aiPlannedBounceCount_ != ball_.bounceCount;
        if (needsNewPlan) {
            const Vec2 landing = predictBallLanding();
            aiMoveTarget_.x = clampDouble(landing.x + randomReal(-profile.planXError, profile.planXError),
                                          -CourtHalfWidth + 0.35, CourtHalfWidth - 0.35);
            aiMoveTarget_.y = clampDouble(landing.y + randomReal(-profile.planYError, profile.planYError),
                                          -CourtHalfLength + 0.55, -1.05);
            aiPlannedLastHitPlayer_ = ball_.lastHitPlayer;
            aiPlannedBounceCount_ = ball_.bounceCount;
        }
        target = aiMoveTarget_;
    } else if (phase_ == MatchPhase::ServeReady && server_ == 2) {
        target = {0.0, -8.8};
    } else {
        target = {0.0, -8.1};
    }

    const double deadZone = 0.18;
    if (p2_.pos.x < target.x - deadZone) {
        input.p2Right = true;
    } else if (p2_.pos.x > target.x + deadZone) {
        input.p2Left = true;
    }

    if (p2_.pos.y < target.y - deadZone) {
        input.p2Down = true;
    } else if (p2_.pos.y > target.y + deadZone) {
        input.p2Up = true;
    }
}

void MatchController::updateAiHitDecision(InputState& input) {
    input.p2HitPressed = false;

    if (!canHit(p2_)) {
        aiOpportunityWasActive_ = false;
        return;
    }

    if (!aiOpportunityWasActive_) {
        const AiProfile profile = profileFor(aiDifficulty_);
        aiOpportunityWasActive_ = true;
        aiReactionDelay_ = randomReal(profile.reactionMin, profile.reactionMax);
    }

    if (time_ - p2_.hitOpportunityStart >= aiReactionDelay_) {
        input.p2HitPressed = true;
    }
}

void MatchController::resetForNextPoint() {
    server_ = (score_.gamesFor(1) + score_.gamesFor(2)) % 2 == 0 ? 1 : 2;
    serveFaults_ = 0;
    serveInFlight_ = false;
    plannedServeFault_ = ServeFaultPlan::None;
    p1_.pos = {0.0, 8.6};
    p2_.pos = {0.0, -8.6};
    p1_.hitOpportunityActive = false;
    p2_.hitOpportunityActive = false;
    p1_.hitPoseTimer = 0.0;
    p2_.hitPoseTimer = 0.0;
    aiOpportunityWasActive_ = false;
    aiMoveTarget_ = {0.0, -8.6};
    aiPlannedLastHitPlayer_ = 0;
    aiPlannedBounceCount_ = -1;
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
    serveInFlight_ = false;
    plannedServeFault_ = ServeFaultPlan::None;

    const bool secondServe = serveFaults_ > 0;
    if (playMode_ == PlayMode::SinglePlayer && serverId == 2) {
        const AiProfile profile = profileFor(aiDifficulty_);
        aiServeTimer_ = randomReal(profile.serveDelayMin, profile.serveDelayMax);
        feedback_ = secondServe ? QStringLiteral("CPU second serve") : QStringLiteral("CPU serve");
    } else {
        if (serverId == 1) {
            feedback_ = secondServe ? QStringLiteral("P1 second serve: Space")
                                    : QStringLiteral("P1 serve: Space");
        } else {
            feedback_ = secondServe ? QStringLiteral("P2 second serve: J")
                                    : QStringLiteral("P2 serve: J");
        }
    }
    feedbackTimer_ = 2.5;
}

void MatchController::tryServe(int playerId) {
    if (phase_ != MatchPhase::ServeReady || playerId != server_) {
        return;
    }

    plannedServeFault_ = chooseServeFaultPlan(playerId);
    const Vec3 target = chooseServeTarget(playerId);
    const double flightTime = plannedServeFault_ == ServeFaultPlan::Net
                                  ? 0.62
                                  : (serveFaults_ > 0 ? 1.12 : 0.95);
    playerById(playerId).hitPoseTimer = 0.38;
    launchBallTo(target, playerId, flightTime);
    serveInFlight_ = true;
    phase_ = MatchPhase::Rally;
    feedback_ = serveFaults_ > 0 ? QStringLiteral("Second serve")
                                 : QStringLiteral("First serve");
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

    player.hitPoseTimer = 0.38;
    const double dt = player.hitOpportunityActive ? time_ - player.hitOpportunityStart : 0.0;
    const double probability = std::min(1.0, hitProbability(dt) + player.racket.hitBonus +
                                                 (playMode_ == PlayMode::SinglePlayer && player.id == 2
                                                      ? aiHitBonus()
                                                      : 0.0));
    if (randomReal(0.0, 1.0) > probability) {
        const QString missText = player.id == 1
                                     ? QStringLiteral("P1 miss")
                                     : (playMode_ == PlayMode::SinglePlayer ? QStringLiteral("CPU miss")
                                                                             : QStringLiteral("P2 miss"));
        endPoint(opponentOf(player.id), missText);
        return;
    }

    Vec3 target = playMode_ == PlayMode::SinglePlayer && player.id == 2
                      ? chooseAiTarget(player)
                      : chooseTarget(player, input);
    const double distance = std::sqrt((target.x - ball_.pos.x) * (target.x - ball_.pos.x) +
                                      (target.y - ball_.pos.y) * (target.y - ball_.pos.y));
    const double flightTime = clampDouble(distance / 10.4 + randomReal(-0.08, 0.16), 0.68, 1.40);
    launchBallTo(target, player.id, flightTime);
    serveInFlight_ = false;
    plannedServeFault_ = ServeFaultPlan::None;

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
    if (serveInFlight_ && ball_.bounceCount == 0) {
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

Vec3 MatchController::chooseAiTarget(const Player& player) {
    const AiProfile profile = profileFor(aiDifficulty_);
    double x = 0.0;
    double y = 0.0;

    if (aiDifficulty_ == AiDifficulty::Easy) {
        x = randomReal(-1.55, 1.55);
        y = randomReal(5.1, 8.4);
    } else if (aiDifficulty_ == AiDifficulty::Medium) {
        x = randomReal(-2.45, 2.45);
        y = randomReal(4.4, 9.8);
        if (randomReal(0.0, 1.0) < 0.35) {
            x = p1_.pos.x < 0.0 ? randomReal(1.0, 3.0) : randomReal(-3.0, -1.0);
        }
    } else {
        const double oppositeCorner = p1_.pos.x < 0.0 ? 1.0 : -1.0;
        if (randomReal(0.0, 1.0) < 0.62) {
            x = oppositeCorner * randomReal(2.15, 3.65);
            y = randomReal(7.2, 10.7);
        } else {
            x = randomReal(-3.4, 3.4);
            y = randomReal(3.8, 6.4);
        }
    }

    const double error = player.racket.controlError * profile.controlScale;
    x += randomReal(-error * profile.targetVariety, error * profile.targetVariety);
    y += randomReal(-error * 1.1, error * 1.1);

    return {clampDouble(x, -CourtHalfWidth + 0.32, CourtHalfWidth - 0.32),
            clampDouble(y, 0.8, CourtHalfLength - 0.45),
            0.0};
}

Vec3 MatchController::chooseServeTarget(int serverId) {
    if (plannedServeFault_ == ServeFaultPlan::Out) {
        const bool longFault = randomReal(0.0, 1.0) < 0.72;
        const double x = longFault
                             ? randomReal(-CourtHalfWidth + 0.35, CourtHalfWidth - 0.35)
                             : randomReal(CourtHalfWidth + 0.45, CourtHalfWidth + 1.15) *
                                   (randomReal(0.0, 1.0) < 0.5 ? -1.0 : 1.0);
        const double y = serverId == 1 ? randomReal(-9.7, -6.75) : randomReal(6.75, 9.7);
        return {x, y, 0.0};
    }

    const double x = randomReal(-3.05, 3.05);
    const double y = serverId == 1 ? randomReal(-5.85, -1.35) : randomReal(1.35, 5.85);
    return {x, y, 0.0};
}

MatchController::ServeFaultPlan MatchController::chooseServeFaultPlan(int serverId) {
    const Player& server = playerById(serverId);
    double chance = serveFaults_ > 0 ? 0.055 : 0.16;
    chance += server.racket.controlError * 0.015;
    chance -= server.racket.hitBonus * 0.45;

    if (playMode_ == PlayMode::SinglePlayer && serverId == 2) {
        switch (aiDifficulty_) {
        case AiDifficulty::Easy:
            chance += 0.035;
            break;
        case AiDifficulty::Hard:
            chance -= 0.025;
            break;
        case AiDifficulty::Medium:
        default:
            break;
        }
    }

    chance = clampDouble(chance, serveFaults_ > 0 ? 0.025 : 0.07, serveFaults_ > 0 ? 0.12 : 0.24);
    if (randomReal(0.0, 1.0) > chance) {
        return ServeFaultPlan::None;
    }

    return randomReal(0.0, 1.0) < 0.48 ? ServeFaultPlan::Net : ServeFaultPlan::Out;
}

Vec2 MatchController::predictBallLanding() const {
    if (!ball_.active) {
        return {p2_.pos.x, p2_.pos.y};
    }

    const double a = -0.5 * Gravity;
    const double b = ball_.vel.z;
    const double c = ball_.pos.z;
    const double discriminant = b * b - 4.0 * a * c;
    double t = 0.0;
    if (discriminant > 0.0) {
        const double root = std::sqrt(discriminant);
        const double t1 = (-b + root) / (2.0 * a);
        const double t2 = (-b - root) / (2.0 * a);
        t = std::max(t1, t2);
    }
    t = clampDouble(t, 0.0, 2.5);

    return {clampDouble(ball_.pos.x + ball_.vel.x * t, -CourtHalfWidth, CourtHalfWidth),
            clampDouble(ball_.pos.y + ball_.vel.y * t, -CourtHalfLength, CourtHalfLength)};
}

double MatchController::aiHitBonus() const {
    return profileFor(aiDifficulty_).hitBonus;
}

bool MatchController::isLegalServeBounce(const Vec3& pos) const {
    const bool insideWidth = std::abs(pos.x) <= CourtHalfWidth;
    if (ball_.expectedReceiver == 2) {
        return insideWidth && pos.y < -0.15 && pos.y >= -6.40;
    }
    if (ball_.expectedReceiver == 1) {
        return insideWidth && pos.y > 0.15 && pos.y <= 6.40;
    }
    return false;
}

void MatchController::registerServeFault(const QString& reason) {
    if (!serveInFlight_) {
        endPoint(opponentOf(ball_.lastHitPlayer), reason);
        return;
    }

    ball_.active = false;
    serveInFlight_ = false;
    plannedServeFault_ = ServeFaultPlan::None;
    ++serveFaults_;

    if (serveFaults_ >= 2) {
        endPoint(opponentOf(server_), QStringLiteral("Double fault"));
        return;
    }

    phase_ = MatchPhase::ServeReady;
    startServe(server_);
    feedback_ = QStringLiteral("%1 - second serve").arg(reason);
    feedbackTimer_ = 1.6;
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
    aiOpportunityWasActive_ = false;
    if (hitterId == 1) {
        aiPlannedLastHitPlayer_ = 0;
        aiPlannedBounceCount_ = -1;
    }
}

void MatchController::handleBounce() {
    const bool inBounds = std::abs(ball_.pos.x) <= CourtHalfWidth &&
                          std::abs(ball_.pos.y) <= CourtHalfLength;
    const bool correctSide = (ball_.expectedReceiver == 1 && ball_.pos.y > 0.0) ||
                             (ball_.expectedReceiver == 2 && ball_.pos.y < 0.0);

    if (ball_.bounceCount == 0) {
        if (serveInFlight_) {
            if (!isLegalServeBounce(ball_.pos)) {
                registerServeFault(QStringLiteral("Fault: out"));
                return;
            }

            serveInFlight_ = false;
            plannedServeFault_ = ServeFaultPlan::None;
            ball_.bounceCount = 1;
            ball_.vel.z = -ball_.vel.z * 0.54;
            ball_.vel.x *= 0.88;
            ball_.vel.y *= 0.88;
            feedback_ = QStringLiteral("Serve in");
            feedbackTimer_ = 0.45;
            return;
        }

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
    serveFaults_ = 0;
    serveInFlight_ = false;
    plannedServeFault_ = ServeFaultPlan::None;
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
    const CourtTheme& theme = courtThemeAt(courtTheme_);
    const double lineW = std::max(3.0, courtRect.width() / 120.0);
    const double thinLineW = std::max(1.4, lineW * 0.45);
    const double outerInset = lineW * 1.15;
    const double serviceTop = courtRect.top() + courtRect.height() * 0.231;
    const double serviceBottom = courtRect.top() + courtRect.height() * 0.769;
    const double netY = courtRect.center().y();

    QPainterPath shadowPath;
    shadowPath.addRoundedRect(courtRect.translated(0.0, 10.0), 14.0, 14.0);
    painter.fillPath(shadowPath, QColor(0, 0, 0, 76));

    QPainterPath clip;
    clip.addRoundedRect(courtRect, 12.0, 12.0);
    painter.setClipPath(clip);

    QLinearGradient base(courtRect.topLeft(), courtRect.bottomRight());
    base.setColorAt(0.0, theme.outerTop.lighter(112));
    base.setColorAt(0.55, theme.outerTop);
    base.setColorAt(1.0, theme.outerBottom);
    painter.fillPath(clip, base);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(255, 255, 255, 26));
    painter.drawRect(QRectF(courtRect.left(), courtRect.top(), courtRect.width(), courtRect.height() * 0.5));
    painter.setBrush(QColor(0, 0, 0, 20));
    painter.drawRect(QRectF(courtRect.left(), netY, courtRect.width(), courtRect.height() * 0.5));

    QLinearGradient serviceWash(0, serviceTop, 0, serviceBottom);
    serviceWash.setColorAt(0.0, theme.serviceTop);
    serviceWash.setColorAt(1.0, theme.serviceBottom);
    painter.fillRect(QRectF(courtRect.left(), serviceTop, courtRect.width(), serviceBottom - serviceTop),
                     serviceWash);

    painter.setBrush(QColor(255, 255, 255, 18));
    painter.drawRect(QRectF(courtRect.left() + courtRect.width() * 0.06,
                            courtRect.top() + courtRect.height() * 0.035,
                            courtRect.width() * 0.88,
                            courtRect.height() * 0.10));
    painter.setBrush(QColor(0, 0, 0, 16));
    painter.drawRect(QRectF(courtRect.left() + courtRect.width() * 0.06,
                            courtRect.bottom() - courtRect.height() * 0.135,
                            courtRect.width() * 0.88,
                            courtRect.height() * 0.10));

    painter.setBrush(theme.texture);
    for (int i = 0; i < 46; ++i) {
        const double x = courtRect.left() + courtRect.width() * ((i * 37) % 101) / 101.0;
        const double y = courtRect.top() + courtRect.height() * ((i * 53 + 17) % 103) / 103.0;
        const double rx = 1.2 + (i % 4) * 0.45;
        painter.drawEllipse(QPointF(x, y), rx, rx * 0.55);
    }

    const QRectF playRect = courtRect.adjusted(outerInset, outerInset, -outerInset, -outerInset);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(theme.lineShadow, lineW + thinLineW, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawRoundedRect(playRect.translated(1.0, 1.5), 7.0, 7.0);
    painter.drawLine(QPointF(playRect.left(), serviceTop + 1.5), QPointF(playRect.right(), serviceTop + 1.5));
    painter.drawLine(QPointF(playRect.left(), serviceBottom + 1.5), QPointF(playRect.right(), serviceBottom + 1.5));
    painter.drawLine(QPointF(playRect.center().x() + 1.0, serviceTop),
                     QPointF(playRect.center().x() + 1.0, serviceBottom));

    painter.setPen(QPen(theme.line, lineW, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.drawRoundedRect(playRect, 7.0, 7.0);
    painter.drawLine(QPointF(playRect.left(), serviceTop), QPointF(playRect.right(), serviceTop));
    painter.drawLine(QPointF(playRect.left(), serviceBottom), QPointF(playRect.right(), serviceBottom));
    painter.drawLine(QPointF(playRect.center().x(), serviceTop), QPointF(playRect.center().x(), serviceBottom));

    painter.setPen(QPen(theme.line, thinLineW, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(courtRect.left() + courtRect.width() * 0.09, courtRect.top() + outerInset),
                     QPointF(courtRect.left() + courtRect.width() * 0.09, courtRect.bottom() - outerInset));
    painter.drawLine(QPointF(courtRect.right() - courtRect.width() * 0.09, courtRect.top() + outerInset),
                     QPointF(courtRect.right() - courtRect.width() * 0.09, courtRect.bottom() - outerInset));

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 58));
    painter.drawRoundedRect(QRectF(playRect.left(), netY - lineW * 1.15,
                                   playRect.width(), lineW * 2.3),
                            lineW * 0.5, lineW * 0.5);
    painter.setBrush(theme.netBand);
    painter.drawRoundedRect(QRectF(playRect.left(), netY - lineW * 0.7,
                                   playRect.width(), lineW * 1.4),
                            lineW * 0.5, lineW * 0.5);
    painter.setPen(QPen(theme.netTop, thinLineW, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(playRect.left(), netY - lineW * 0.92),
                     QPointF(playRect.right(), netY - lineW * 0.92));
    painter.setPen(QPen(QColor(0, 0, 0, 76), thinLineW, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(playRect.left(), netY + lineW * 0.95),
                     QPointF(playRect.right(), netY + lineW * 0.95));

    painter.setBrush(theme.accent);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QPointF(playRect.left() + lineW * 1.4, netY), lineW * 0.8, lineW * 0.8);
    painter.drawEllipse(QPointF(playRect.right() - lineW * 1.4, netY), lineW * 0.8, lineW * 0.8);

    painter.setClipping(false);
    painter.setPen(QPen(QColor(255, 255, 255, 112), 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(courtRect.adjusted(1, 1, -1, -1), 12, 12);

    painter.restore();
}

void MatchController::drawPlayer(QPainter& painter, const Player& player, const QRectF& courtRect) const {
    painter.save();
    const QPointF feet = courtToScreen(player.pos.x, player.pos.y, 0.0, courtRect);
    const double scale = courtRect.height() / 720.0;
    const bool hitting = player.hitPoseTimer > 0.0;
    const QPixmap& sprite = playerSpritePixmap(player, hitting);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, hitting ? 82 : 70));
    painter.drawEllipse(QPointF(feet.x(), feet.y() + 8.0 * scale),
                        (hitting ? 43.0 : 35.0) * scale,
                        (hitting ? 12.0 : 10.0) * scale);

    if (!sprite.isNull()) {
        const double spriteHeight = (hitting ? 176.0 : 166.0) * scale;
        const double spriteWidth = spriteHeight * sprite.width() / std::max(1, sprite.height());
        const double anchorRatio = hitting ? 0.43 : 0.50;
        const QRectF target(feet.x() - spriteWidth * anchorRatio,
                            feet.y() - spriteHeight + 8.0 * scale,
                            spriteWidth,
                            spriteHeight);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.drawPixmap(target, sprite, QRectF(sprite.rect()));
    } else {
        painter.setBrush(player.outfit.shirtAccent);
        painter.drawRoundedRect(QRectF(feet.x() - 18.0 * scale, feet.y() - 78.0 * scale,
                                       36.0 * scale, 54.0 * scale),
                                8.0 * scale, 8.0 * scale);
        painter.setBrush(QColor(245, 202, 164));
        painter.drawEllipse(QPointF(feet.x(), feet.y() - 100.0 * scale),
                            23.0 * scale, 23.0 * scale);
    }

    painter.setFont(QFont(QStringLiteral("Segoe UI"), static_cast<int>(11 * scale), QFont::Bold));
    painter.setPen(QColor(245, 248, 250));
    painter.drawText(QRectF(feet.x() - 36.0 * scale, feet.y() + 14.0 * scale,
                            72.0 * scale, 18.0 * scale),
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

    const QString serveText = serveFaults_ > 0 ? QStringLiteral("2ND SERVE") : QStringLiteral("1ST SERVE");
    const double panelWidth = std::min(560.0, std::max(360.0, size.width() - 56.0));
    QRectF scorePanel((size.width() - panelWidth) * 0.5, 14.0, panelWidth, 78.0);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(20, 35, 42, 86));
    painter.drawRoundedRect(scorePanel.translated(0.0, 5.0), 8, 8);

    QLinearGradient panelFill(scorePanel.topLeft(), scorePanel.bottomRight());
    panelFill.setColorAt(0.0, QColor(255, 252, 231, 246));
    panelFill.setColorAt(1.0, QColor(255, 226, 190, 242));
    painter.setBrush(panelFill);
    painter.drawRoundedRect(scorePanel, 8, 8);
    painter.setPen(QPen(QColor(255, 144, 123, 210), 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(scorePanel.adjusted(1, 1, -1, -1), 8, 8);

    auto drawHudBall = [&](const QPointF& c, double r) {
        painter.setPen(QPen(QColor(255, 255, 246, 210), 1.2));
        painter.setBrush(QColor(220, 238, 64));
        painter.drawEllipse(c, r, r);
        painter.drawArc(QRectF(c.x() - r * 0.72, c.y() - r, r * 1.44, r * 2.0), 70 * 16, 118 * 16);
        painter.drawArc(QRectF(c.x() - r * 0.72, c.y() - r, r * 1.44, r * 2.0), 252 * 16, 118 * 16);
    };
    drawHudBall(QPointF(scorePanel.left() + 18.0, scorePanel.top() + 17.0), 7.0);
    drawHudBall(QPointF(scorePanel.right() - 18.0, scorePanel.bottom() - 17.0), 7.0);

    const double gap = 10.0;
    const double sideWidth = std::min(138.0, scorePanel.width() * 0.29);
    QRectF p1Box(scorePanel.left() + 24.0, scorePanel.top() + 16.0, sideWidth, 48.0);
    QRectF p2Box(scorePanel.right() - 24.0 - sideWidth, scorePanel.top() + 16.0, sideWidth, 48.0);
    QRectF centerBox(p1Box.right() + gap, scorePanel.top() + 16.0,
                     p2Box.left() - p1Box.right() - gap * 2.0, 48.0);

    auto drawScoreCard = [&](const QRectF& box, const QString& label, const QString& value, const QColor& accent) {
        QLinearGradient cardFill(box.topLeft(), box.bottomRight());
        cardFill.setColorAt(0.0, QColor(255, 255, 255, 248));
        cardFill.setColorAt(1.0, accent.lighter(174));
        painter.setPen(QPen(accent.darker(112), 1.6));
        painter.setBrush(cardFill);
        painter.drawRoundedRect(box, 8, 8);

        QRectF labelPill(box.left() + 8.0, box.top() + 8.0, 42.0, box.height() - 16.0);
        painter.setPen(Qt::NoPen);
        painter.setBrush(accent);
        painter.drawRoundedRect(labelPill, 8, 8);
        painter.setPen(QColor(255, 255, 248));
        painter.setFont(QFont(QStringLiteral("Segoe UI"), 10, QFont::Black));
        painter.drawText(labelPill, Qt::AlignCenter, label);

        painter.setPen(QColor(35, 45, 50));
        painter.setFont(QFont(QStringLiteral("Segoe UI"), 20, QFont::Black));
        painter.drawText(QRectF(labelPill.right() + 7.0, box.top() + 4.0,
                                box.right() - labelPill.right() - 12.0, box.height() - 8.0),
                         Qt::AlignCenter, value);
    };

    drawScoreCard(p1Box, QStringLiteral("P1"), score_.pointTextFor(1), QColor(255, 116, 122));
    drawScoreCard(p2Box, playMode_ == PlayMode::SinglePlayer ? QStringLiteral("CPU") : QStringLiteral("P2"),
                  score_.pointTextFor(2), QColor(64, 171, 165));

    painter.setPen(QPen(QColor(255, 183, 84, 220), 1.6));
    painter.setBrush(QColor(255, 248, 218, 245));
    painter.drawRoundedRect(centerBox, 8, 8);
    painter.setPen(QColor(70, 64, 54));
    painter.setFont(QFont(QStringLiteral("Segoe UI"), 12, QFont::Black));
    painter.drawText(QRectF(centerBox.left() + 6.0, centerBox.top() + 4.0,
                            centerBox.width() - 12.0, 20.0),
                     Qt::AlignCenter, score_.scoreNoteText());
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 9, QFont::DemiBold));
    painter.setPen(QColor(118, 91, 73));
    const QString meta = QStringLiteral("局数 %1 : %2  %3")
                             .arg(score_.gamesFor(1))
                             .arg(score_.gamesFor(2))
                             .arg(serveText);
    painter.drawText(QRectF(centerBox.left() + 6.0, centerBox.top() + 26.0,
                            centerBox.width() - 12.0, 17.0),
                     Qt::AlignCenter, meta);

    if (feedbackTimer_ > 0.0 || phase_ == MatchPhase::ServeReady || phase_ == MatchPhase::MatchOver) {
        QRectF box(size.width() / 2.0 - 205, size.height() - 88, 410, 48);
        painter.setPen(Qt::NoPen);
        QColor feedbackColor(11, 24, 31, 222);
        if (feedback_.contains(QStringLiteral("Fault")) ||
            feedback_.contains(QStringLiteral("Out")) ||
            feedback_.contains(QStringLiteral("Net"))) {
            feedbackColor = QColor(122, 45, 38, 232);
        }
        painter.setBrush(feedbackColor);
        painter.drawRoundedRect(box, 8, 8);
        painter.setBrush(QColor(214, 247, 91));
        painter.drawRoundedRect(QRectF(box.left() + 10, box.top() + 10, 5, box.height() - 20), 2, 2);
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
