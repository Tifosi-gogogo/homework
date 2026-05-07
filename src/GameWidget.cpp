#include "GameWidget.h"

#include <QKeyEvent>
#include <QPainter>

#include <utility>

GameWidget::GameWidget(QWidget* parent)
    : QWidget(parent) {
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(1000, 680);

    elapsedTimer_.start();
    connect(&frameTimer_, &QTimer::timeout, this, &GameWidget::tick);
    frameTimer_.start(16);
}

void GameWidget::startMatch(const OutfitItem& p1Outfit, const RacketItem& p1Racket,
                            const OutfitItem& p2Outfit, const RacketItem& p2Racket,
                            PlayMode playMode, AiDifficulty aiDifficulty) {
    match_.reset(p1Outfit, p1Racket, p2Outfit, p2Racket, playMode, aiDifficulty);
    finishSignalSent_ = false;
    input_ = InputState{};
    elapsedTimer_.restart();
    setFocus();
    update();
}

void GameWidget::setMatchFinishedHandler(std::function<void(int, const QString&)> handler) {
    matchFinishedHandler_ = std::move(handler);
}

void GameWidget::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    match_.draw(painter, size());
}

void GameWidget::keyPressEvent(QKeyEvent* event) {
    if (event->isAutoRepeat()) {
        QWidget::keyPressEvent(event);
        return;
    }
    setKeyState(event->key(), true);
}

void GameWidget::keyReleaseEvent(QKeyEvent* event) {
    if (event->isAutoRepeat()) {
        QWidget::keyReleaseEvent(event);
        return;
    }
    setKeyState(event->key(), false);
}

void GameWidget::tick() {
    const double dt = elapsedTimer_.restart() / 1000.0;
    match_.update(input_, dt);
    clearPressedFlags();

    if (match_.isMatchOver() && !finishSignalSent_) {
        finishSignalSent_ = true;
        if (matchFinishedHandler_) {
            matchFinishedHandler_(match_.matchWinner(), match_.matchSummary());
        }
    }

    update();
}

void GameWidget::setKeyState(int key, bool down) {
    switch (key) {
    case Qt::Key_W:
        input_.p1Up = down;
        break;
    case Qt::Key_S:
        input_.p1Down = down;
        break;
    case Qt::Key_A:
        input_.p1Left = down;
        break;
    case Qt::Key_D:
        input_.p1Right = down;
        break;
    case Qt::Key_Up:
        input_.p2Up = down;
        break;
    case Qt::Key_Down:
        input_.p2Down = down;
        break;
    case Qt::Key_Left:
        input_.p2Left = down;
        break;
    case Qt::Key_Right:
        input_.p2Right = down;
        break;
    case Qt::Key_Space:
        if (down) {
            input_.p1HitPressed = true;
        }
        break;
    case Qt::Key_J:
        if (down) {
            input_.p2HitPressed = true;
        }
        break;
    case Qt::Key_Escape:
        if (down) {
            input_.pausePressed = true;
        }
        break;
    default:
        break;
    }
}

void GameWidget::clearPressedFlags() {
    input_.p1HitPressed = false;
    input_.p2HitPressed = false;
    input_.pausePressed = false;
}
