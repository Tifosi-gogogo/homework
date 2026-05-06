#pragma once

#include "GameTypes.h"
#include "MatchController.h"

#include <QElapsedTimer>
#include <QTimer>
#include <QWidget>

#include <functional>

class GameWidget : public QWidget {
public:
    explicit GameWidget(QWidget* parent = nullptr);

    void startMatch(Gender p1Gender, Gender p2Gender);
    void setMatchFinishedHandler(std::function<void(int, const QString&)> handler);

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

    void tick();

private:
    void setKeyState(int key, bool down);
    void clearPressedFlags();

    QTimer frameTimer_;
    QElapsedTimer elapsedTimer_;
    InputState input_;
    MatchController match_;
    bool finishSignalSent_ = false;
    std::function<void(int, const QString&)> matchFinishedHandler_;
};
