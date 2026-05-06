#include "MainWindow.h"

#include <QApplication>
#include <QAudioOutput>
#include <QFrame>
#include <QHBoxLayout>
#include <QMediaPlayer>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QPixmap>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QUrl>
#include <QVariant>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

namespace {

QLabel* makeTitle(const QString& text, int size = 34) {
    auto* label = new QLabel(text);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet(QStringLiteral("font-size:%1px;font-weight:800;color:#f7fbfb;").arg(size));
    return label;
}

QLabel* makeBody(const QString& text) {
    auto* label = new QLabel(text);
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet(QStringLiteral("font-size:16px;line-height:150%;color:#dbe6e8;"));
    return label;
}

QPushButton* makeButton(const QString& text) {
    auto* button = new QPushButton(text);
    button->setMinimumHeight(46);
    button->setCursor(Qt::PointingHandCursor);
    button->setStyleSheet(QStringLiteral(
        "QPushButton{"
        "background:#f2f7f4;color:#14262d;border:0;border-radius:6px;"
        "font-size:17px;font-weight:700;padding:9px 22px;}"
        "QPushButton:hover{background:#dff2e8;}"
        "QPushButton:pressed{background:#b9ded0;}"));
    return button;
}

QWidget* makePanel() {
    auto* panel = new QFrame;
    panel->setObjectName(QStringLiteral("panel"));
    panel->setStyleSheet(QStringLiteral(
        "#panel{background:rgba(9,24,30,215);border:1px solid rgba(255,255,255,45);border-radius:10px;}"));
    return panel;
}

void drawOutlinedText(QPainter& painter, const QRectF& rect, const QString& text, const QFont& font,
                      const QColor& fill, const QColor& outline, int flags, int outlineWidth = 3) {
    painter.setFont(font);
    painter.setPen(outline);
    for (int dx = -outlineWidth; dx <= outlineWidth; ++dx) {
        for (int dy = -outlineWidth; dy <= outlineWidth; ++dy) {
            if (dx == 0 && dy == 0) {
                continue;
            }
            painter.drawText(rect.translated(dx, dy), flags, text);
        }
    }
    painter.setPen(fill);
    painter.drawText(rect, flags, text);
}

void drawTennisBall(QPainter& painter, const QPointF& center, double radius) {
    QRadialGradient ball(center.x() - radius * 0.35, center.y() - radius * 0.45, radius * 1.25);
    ball.setColorAt(0.0, QColor(246, 255, 112));
    ball.setColorAt(0.65, QColor(196, 224, 38));
    ball.setColorAt(1.0, QColor(84, 112, 27));
    painter.setPen(QPen(QColor(48, 69, 24), radius * 0.12));
    painter.setBrush(ball);
    painter.drawEllipse(center, radius, radius);
    painter.setPen(QPen(QColor(248, 255, 215), radius * 0.12, Qt::SolidLine, Qt::RoundCap));
    painter.drawArc(QRectF(center.x() - radius * 0.78, center.y() - radius * 0.95,
                           radius * 1.06, radius * 1.9),
                    -72 * 16, 134 * 16);
    painter.drawArc(QRectF(center.x() - radius * 0.28, center.y() - radius * 0.95,
                           radius * 1.06, radius * 1.9),
                    118 * 16, 134 * 16);
}

void drawRacket(QPainter& painter, const QPointF& grip, double scale, const RacketItem& racket, bool flip) {
    painter.save();
    painter.translate(grip);
    painter.scale(flip ? -scale : scale, scale);
    painter.rotate(-24);

    painter.setPen(QPen(racket.gripColor, 8, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(0, 0), QPointF(24, -72));

    painter.setPen(QPen(racket.frameColor, 8));
    painter.setBrush(QColor(255, 255, 255, 36));
    painter.drawEllipse(QPointF(36, -102), 31, 45);

    painter.setPen(QPen(racket.accentColor, 4));
    painter.drawArc(QRectF(7, -143, 58, 82), 40 * 16, 120 * 16);

    painter.setPen(QPen(racket.stringColor, 1.4));
    for (int i = -3; i <= 3; ++i) {
        painter.drawLine(QPointF(36 + i * 7, -135), QPointF(36 + i * 7, -70));
        painter.drawLine(QPointF(12, -102 + i * 8), QPointF(60, -102 + i * 8));
    }

    painter.restore();
}

bool drawRacketImage(QPainter& painter, const QRectF& rect, const RacketItem& racket, bool mirrored = false) {
    if (racket.imagePath.isEmpty()) {
        return false;
    }

    const QPixmap pixmap(racket.imagePath);
    if (pixmap.isNull()) {
        return false;
    }

    QSizeF targetSize = pixmap.size();
    targetSize.scale(rect.size(), Qt::KeepAspectRatio);
    const QRectF target(QPointF(rect.center().x() - targetSize.width() / 2.0,
                                rect.center().y() - targetSize.height() / 2.0),
                        targetSize);

    painter.save();
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    if (mirrored) {
        painter.translate(target.center());
        painter.scale(-1.0, 1.0);
        painter.translate(-target.center());
    }
    painter.drawPixmap(target, pixmap, QRectF(pixmap.rect()));
    painter.restore();
    return true;
}

void drawRacketIcon(QPainter& painter, const QRectF& rect, const RacketItem& racket) {
    if (drawRacketImage(painter, rect, racket)) {
        return;
    }

    painter.save();
    painter.translate(rect.center());
    const double scale = std::min(rect.width() / 130.0, rect.height() / 150.0);
    painter.scale(scale, scale);
    painter.rotate(-24);

    painter.setPen(QPen(racket.gripColor, 8, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(-8, 48), QPointF(18, -26));

    painter.setPen(QPen(racket.frameColor, 8));
    painter.setBrush(QColor(255, 255, 255, 38));
    painter.drawEllipse(QPointF(28, -58), 31, 43);

    painter.setPen(QPen(racket.accentColor, 4));
    painter.drawArc(QRectF(0, -97, 56, 78), 40 * 16, 120 * 16);

    painter.setPen(QPen(racket.stringColor, 1.35));
    for (int i = -3; i <= 3; ++i) {
        painter.drawLine(QPointF(28 + i * 7, -89), QPointF(28 + i * 7, -29));
        painter.drawLine(QPointF(5, -58 + i * 8), QPointF(51, -58 + i * 8));
    }
    painter.restore();
}

bool drawOutfitImage(QPainter& painter, const QRectF& rect, const OutfitItem& outfit, bool mirrored = false) {
    if (outfit.imagePath.isEmpty()) {
        return false;
    }

    const QPixmap pixmap(outfit.imagePath);
    if (pixmap.isNull()) {
        return false;
    }

    QSizeF targetSize = pixmap.size();
    targetSize.scale(rect.size(), Qt::KeepAspectRatio);
    QRectF target(QPointF(rect.center().x() - targetSize.width() / 2.0,
                         rect.center().y() - targetSize.height() / 2.0),
                  targetSize);

    painter.save();
    if (mirrored) {
        painter.translate(target.center());
        painter.scale(-1.0, 1.0);
        painter.translate(-target.center());
    }
    painter.drawPixmap(target, pixmap, QRectF(pixmap.rect()));
    painter.restore();
    return true;
}

void drawOutfitIcon(QPainter& painter, const QRectF& rect, const OutfitItem& outfit) {
    if (drawOutfitImage(painter, rect, outfit)) {
        return;
    }

    painter.save();
    const double scale = std::min(rect.width() / 170.0, rect.height() / 225.0);
    painter.translate(rect.center().x(), rect.bottom() - 8);
    painter.scale(scale, scale);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 45));
    painter.drawEllipse(QPointF(0, 2), 48, 9);

    painter.setPen(QPen(QColor(224, 171, 142), 10, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(-17, -48), QPointF(-27, -4));
    painter.drawLine(QPointF(17, -48), QPointF(29, -4));
    painter.setPen(QPen(outfit.shoeColor.darker(114), 9, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(-34, 0), QPointF(-8, 0));
    painter.drawLine(QPointF(13, 0), QPointF(39, 0));

    painter.setPen(Qt::NoPen);
    if (outfit.gender == Gender::Female) {
        QPolygonF skirt;
        skirt << QPointF(-34, -80) << QPointF(34, -80) << QPointF(44, -49) << QPointF(-42, -49);
        painter.setBrush(outfit.bottomMain);
        painter.drawPolygon(skirt);
        painter.setPen(QPen(outfit.bottomAccent, 3));
        painter.drawLine(QPointF(-18, -77), QPointF(-25, -52));
        painter.drawLine(QPointF(0, -77), QPointF(0, -52));
        painter.drawLine(QPointF(18, -77), QPointF(25, -52));
        painter.setPen(Qt::NoPen);
    } else {
        painter.setBrush(outfit.bottomMain);
        painter.drawRoundedRect(QRectF(-32, -80, 28, 34), 7, 7);
        painter.drawRoundedRect(QRectF(4, -80, 28, 34), 7, 7);
    }

    painter.setBrush(outfit.shirtMain);
    painter.drawRoundedRect(QRectF(-38, -142, 76, 66), 16, 16);
    painter.setBrush(outfit.shirtAccent);
    painter.drawRoundedRect(QRectF(-8, -139, 16, 42), 8, 8);
    painter.setBrush(outfit.collarColor);
    QPolygonF collar;
    collar << QPointF(-16, -142) << QPointF(0, -124) << QPointF(16, -142);
    painter.drawPolygon(collar);

    painter.setPen(QPen(QColor(224, 171, 142), 10, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(-36, -126), QPointF(-57, -89));
    painter.drawLine(QPointF(36, -126), QPointF(57, -89));

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(224, 171, 142));
    painter.drawRoundedRect(QRectF(-11, -159, 22, 22), 8, 8);
    painter.drawEllipse(QPointF(0, -193), 44, 42);

    painter.setBrush(outfit.hairColor);
    painter.drawPie(QRectF(-44, -231, 88, 60), 0, 180 * 16);
    if (outfit.gender == Gender::Female) {
        painter.drawEllipse(QPointF(36, -198), 15, 19);
        painter.drawEllipse(QPointF(48, -184), 12, 16);
    }

    painter.setBrush(outfit.hatColor);
    painter.drawPie(QRectF(-49, -235, 98, 44), 0, 180 * 16);
    painter.setBrush(outfit.shirtAccent);
    painter.drawRoundedRect(QRectF(-6, -224, 52, 9), 5, 5);

    painter.setBrush(QColor(28, 33, 38));
    painter.drawEllipse(QPointF(-14, -193), 4, 4);
    painter.drawEllipse(QPointF(14, -193), 4, 4);
    painter.setPen(QPen(QColor(119, 66, 58), 2.5, Qt::SolidLine, Qt::RoundCap));
    painter.drawArc(QRectF(-13, -183, 26, 14), 205 * 16, 130 * 16);

    painter.restore();
}

void drawArcadePlayer(QPainter& painter, const QPointF& feet, double scale,
                      const OutfitItem& outfit, const RacketItem& racket, bool facingRight) {
    if (!outfit.imagePath.isEmpty()) {
        painter.save();
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 70));
        painter.drawEllipse(QPointF(feet.x(), feet.y() + 10 * scale), 64 * scale, 14 * scale);
        drawOutfitImage(painter, QRectF(feet.x() - 160 * scale, feet.y() - 315 * scale,
                                        320 * scale, 320 * scale),
                        outfit, !facingRight);
        painter.restore();
        return;
    }

    painter.save();
    painter.translate(feet);
    painter.scale(facingRight ? scale : -scale, scale);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 76));
    painter.drawEllipse(QPointF(0, 10), 62, 13);

    drawRacket(painter, QPointF(-62, -80), 0.82, racket, true);

    painter.setPen(QPen(QColor(224, 171, 142), 12, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(-18, -54), QPointF(-30, -4));
    painter.drawLine(QPointF(20, -54), QPointF(34, -4));
    painter.setPen(QPen(outfit.shoeColor.darker(118), 11, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(-38, 0), QPointF(-5, 0));
    painter.drawLine(QPointF(14, 0), QPointF(48, 0));
    painter.setPen(QPen(QColor(214, 103, 42), 3, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(-31, -5), QPointF(-14, -5));
    painter.drawLine(QPointF(22, -5), QPointF(39, -5));

    painter.setPen(Qt::NoPen);
    if (outfit.gender == Gender::Female) {
        QPolygonF skirt;
        skirt << QPointF(-40, -88) << QPointF(40, -88) << QPointF(52, -52) << QPointF(-48, -52);
        painter.setBrush(outfit.bottomMain);
        painter.drawPolygon(skirt);
        painter.setPen(QPen(outfit.bottomAccent, 3));
        painter.drawLine(QPointF(-23, -84), QPointF(-31, -56));
        painter.drawLine(QPointF(0, -84), QPointF(0, -55));
        painter.drawLine(QPointF(23, -84), QPointF(31, -56));
        painter.setPen(Qt::NoPen);
    } else {
        painter.setBrush(outfit.bottomMain);
        painter.drawRoundedRect(QRectF(-38, -88, 32, 38), 8, 8);
        painter.drawRoundedRect(QRectF(6, -88, 32, 38), 8, 8);
    }

    painter.setBrush(outfit.shirtMain);
    painter.drawRoundedRect(QRectF(-43, -158, 86, 78), 18, 18);
    painter.setBrush(outfit.shirtAccent);
    painter.drawRoundedRect(QRectF(-9, -154, 18, 50), 9, 9);
    painter.setBrush(outfit.collarColor);
    QPolygonF collar;
    collar << QPointF(-18, -157) << QPointF(0, -136) << QPointF(18, -157);
    painter.drawPolygon(collar);

    painter.setPen(QPen(QColor(224, 171, 142), 12, Qt::SolidLine, Qt::RoundCap));
    painter.drawLine(QPointF(-40, -138), QPointF(-68, -88));
    painter.drawLine(QPointF(40, -138), QPointF(66, -92));
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(224, 171, 142));
    painter.drawEllipse(QPointF(-70, -84), 13, 12);
    painter.drawEllipse(QPointF(68, -89), 13, 12);

    painter.setBrush(QColor(224, 171, 142));
    painter.drawRoundedRect(QRectF(-13, -178, 26, 24), 8, 8);
    painter.drawEllipse(QPointF(0, -219), 52, 48);

    painter.setBrush(outfit.hairColor);
    painter.drawPie(QRectF(-52, -261, 104, 70), 0, 180 * 16);
    painter.drawEllipse(QPointF(-39, -207), 10, 17);
    if (outfit.gender == Gender::Female) {
        painter.drawEllipse(QPointF(39, -225), 17, 22);
        painter.drawEllipse(QPointF(53, -210), 14, 18);
    }

    painter.setBrush(outfit.hatColor);
    painter.drawPie(QRectF(-58, -266, 116, 50), 0, 180 * 16);
    painter.setBrush(outfit.shirtAccent);
    painter.drawRoundedRect(QRectF(-8, -253, 60, 10), 5, 5);

    painter.setBrush(QColor(28, 33, 38));
    painter.drawEllipse(QPointF(-17, -218), 5, 5);
    painter.drawEllipse(QPointF(17, -218), 5, 5);
    painter.setPen(QPen(QColor(119, 66, 58), 3, Qt::SolidLine, Qt::RoundCap));
    painter.drawArc(QRectF(-16, -207, 32, 17), 205 * 16, 130 * 16);

    painter.restore();
}

void drawArcadeLogo(QPainter& painter, const QRectF& rect, const QString& bottomText) {
    const QRectF tennisRect(rect.left(), rect.top(), rect.width(), rect.height() * 0.46);
    const QRectF duelRect(rect.left() + rect.width() * 0.18, rect.top() + rect.height() * 0.36,
                          rect.width() * 0.78, rect.height() * 0.42);

    drawOutlinedText(painter, tennisRect, QStringLiteral("TENNIS"),
                     QFont(QStringLiteral("Arial Black"), static_cast<int>(rect.height() * 0.30), QFont::Black),
                     QColor(255, 205, 61), QColor(63, 73, 178), Qt::AlignCenter, 5);
    drawOutlinedText(painter, duelRect, QStringLiteral("DUEL"),
                     QFont(QStringLiteral("Arial Black"), static_cast<int>(rect.height() * 0.25), QFont::Black),
                     QColor(247, 248, 255), QColor(91, 84, 184), Qt::AlignCenter, 5);

    drawTennisBall(painter, QPointF(rect.left() + rect.width() * 0.18, rect.top() + rect.height() * 0.56),
                   rect.height() * 0.14);

    painter.setPen(QPen(QColor(83, 61, 173), 4));
    painter.setBrush(QColor(123, 80, 207));
    QPolygonF ribbon;
    ribbon << QPointF(rect.left() + rect.width() * 0.21, rect.bottom() - rect.height() * 0.17)
           << QPointF(rect.right() - rect.width() * 0.08, rect.bottom() - rect.height() * 0.17)
           << QPointF(rect.right() - rect.width() * 0.04, rect.bottom() - rect.height() * 0.02)
           << QPointF(rect.left() + rect.width() * 0.17, rect.bottom() - rect.height() * 0.02);
    painter.drawPolygon(ribbon);
    drawOutlinedText(painter, QRectF(rect.left() + rect.width() * 0.20, rect.bottom() - rect.height() * 0.19,
                                     rect.width() * 0.76, rect.height() * 0.18),
                     bottomText, QFont(QStringLiteral("Arial Black"), static_cast<int>(rect.height() * 0.12), QFont::Black),
                     QColor(255, 214, 48), QColor(30, 20, 48), Qt::AlignCenter, 2);

    painter.setPen(QPen(QColor(83, 92, 191), 5));
    painter.setBrush(QColor(202, 236, 247));
    const QRectF cup(rect.right() - rect.width() * 0.24, rect.top() + rect.height() * 0.01,
                    rect.width() * 0.15, rect.height() * 0.28);
    painter.drawRoundedRect(cup, 12, 12);
    painter.drawLine(QPointF(cup.center().x(), cup.bottom()), QPointF(cup.center().x(), cup.bottom() + rect.height() * 0.09));
    painter.drawLine(QPointF(cup.center().x() - rect.width() * 0.05, cup.bottom() + rect.height() * 0.09),
                     QPointF(cup.center().x() + rect.width() * 0.05, cup.bottom() + rect.height() * 0.09));
}

void drawMenuLogoImage(QPainter& painter, const QRectF& rect) {
    const QPixmap logo(QStringLiteral(":/assets/menu_logo.png"));
    if (logo.isNull()) {
        drawArcadeLogo(painter, rect, QStringLiteral("2026"));
        return;
    }

    QSizeF targetSize = logo.size();
    targetSize.scale(rect.size(), Qt::KeepAspectRatio);
    const QRectF target(QPointF(rect.center().x() - targetSize.width() / 2.0,
                               rect.center().y() - targetSize.height() / 2.0),
                        targetSize);

    painter.save();
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.drawPixmap(target, logo, QRectF(logo.rect()));
    painter.restore();
}

void drawCrowd(QPainter& painter, const QRectF& area) {
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(86, 83, 90, 230));
    for (int row = 0; row < 5; ++row) {
        const double y = area.top() + row * area.height() / 5.0;
        painter.setBrush(row % 2 == 0 ? QColor(70, 66, 71, 230) : QColor(82, 78, 82, 230));
        painter.drawRect(QRectF(area.left(), y, area.width(), area.height() / 7.0));
    }

    const QVector<QColor> shirts = {
        QColor(72, 160, 201), QColor(116, 92, 171), QColor(118, 77, 54),
        QColor(55, 142, 91), QColor(198, 103, 53), QColor(42, 48, 70)
    };
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 18; ++col) {
            if ((row + col) % 5 == 0) {
                continue;
            }
            const double x = area.left() + 28 + col * area.width() / 18.5 + (row % 2) * 10;
            const double y = area.top() + 30 + row * area.height() / 4.5;
            const double s = 0.84 + ((row + col) % 3) * 0.08;
            painter.setBrush(QColor(45, 36, 31, 72));
            painter.drawEllipse(QPointF(x, y + 35 * s), 15 * s, 6 * s);
            painter.setBrush(shirts[(row + col) % shirts.size()]);
            painter.drawRoundedRect(QRectF(x - 14 * s, y + 14 * s, 28 * s, 28 * s), 7, 7);
            painter.setBrush(QColor(203 + (col % 3) * 16, 151 + (row % 2) * 18, 116));
            painter.drawEllipse(QPointF(x, y), 15 * s, 15 * s);
            painter.setBrush((row + col) % 4 == 0 ? QColor(42, 30, 26) : QColor(93, 55, 33));
            painter.drawPie(QRectF(x - 15 * s, y - 15 * s, 30 * s, 22 * s), 0, 180 * 16);
            painter.setBrush(QColor(26, 31, 34));
            painter.drawEllipse(QPointF(x - 5 * s, y - 1 * s), 2.2 * s, 2.2 * s);
            painter.drawEllipse(QPointF(x + 5 * s, y - 1 * s), 2.2 * s, 2.2 * s);
            painter.setPen(QPen(QColor(80, 38, 32), 1.6 * s, Qt::SolidLine, Qt::RoundCap));
            painter.drawArc(QRectF(x - 7 * s, y + 4 * s, 14 * s, 8 * s), 200 * 16, 140 * 16);
            painter.setPen(Qt::NoPen);
        }
    }
}

QPushButton* makeArcadeButton(const QString& text, const QColor& bg, const QColor& hover, const QColor& border) {
    auto* button = new QPushButton(text);
    button->setMinimumHeight(76);
    button->setCursor(Qt::PointingHandCursor);
    button->setStyleSheet(QStringLiteral(
                              "QPushButton{background:%1;color:#ffffff;border:4px solid %2;border-radius:9px;"
                              "font-size:30px;font-weight:900;padding:10px 22px;}"
                              "QPushButton:hover{background:%3;}"
                              "QPushButton:pressed{background:%4;}")
                              .arg(bg.name(), border.name(), hover.name(), bg.darker(116).name()));
    return button;
}

class MenuOptionButton : public QPushButton {
public:
    MenuOptionButton(const QString& text, const QColor& base, const QColor& hover, QWidget* parent = nullptr)
        : QPushButton(text, parent),
          base_(base),
          hover_(hover) {
        setCursor(Qt::PointingHandCursor);
        setMinimumHeight(76);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setStyleSheet(QStringLiteral("QPushButton{background:transparent;border:0;}"));
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const bool pressed = isDown();
        const QColor fill = pressed ? base_.darker(112) : (underMouse() ? hover_ : base_);
        QRectF body(4, 3, width() - 10, height() - 13);
        if (pressed) {
            body.translate(0, 4);
        }
        const double radius = body.height() / 2.0;

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(23, 46, 71, 78));
        painter.drawRoundedRect(body.translated(0, 8), radius, radius);

        QLinearGradient gradient(body.topLeft(), body.bottomLeft());
        gradient.setColorAt(0.0, fill.lighter(130));
        gradient.setColorAt(0.48, fill);
        gradient.setColorAt(1.0, fill.darker(118));
        painter.setBrush(gradient);
        painter.drawRoundedRect(body, radius, radius);

        painter.setPen(QPen(QColor(255, 255, 255, 115), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(body.adjusted(2, 2, -2, -2), radius - 2, radius - 2);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 255, 120));
        painter.drawEllipse(QRectF(body.left() + 24, body.top() + 13,
                                   body.width() * 0.08, body.height() * 0.27));

        QFont font(QStringLiteral("Microsoft YaHei UI"), std::max(21, static_cast<int>(body.height() * 0.40)), QFont::Black);
        painter.setFont(font);
        const QRectF textRect = body.adjusted(20, 0, -20, -2);
        painter.setPen(QColor(120, 82, 64, 125));
        painter.drawText(textRect.translated(0, 4), Qt::AlignCenter, text());
        painter.setPen(QColor(255, 255, 255));
        painter.drawText(textRect, Qt::AlignCenter, text());
    }

private:
    QColor base_;
    QColor hover_;
};

class MenuImageButton : public QPushButton {
public:
    MenuImageButton(const QString& text, const QString& imagePath, QWidget* parent = nullptr)
        : QPushButton(text, parent),
          pixmap_(imagePath) {
        setCursor(Qt::PointingHandCursor);
        setMinimumHeight(82);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setStyleSheet(QStringLiteral("QPushButton{background:transparent;border:0;}"));
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        if (pixmap_.isNull()) {
            QPushButton::paintEvent(event);
            return;
        }

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        QRectF target(0, isDown() ? 3 : 0, width(), height() - 4);
        if (underMouse() && !isDown()) {
            target = target.adjusted(-2, -1, 2, 1);
        }
        painter.drawPixmap(target, pixmap_, QRectF(pixmap_.rect()));
    }

private:
    QPixmap pixmap_;
};

QPushButton* makeMenuOptionButton(const QString& text, const QColor& base, const QColor& hover) {
    return new MenuOptionButton(text, base, hover);
}

QPushButton* makeMenuImageButton(const QString& text, const QString& imagePath) {
    return new MenuImageButton(text, imagePath);
}

QPushButton* makeSelectBackButton() {
    auto* button = new QPushButton(QStringLiteral("返回"));
    button->setFixedSize(96, 54);
    button->setCursor(Qt::PointingHandCursor);
    button->setStyleSheet(QStringLiteral(
        "QPushButton{background:#ff9f08;color:#ffffff;border:3px solid #b95d00;border-radius:0;"
        "font-size:20px;font-weight:900;}"
        "QPushButton:hover{background:#ffb130;}"
        "QPushButton:pressed{background:#d97900;}"));
    return button;
}

void styleSelectCombo(QComboBox* combo) {
    if (!combo) {
        return;
    }
    combo->setMinimumHeight(38);
    combo->setStyleSheet(QStringLiteral(
        "QComboBox{background:#ffffff;color:#162538;border:3px solid #253050;border-radius:8px;"
        "font-size:14px;font-weight:800;padding:5px 12px;}"
        "QComboBox::drop-down{width:28px;border:0;}"
        "QComboBox QAbstractItemView{background:#ffffff;color:#162538;selection-background-color:#d7f75b;}"));
}

class SoundToggleButton : public QPushButton {
public:
    explicit SoundToggleButton(QWidget* parent = nullptr)
        : QPushButton(parent) {
        setCheckable(true);
        setChecked(true);
        setFixedSize(86, 68);
        setCursor(Qt::PointingHandCursor);
        setToolTip(QStringLiteral("声音开关"));
        setStyleSheet(QStringLiteral("QPushButton{background:transparent;border:0;}"));
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const bool enabled = isChecked();
        QRectF box(4, 4, width() - 8, height() - 8);
        painter.setPen(QPen(QColor(30, 57, 125), 3));
        painter.setBrush(enabled ? QColor(42, 147, 239) : QColor(116, 123, 135));
        painter.drawRoundedRect(box, 5, 5);

        painter.setPen(QPen(QColor(18, 25, 31), 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(QColor(255, 255, 255));
        QPolygonF speaker;
        speaker << QPointF(20, 37) << QPointF(33, 26) << QPointF(45, 26)
                << QPointF(45, 50) << QPointF(33, 50) << QPointF(20, 39);
        painter.drawPolygon(speaker);
        painter.drawRect(QRectF(13, 30, 10, 18));

        painter.setBrush(Qt::NoBrush);
        if (enabled) {
            painter.setPen(QPen(QColor(255, 255, 255), 4, Qt::SolidLine, Qt::RoundCap));
            painter.drawArc(QRectF(44, 27, 20, 22), -55 * 16, 110 * 16);
            painter.drawArc(QRectF(50, 21, 30, 34), -55 * 16, 110 * 16);
            painter.setPen(QPen(QColor(18, 25, 31), 2, Qt::SolidLine, Qt::RoundCap));
            painter.drawArc(QRectF(44, 27, 20, 22), -55 * 16, 110 * 16);
            painter.drawArc(QRectF(50, 21, 30, 34), -55 * 16, 110 * 16);
        } else {
            painter.setPen(QPen(QColor(255, 255, 255), 7, Qt::SolidLine, Qt::RoundCap));
            painter.drawLine(QPointF(57, 27), QPointF(76, 49));
            painter.drawLine(QPointF(76, 27), QPointF(57, 49));
            painter.setPen(QPen(QColor(153, 30, 38), 4, Qt::SolidLine, Qt::RoundCap));
            painter.drawLine(QPointF(57, 27), QPointF(76, 49));
            painter.drawLine(QPointF(76, 27), QPointF(57, 49));
        }
    }
};

class OutfitChoiceWidget : public QWidget {
public:
    explicit OutfitChoiceWidget(const QString& title, QWidget* parent = nullptr)
        : QWidget(parent),
          title_(title) {
        setMinimumSize(280, 186);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setCursor(Qt::PointingHandCursor);
    }

    void setProviders(std::function<std::vector<int>()> listProvider,
                      std::function<int()> currentProvider,
                      std::function<void(int)> selectHandler) {
        listProvider_ = std::move(listProvider);
        currentProvider_ = std::move(currentProvider);
        selectHandler_ = std::move(selectHandler);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        QRectF card(5, 5, width() - 10, height() - 10);
        painter.setPen(QPen(QColor(31, 42, 65), 3));
        painter.setBrush(QColor(255, 255, 255, 228));
        painter.drawRoundedRect(card, 10, 10);

        painter.setPen(QColor(37, 48, 80));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 14, QFont::Black));
        painter.drawText(card.adjusted(16, 8, -16, -8), Qt::AlignTop | Qt::AlignHCenter, title_);

        const OutfitItem& outfit = outfitByCatalogIndex(currentCatalogIndex());
        const QRectF outfitRect = outfit.imagePath.isEmpty()
                                      ? QRectF(card.center().x() - 55, card.top() + 34, 110, 104)
                                      : QRectF(card.center().x() - 70, card.top() + 30, 140, 110);
        drawOutfitIcon(painter, outfitRect, outfit);

        painter.setPen(QPen(QColor(246, 202, 46), 6, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(QColor(255, 224, 72));
        leftArrow_ = QRectF(card.left() + 14, card.top() + 64, 48, 46);
        rightArrow_ = QRectF(card.right() - 62, card.top() + 64, 48, 46);
        drawArrow(painter, leftArrow_, false);
        drawArrow(painter, rightArrow_, true);

        painter.setPen(QColor(27, 37, 55));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 11, QFont::Black));
        painter.drawText(QRectF(card.left() + 14, card.bottom() - 42, card.width() - 28, 20),
                         Qt::AlignCenter, outfit.name);
        painter.setPen(QColor(85, 95, 111));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 9, QFont::DemiBold));
        painter.drawText(QRectF(card.left() + 18, card.bottom() - 22, card.width() - 36, 16),
                         Qt::AlignCenter, outfit.gender == Gender::Female ? QStringLiteral("女生服装") : QStringLiteral("男生服装"));
    }

    void mousePressEvent(QMouseEvent* event) override {
        const auto list = listProvider_ ? listProvider_() : std::vector<int>{};
        if (list.empty() || !selectHandler_) {
            return;
        }

        int direction = 0;
        if (leftArrow_.contains(event->position())) {
            direction = -1;
        } else if (rightArrow_.contains(event->position())) {
            direction = 1;
        } else {
            return;
        }

        const int current = currentCatalogIndex();
        auto it = std::find(list.begin(), list.end(), current);
        int index = it == list.end() ? 0 : static_cast<int>(std::distance(list.begin(), it));
        index = (index + direction + static_cast<int>(list.size())) % static_cast<int>(list.size());
        selectHandler_(list[index]);
        update();
    }

private:
    int currentCatalogIndex() const {
        return currentProvider_ ? currentProvider_() : 0;
    }

    static void drawArrow(QPainter& painter, const QRectF& rect, bool right) {
        const double midY = rect.center().y();
        QPolygonF arrow;
        if (right) {
            arrow << QPointF(rect.left() + 6, rect.top() + 11)
                  << QPointF(rect.right() - 6, midY)
                  << QPointF(rect.left() + 6, rect.bottom() - 11)
                  << QPointF(rect.left() + 14, midY);
        } else {
            arrow << QPointF(rect.right() - 6, rect.top() + 11)
                  << QPointF(rect.left() + 6, midY)
                  << QPointF(rect.right() - 6, rect.bottom() - 11)
                  << QPointF(rect.right() - 14, midY);
        }
        painter.drawPolygon(arrow);
    }

    QString title_;
    QRectF leftArrow_;
    QRectF rightArrow_;
    std::function<std::vector<int>()> listProvider_;
    std::function<int()> currentProvider_;
    std::function<void(int)> selectHandler_;
};

class RacketChoiceStrip : public QWidget {
public:
    explicit RacketChoiceStrip(const QString& title, QWidget* parent = nullptr)
        : QWidget(parent),
          title_(title) {
        setMinimumHeight(152);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setCursor(Qt::PointingHandCursor);
    }

    void setCombo(QComboBox* combo) {
        combo_ = combo;
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        cardRects_.clear();

        QRectF outer(4, 4, width() - 8, height() - 8);
        painter.setPen(QPen(QColor(31, 42, 65), 3));
        painter.setBrush(QColor(255, 255, 255, 222));
        painter.drawRoundedRect(outer, 10, 10);

        painter.setPen(QColor(37, 48, 80));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 13, QFont::Black));
        painter.drawText(outer.adjusted(12, 6, -12, -6), Qt::AlignTop | Qt::AlignHCenter, title_);

        const int count = combo_ ? combo_->count() : 0;
        if (count == 0) {
            painter.setPen(QColor(90, 96, 108));
            painter.drawText(outer, Qt::AlignCenter, QStringLiteral("暂无球拍"));
            return;
        }

        const double gap = 8.0;
        const double maxCardW = 112.0;
        const double available = outer.width() - 24.0 - gap * (count - 1);
        const double cardW = std::min(maxCardW, available / count);
        const double totalW = cardW * count + gap * (count - 1);
        double x = outer.center().x() - totalW / 2.0;
        const double y = outer.top() + 34.0;

        for (int i = 0; i < count; ++i) {
            QRectF card(x, y, cardW, outer.height() - 44.0);
            cardRects_.push_back(card);

            const int racketIndex = combo_->itemData(i).toInt();
            const RacketItem& racket = racketByIndex(racketIndex);
            const bool selected = i == combo_->currentIndex();

            painter.setPen(QPen(selected ? QColor(255, 205, 61) : QColor(123, 130, 146), selected ? 5 : 3));
            painter.setBrush(selected ? QColor(42, 204, 104) : QColor(241, 244, 248));
            painter.drawRoundedRect(card, 8, 8);

            drawRacketIcon(painter, card.adjusted(8, 2, -8, -32), racket);

            painter.setPen(selected ? QColor(255, 255, 255) : QColor(31, 42, 65));
            painter.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 9, QFont::Black));
            painter.drawText(QRectF(card.left() + 4, card.bottom() - 30, card.width() - 8, 16),
                             Qt::AlignCenter, racket.name);
            painter.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 8, QFont::DemiBold));
            painter.drawText(QRectF(card.left() + 4, card.bottom() - 15, card.width() - 8, 13),
                             Qt::AlignCenter, QStringLiteral("命中+%1%").arg(static_cast<int>(racket.hitBonus * 100.0)));

            x += cardW + gap;
        }
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (!combo_) {
            return;
        }
        for (int i = 0; i < static_cast<int>(cardRects_.size()); ++i) {
            if (cardRects_[i].contains(event->position())) {
                combo_->setCurrentIndex(i);
                update();
                return;
            }
        }
    }

private:
    QString title_;
    QComboBox* combo_ = nullptr;
    std::vector<QRectF> cardRects_;
};

QPushButton* makeMenuButton(const QString& text, bool primary = false) {
    auto* button = new QPushButton(text);
    button->setMinimumHeight(primary ? 58 : 52);
    button->setCursor(Qt::PointingHandCursor);
    button->setStyleSheet(primary
                              ? QStringLiteral(
                                    "QPushButton{background:#d7f75b;color:#17322b;border:1px solid rgba(255,255,255,165);"
                                    "border-radius:8px;font-size:19px;font-weight:900;text-align:left;padding:11px 24px;}"
                                    "QPushButton:hover{background:#e6ff77;border-color:#ffffff;}"
                                    "QPushButton:pressed{background:#b9db42;}")
                              : QStringLiteral(
                                    "QPushButton{background:rgba(248,250,245,235);color:#17322b;border:1px solid rgba(255,255,255,130);"
                                    "border-radius:8px;font-size:17px;font-weight:800;text-align:left;padding:10px 22px;}"
                                    "QPushButton:hover{background:#ffffff;border-color:#d7f75b;}"
                                    "QPushButton:pressed{background:#d8e8df;}"));
    return button;
}

class MenuBackdropPage : public QWidget {
public:
    explicit MenuBackdropPage(QWidget* parent = nullptr)
        : QWidget(parent) {
        setAutoFillBackground(false);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const double w = width();
        const double h = height();

        QLinearGradient background(0, 0, w, h);
        background.setColorAt(0.0, QColor(152, 187, 212));
        background.setColorAt(0.48, QColor(110, 134, 162));
        background.setColorAt(1.0, QColor(64, 78, 104));
        painter.fillRect(rect(), background);

        painter.setPen(Qt::NoPen);
        for (int i = 0; i < 9; ++i) {
            const double x = i * w / 8.0;
            painter.setBrush(i % 2 == 0 ? QColor(106, 141, 171, 86) : QColor(168, 196, 218, 62));
            painter.drawRect(QRectF(x - w / 16.0, 0, w / 8.0, h * 0.36));
        }

        painter.setBrush(QColor(63, 70, 79, 232));
        painter.drawRect(QRectF(0, h * 0.18, w * 0.72, h * 0.47));
        painter.setPen(QPen(QColor(38, 51, 61), 4));
        painter.drawLine(QPointF(0, h * 0.25), QPointF(w * 0.72, h * 0.25));
        painter.drawLine(QPointF(0, h * 0.48), QPointF(w * 0.72, h * 0.48));
        drawCrowd(painter, QRectF(w * 0.03, h * 0.20, w * 0.64, h * 0.36));

        painter.setPen(QPen(QColor(76, 83, 89), 4));
        for (int i = 0; i < 18; ++i) {
            const double x = i * w * 0.72 / 17.0;
            painter.drawLine(QPointF(x, h * 0.27), QPointF(x + w * 0.035, h * 0.38));
            painter.drawLine(QPointF(x + w * 0.035, h * 0.27), QPointF(x, h * 0.38));
        }

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(172, 70, 47));
        painter.drawRect(QRectF(0, h * 0.60, w * 0.72, h * 0.08));
        painter.setBrush(QColor(7, 121, 82));
        painter.drawRect(QRectF(0, h * 0.68, w * 0.72, h * 0.27));
        painter.setBrush(QColor(77, 38, 9));
        painter.drawRect(QRectF(0, h * 0.95, w, h * 0.05));

        painter.setPen(QPen(QColor(244, 252, 247), 4));
        const QRectF court(w * 0.03, h * 0.73, w * 0.61, h * 0.13);
        painter.drawRect(court);
        painter.drawLine(QPointF(court.left() + court.width() * 0.28, court.top()),
                         QPointF(court.left() + court.width() * 0.23, court.bottom()));
        painter.drawLine(QPointF(court.left() + court.width() * 0.56, court.top()),
                         QPointF(court.left() + court.width() * 0.51, court.bottom()));
        painter.drawLine(QPointF(court.left(), court.center().y()),
                         QPointF(court.right(), court.center().y()));
        painter.setPen(QPen(QColor(25, 40, 41), 7, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(court.right() - 4, court.top() - 24),
                         QPointF(court.right() - 4, court.bottom() + 16));
        painter.setPen(QPen(QColor(248, 251, 250), 3));
        painter.drawLine(QPointF(court.right() - 4, court.top() - 24),
                         QPointF(court.right() - 4, court.bottom() + 16));

        drawMenuLogoImage(painter, QRectF(w * 0.105, -h * 0.025, w * 0.55, h * 0.37));
        drawArcadePlayer(painter, QPointF(w * 0.29, h * 0.78), std::min(w / 1280.0, h / 720.0) * 1.06,
                         outfitByCatalogIndex(5), racketByIndex(0), true);
        drawArcadePlayer(painter, QPointF(w * 0.47, h * 0.79), std::min(w / 1280.0, h / 720.0) * 1.02,
                         outfitByCatalogIndex(0), racketByIndex(1), false);

        QPolygonF sidePanel;
        sidePanel << QPointF(w * 0.72, 0) << QPointF(w, 0) << QPointF(w, h) << QPointF(w * 0.64, h);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(37, 160, 211));
        painter.drawPolygon(sidePanel);
        painter.setPen(QPen(QColor(21, 84, 192), 13));
        painter.drawLine(QPointF(w * 0.72, 0), QPointF(w * 0.64, h));
        painter.setPen(QPen(QColor(73, 211, 244), 5));
        painter.drawLine(QPointF(w * 0.735, 0), QPointF(w * 0.655, h));

    }
};

class HelpImagePage : public QWidget {
public:
    explicit HelpImagePage(const QString& imagePath, QWidget* parent = nullptr)
        : QWidget(parent),
          pixmap_(imagePath) {
        setAutoFillBackground(false);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        painter.fillRect(rect(), QColor(19, 40, 47));

        if (pixmap_.isNull()) {
            return;
        }

        painter.drawPixmap(QRectF(rect()), pixmap_, QRectF(pixmap_.rect()));
    }

private:
    QPixmap pixmap_;
};

class ShopBackgroundPage : public QWidget {
public:
    explicit ShopBackgroundPage(QWidget* parent = nullptr)
        : QWidget(parent) {
        setAutoFillBackground(false);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        QLinearGradient gradient(rect().topLeft(), rect().bottomRight());
        gradient.setColorAt(0.0, QColor(255, 219, 120));
        gradient.setColorAt(0.42, QColor(247, 151, 52));
        gradient.setColorAt(1.0, QColor(187, 74, 23));
        painter.fillRect(rect(), gradient);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 255, 34));
        painter.drawEllipse(QPointF(width() * 0.08, height() * 0.12), width() * 0.28, height() * 0.20);
        painter.setBrush(QColor(255, 245, 190, 28));
        painter.drawEllipse(QPointF(width() * 0.92, height() * 0.88), width() * 0.30, height() * 0.24);
    }
};

class MenuHeroPreview : public QWidget {
public:
    explicit MenuHeroPreview(QWidget* parent = nullptr)
        : QWidget(parent) {
        setMinimumSize(430, 420);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const QRectF panel(3, 3, width() - 6, height() - 6);
        QPainterPath clip;
        clip.addRoundedRect(panel, 14, 14);

        painter.save();
        painter.setClipPath(clip);
        QLinearGradient panelFill(panel.topLeft(), panel.bottomRight());
        panelFill.setColorAt(0.0, QColor(247, 253, 241, 238));
        panelFill.setColorAt(0.45, QColor(212, 237, 217, 232));
        panelFill.setColorAt(1.0, QColor(62, 149, 105, 230));
        painter.fillPath(clip, panelFill);

        drawMiniCourt(painter, panel.adjusted(26, 82, -26, -28));
        drawPlayer(painter, QPointF(panel.left() + panel.width() * 0.36, panel.bottom() - 68), 1.18,
                   QColor(74, 166, 236), QColor(245, 252, 255), false, true);
        drawPlayer(painter, QPointF(panel.left() + panel.width() * 0.67, panel.bottom() - 78), 1.02,
                   QColor(255, 136, 179), QColor(255, 234, 105), true, false);
        drawFlyingBall(painter, panel);
        painter.restore();

        painter.setPen(QPen(QColor(255, 255, 255, 78), 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(panel, 14, 14);

        painter.setPen(QColor(18, 49, 46));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 18, QFont::Bold));
        painter.drawText(panel.adjusted(28, 22, -28, -10), Qt::AlignLeft | Qt::AlignTop,
                         QStringLiteral("主场大厅"));

        painter.setPen(QColor(59, 83, 77));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 10, QFont::DemiBold));
        painter.drawText(panel.adjusted(30, 52, -30, -10), Qt::AlignLeft | Qt::AlignTop,
                         QStringLiteral("Singles  /  Versus  /  Shop"));
    }

private:
    static void drawMiniCourt(QPainter& painter, const QRectF& r) {
        const QPointF tl(r.left() + r.width() * 0.20, r.top() + r.height() * 0.08);
        const QPointF tr(r.right() - r.width() * 0.20, r.top() + r.height() * 0.08);
        const QPointF br(r.right(), r.bottom());
        const QPointF bl(r.left(), r.bottom());

        QPolygonF court;
        court << tl << tr << br << bl;
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(46, 139, 87, 222));
        painter.drawPolygon(court);

        auto point = [&](double x, double y) {
            const double left = tl.x() + (bl.x() - tl.x()) * y;
            const double right = tr.x() + (br.x() - tr.x()) * y;
            return QPointF(left + (right - left) * x, tl.y() + (bl.y() - tl.y()) * y);
        };

        painter.setPen(QPen(QColor(252, 255, 246, 225), 3, Qt::SolidLine, Qt::RoundCap));
        auto line = [&](double x1, double y1, double x2, double y2) {
            painter.drawLine(point(x1, y1), point(x2, y2));
        };
        line(0.10, 0.10, 0.90, 0.10);
        line(0.10, 0.90, 0.90, 0.90);
        line(0.10, 0.10, 0.10, 0.90);
        line(0.90, 0.10, 0.90, 0.90);
        line(0.25, 0.34, 0.75, 0.34);
        line(0.25, 0.66, 0.75, 0.66);
        line(0.50, 0.34, 0.50, 0.66);
        painter.setPen(QPen(QColor(19, 54, 49, 210), 5, Qt::SolidLine, Qt::RoundCap));
        line(0.08, 0.50, 0.92, 0.50);
    }

    static void drawPlayer(QPainter& painter, const QPointF& feet, double scale,
                           const QColor& shirt, const QColor& accent, bool female, bool facingRight) {
        painter.save();
        painter.translate(feet);
        painter.scale(facingRight ? scale : -scale, scale);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 45));
        painter.drawEllipse(QPointF(0, 2), 34, 8);

        painter.setPen(QPen(QColor(245, 201, 162), 9, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(-12, -42), QPointF(-20, -4));
        painter.drawLine(QPointF(12, -42), QPointF(21, -4));
        painter.setPen(QPen(QColor(31, 55, 63), 8, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(-24, -2), QPointF(-8, -2));
        painter.drawLine(QPointF(14, -2), QPointF(30, -2));

        painter.setPen(Qt::NoPen);
        if (female) {
            QPolygonF skirt;
            skirt << QPointF(-28, -66) << QPointF(28, -66) << QPointF(38, -40) << QPointF(-34, -40);
            painter.setBrush(accent);
            painter.drawPolygon(skirt);
        } else {
            painter.setBrush(accent);
            painter.drawRoundedRect(QRectF(-25, -68, 22, 28), 6, 6);
            painter.drawRoundedRect(QRectF(3, -68, 22, 28), 6, 6);
        }

        painter.setBrush(shirt);
        painter.drawRoundedRect(QRectF(-28, -122, 56, 58), 13, 13);
        painter.setBrush(accent);
        painter.drawRoundedRect(QRectF(-8, -121, 16, 43), 7, 7);
        painter.setBrush(QColor(255, 255, 255, 215));
        painter.drawPie(QRectF(-16, -126, 32, 24), 200 * 16, 140 * 16);

        painter.setPen(QPen(QColor(245, 201, 162), 9, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(-25, -108), QPointF(-48, -84));
        painter.drawLine(QPointF(25, -108), QPointF(54, -138));

        painter.setPen(QPen(QColor(38, 52, 61), 7, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(54, -138), QPointF(75, -164));
        painter.setPen(QPen(QColor(37, 65, 70), 5));
        painter.setBrush(QColor(248, 253, 255, 48));
        painter.drawEllipse(QPointF(83, -174), 18, 25);
        painter.setPen(QPen(QColor(236, 248, 246), 1.1));
        for (int i = -2; i <= 2; ++i) {
            painter.drawLine(QPointF(83 + i * 6, -194), QPointF(83 + i * 6, -154));
            painter.drawLine(QPointF(69, -174 + i * 7), QPointF(97, -174 + i * 7));
        }

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(245, 201, 162));
        painter.drawRoundedRect(QRectF(-10, -140, 20, 20), 8, 8);
        painter.drawEllipse(QPointF(0, -162), 30, 31);

        painter.setBrush(female ? QColor(78, 48, 36) : QColor(54, 38, 28));
        painter.drawPie(QRectF(-29, -187, 58, 42), 0, 180 * 16);
        if (female) {
            painter.drawEllipse(QPointF(-25, -156), 8, 17);
            painter.drawEllipse(QPointF(24, -157), 8, 17);
        }

        painter.setBrush(accent);
        painter.drawPie(QRectF(-34, -192, 68, 36), 0, 180 * 16);
        painter.setBrush(QColor(255, 255, 255, 230));
        painter.drawRoundedRect(QRectF(3, -184, 34, 8), 4, 4);

        painter.setBrush(QColor(31, 39, 45));
        painter.drawEllipse(QPointF(-10, -160), 3, 4);
        painter.drawEllipse(QPointF(10, -160), 3, 4);
        painter.setPen(QPen(QColor(122, 63, 58), 2, Qt::SolidLine, Qt::RoundCap));
        painter.drawArc(QRectF(-9, -153, 18, 10), 205 * 16, 130 * 16);

        painter.restore();
    }

    static void drawFlyingBall(QPainter& painter, const QRectF& panel) {
        QPainterPath path;
        path.moveTo(panel.left() + panel.width() * 0.47, panel.top() + panel.height() * 0.36);
        path.cubicTo(panel.left() + panel.width() * 0.58, panel.top() + panel.height() * 0.16,
                     panel.left() + panel.width() * 0.77, panel.top() + panel.height() * 0.26,
                     panel.left() + panel.width() * 0.83, panel.top() + panel.height() * 0.42);
        QPen pen(QColor(255, 255, 255, 150), 3, Qt::DashLine, Qt::RoundCap);
        pen.setDashPattern({4, 7});
        painter.setPen(pen);
        painter.drawPath(path);

        const QPointF ball(panel.left() + panel.width() * 0.83, panel.top() + panel.height() * 0.42);
        painter.setPen(QPen(QColor(235, 255, 118), 2));
        painter.setBrush(QColor(213, 242, 54));
        painter.drawEllipse(ball, 12, 12);
        painter.setPen(QPen(QColor(255, 255, 255, 190), 1.4));
        painter.drawArc(QRectF(ball.x() - 9, ball.y() - 10, 18, 20), 75 * 16, 160 * 16);
    }
};

class CharacterSelectPage : public QWidget {
public:
    explicit CharacterSelectPage(QWidget* parent = nullptr)
        : QWidget(parent) {
        setAutoFillBackground(false);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const double w = width();
        const double h = height();

        painter.fillRect(rect(), QColor(31, 166, 222));
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(236, 5, 106));
        QPolygonF rightSide;
        rightSide << QPointF(w * 0.66, 0) << QPointF(w, 0) << QPointF(w, h) << QPointF(w * 0.34, h);
        painter.drawPolygon(rightSide);

        painter.setBrush(QColor(255, 255, 255, 210));
        QPolygonF stripe;
        stripe << QPointF(w * 0.68, 0) << QPointF(w * 0.72, 0)
               << QPointF(w * 0.40, h) << QPointF(w * 0.36, h);
        painter.drawPolygon(stripe);
        painter.setPen(QPen(QColor(248, 68, 146), 5));
        painter.drawLine(QPointF(w * 0.69, 0), QPointF(w * 0.37, h));
        painter.drawLine(QPointF(w * 0.72, 0), QPointF(w * 0.40, h));

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 255, 255, 26));
        for (int y = 18; y < h; y += 34) {
            for (int x = 22; x < w; x += 34) {
                painter.drawEllipse(QPointF(x, y), 5, 5);
            }
        }

        drawOutlinedText(painter, QRectF(w * 0.43, h * 0.39, w * 0.14, h * 0.12), QStringLiteral("VS"),
                         QFont(QStringLiteral("Arial Black"), static_cast<int>(h * 0.105), QFont::Black),
                         QColor(255, 255, 255), QColor(20, 20, 24), Qt::AlignCenter, 4);
        drawOutlinedText(painter, QRectF(w * 0.34, h * 0.51, w * 0.32, h * 0.08), QStringLiteral("CHARACTER SELECT"),
                         QFont(QStringLiteral("Arial Black"), static_cast<int>(h * 0.044), QFont::Black),
                         QColor(255, 255, 255), QColor(58, 70, 98), Qt::AlignCenter, 2);
    }
};

class SelectPlayerPreview : public QWidget {
public:
    explicit SelectPlayerPreview(bool facingRight, QWidget* parent = nullptr)
        : QWidget(parent),
          facingRight_(facingRight) {
        setMinimumSize(310, 330);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }

    void setProviders(std::function<OutfitItem()> outfitProvider,
                      std::function<RacketItem()> racketProvider) {
        outfitProvider_ = std::move(outfitProvider);
        racketProvider_ = std::move(racketProvider);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const QRectF r(0, 0, width(), height());
        OutfitItem outfit = outfitProvider_ ? outfitProvider_() : outfitByCatalogIndex(0);
        RacketItem racket = racketProvider_ ? racketProvider_() : racketByIndex(0);
        const double scale = std::min(width() / 380.0, height() / 430.0);
        drawArcadePlayer(painter, QPointF(r.center().x(), r.bottom() - 18), scale * 1.18,
                         outfit, racket, facingRight_);
    }

private:
    bool facingRight_ = true;
    std::function<OutfitItem()> outfitProvider_;
    std::function<RacketItem()> racketProvider_;
};

class ProductCard : public QWidget {
public:
    explicit ProductCard(const OutfitItem& outfit,
                         std::function<bool(int)> isOwned,
                         std::function<int(int)> coinsFor,
                         std::function<bool(int)> buyFor,
                         QWidget* parent = nullptr)
        : QWidget(parent),
          outfit_(outfit),
          isRacket_(false),
          isOwned_(std::move(isOwned)),
          coinsFor_(std::move(coinsFor)),
          buyFor_(std::move(buyFor)) {
        setFixedSize(178, 252);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setupButtons();
    }

    explicit ProductCard(const RacketItem& racket,
                         std::function<bool(int)> isOwned,
                         std::function<int(int)> coinsFor,
                         std::function<bool(int)> buyFor,
                         QWidget* parent = nullptr)
        : QWidget(parent),
          racket_(racket),
          isRacket_(true),
          isOwned_(std::move(isOwned)),
          coinsFor_(std::move(coinsFor)),
          buyFor_(std::move(buyFor)) {
        setFixedSize(178, 252);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setupButtons();
    }

    void refreshButtons() {
        refreshButton(p1Button_, 1);
        refreshButton(p2Button_, 2);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        QRectF card(4, 4, width() - 8, height() - 8);
        painter.setPen(QPen(QColor(255, 255, 255, 48), 1));
        painter.setBrush(QColor(244, 249, 247));
        painter.drawRoundedRect(card, 8, 8);

        QRectF preview(14, 14, width() - 28, 108);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(226, 241, 238));
        painter.drawRoundedRect(preview, 8, 8);

        if (isRacket_) {
            drawRacketPreview(painter, preview);
        } else {
            drawOutfitPreview(painter, preview);
        }

        painter.setPen(QColor(23, 44, 52));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 10, QFont::Bold));
        painter.drawText(QRectF(12, 126, width() - 24, 22), Qt::AlignCenter,
                         isRacket_ ? racket_.name : outfit_.name);

        painter.setPen(QColor(78, 91, 96));
        painter.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 9));
        const QString meta = isRacket_
                                 ? QStringLiteral("%1 金币  命中+%2%")
                                       .arg(racket_.price)
                                       .arg(static_cast<int>(racket_.hitBonus * 100.0))
                                 : QStringLiteral("%1 金币").arg(outfit_.price);
        painter.drawText(QRectF(12, 149, width() - 24, 18), Qt::AlignCenter, meta);

        if (isRacket_) {
            painter.setPen(QColor(97, 112, 118));
            painter.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 8));
            painter.drawText(QRectF(10, 168, width() - 20, 15), Qt::AlignCenter,
                             QStringLiteral("范围+%1 控球±%2")
                                 .arg(racket_.rangeBonus, 0, 'f', 2)
                                 .arg(racket_.controlError, 0, 'f', 2));
        }
    }

private:
    int price() const {
        return isRacket_ ? racket_.price : outfit_.price;
    }

    void setupButtons() {
        p1Button_ = new QPushButton(this);
        p2Button_ = new QPushButton(this);
        for (QPushButton* button : {p1Button_, p2Button_}) {
            button->setCursor(Qt::PointingHandCursor);
            button->setGeometry(button == p1Button_ ? QRect(16, 205, 68, 30) : QRect(94, 205, 68, 30));
            button->setStyleSheet(QStringLiteral(
                "QPushButton{background:#244f58;color:#f8fbfb;border:0;border-radius:5px;"
                "font-size:12px;font-weight:700;}"
                "QPushButton:hover{background:#2d6570;}"
                "QPushButton:disabled{background:#d5ddda;color:#7a8789;}"));
        }
        connect(p1Button_, &QPushButton::clicked, this, [this] {
            if (buyFor_) {
                buyFor_(1);
            }
        });
        connect(p2Button_, &QPushButton::clicked, this, [this] {
            if (buyFor_) {
                buyFor_(2);
            }
        });
        refreshButtons();
    }

    void refreshButton(QPushButton* button, int playerId) {
        if (!button) {
            return;
        }
        const bool owned = isOwned_ && isOwned_(playerId);
        const int coins = coinsFor_ ? coinsFor_(playerId) : 0;
        if (owned) {
            button->setText(QStringLiteral("P%1已购").arg(playerId));
            button->setEnabled(false);
        } else if (coins < price()) {
            button->setText(QStringLiteral("P%1不足").arg(playerId));
            button->setEnabled(false);
        } else {
            button->setText(QStringLiteral("P%1购买").arg(playerId));
            button->setEnabled(true);
        }
    }

    void drawOutfitPreview(QPainter& painter, const QRectF& r) const {
        if (drawOutfitImage(painter, r.adjusted(2, -4, -2, 4), outfit_)) {
            return;
        }

        const double cx = r.center().x();
        const double top = r.top() + 13;
        const double s = 1.0;

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 38));
        painter.drawEllipse(QPointF(cx, r.bottom() - 8), 34, 7);

        painter.setBrush(QColor(245, 202, 164));
        painter.drawEllipse(QPointF(cx, top + 26), 15 * s, 15 * s);

        painter.setBrush(outfit_.hairColor);
        painter.drawPie(QRectF(cx - 16, top + 7, 32, 18), 0, 180 * 16);
        painter.drawEllipse(QPointF(cx - 9, top + 22), 4, 5);
        painter.drawEllipse(QPointF(cx + 8, top + 22), 4, 5);

        painter.setBrush(outfit_.hatColor);
        painter.drawPie(QRectF(cx - 20, top - 1, 40, 20), 0, 180 * 16);
        painter.setBrush(outfit_.shirtAccent);
        painter.drawRoundedRect(QRectF(cx + 7, top + 8, 22, 5), 3, 3);

        if (outfit_.gender == Gender::Female) {
            painter.setBrush(outfit_.hatColor.lighter(112));
            painter.drawEllipse(QPointF(cx - 11, top - 3), 5, 12);
            painter.drawEllipse(QPointF(cx + 5, top - 4), 5, 12);
        }

        painter.setBrush(QColor(33, 39, 45));
        painter.drawEllipse(QPointF(cx - 6, top + 25), 2, 2.6);
        painter.drawEllipse(QPointF(cx + 6, top + 25), 2, 2.6);
        painter.setBrush(QColor(255, 150, 165, 135));
        painter.drawEllipse(QPointF(cx - 11, top + 31), 3.2, 2.1);
        painter.drawEllipse(QPointF(cx + 11, top + 31), 3.2, 2.1);

        QRectF body(cx - 22, top + 44, 44, 42);
        painter.setBrush(outfit_.shirtMain);
        painter.drawRoundedRect(body, 8, 8);

        painter.setBrush(outfit_.shirtAccent);
        painter.drawRoundedRect(QRectF(body.left() + 5, body.top() + 6, 8, 31), 4, 4);
        painter.drawRoundedRect(QRectF(body.right() - 13, body.top() + 6, 8, 31), 4, 4);

        painter.setBrush(outfit_.collarColor);
        QPolygonF collar;
        collar << QPointF(cx - 9, body.top() + 3) << QPointF(cx, body.top() + 14)
               << QPointF(cx + 9, body.top() + 3);
        painter.drawPolygon(collar);

        painter.setBrush(outfit_.sleeveColor);
        painter.drawEllipse(QPointF(body.left() - 1, body.top() + 15), 9, 11);
        painter.drawEllipse(QPointF(body.right() + 1, body.top() + 15), 9, 11);

        painter.setFont(QFont(QStringLiteral("Segoe UI Symbol"), 15, QFont::Bold));
        painter.setPen(outfit_.stripeColor);
        painter.drawText(QRectF(cx - 13, body.top() + 16, 26, 18), Qt::AlignCenter, outfit_.chestMark);
        painter.setPen(Qt::NoPen);

        if (outfit_.gender == Gender::Female) {
            QPolygonF skirt;
            skirt << QPointF(body.left() - 5, body.bottom() - 2)
                  << QPointF(body.right() + 5, body.bottom() - 2)
                  << QPointF(cx + 24, body.bottom() + 21)
                  << QPointF(cx - 24, body.bottom() + 21);
            painter.setBrush(outfit_.bottomMain);
            painter.drawPolygon(skirt);
            painter.setPen(QPen(outfit_.bottomAccent, 2));
            for (int i = -2; i <= 2; ++i) {
                painter.drawLine(QPointF(cx + i * 8, body.bottom() + 1),
                                 QPointF(cx + i * 10, body.bottom() + 19));
            }
            painter.setPen(Qt::NoPen);
        } else {
            painter.setBrush(outfit_.bottomMain);
            painter.drawRoundedRect(QRectF(cx - 22, body.bottom() - 1, 20, 21), 4, 4);
            painter.drawRoundedRect(QRectF(cx + 2, body.bottom() - 1, 20, 21), 4, 4);
            painter.setPen(QPen(outfit_.bottomAccent, 2));
            painter.drawLine(QPointF(cx, body.bottom() + 3), QPointF(cx, body.bottom() + 18));
            painter.setPen(Qt::NoPen);
        }

        painter.setBrush(outfit_.shoeColor);
        painter.drawRoundedRect(QRectF(cx - 26, r.bottom() - 18, 20, 8), 4, 4);
        painter.drawRoundedRect(QRectF(cx + 6, r.bottom() - 18, 20, 8), 4, 4);
    }

    void drawRacketPreview(QPainter& painter, const QRectF& r) const {
        if (!racket_.imagePath.isEmpty()) {
            const QPixmap pixmap(racket_.imagePath);
            if (!pixmap.isNull()) {
                QSizeF targetSize = pixmap.size();
                targetSize.scale(r.size(), Qt::KeepAspectRatio);
                const QRectF target(QPointF(r.center().x() - targetSize.width() / 2.0,
                                            r.center().y() - targetSize.height() / 2.0),
                                    targetSize);

                painter.save();
                painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
                painter.drawPixmap(target, pixmap, QRectF(pixmap.rect()));
                painter.restore();
                return;
            }
        }

        const QPointF head(r.center().x(), r.top() + 45);
        const QPointF grip(r.center().x() - 34, r.bottom() - 18);

        painter.save();
        painter.translate(r.center());
        painter.rotate(-28);
        painter.translate(-r.center());

        painter.setPen(QPen(racket_.gripColor, 8, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(head.x() - 8, head.y() + 42), grip);

        painter.setPen(QPen(racket_.frameColor, 7));
        painter.setBrush(QColor(255, 255, 255, 26));
        painter.drawEllipse(head, 31, 42);

        painter.setPen(QPen(racket_.accentColor, 3));
        painter.drawArc(QRectF(head.x() - 27, head.y() - 38, 54, 76), 30 * 16, 120 * 16);

        painter.setPen(QPen(racket_.stringColor, 1.5));
        for (int i = -3; i <= 3; ++i) {
            painter.drawLine(QPointF(head.x() + i * 7, head.y() - 32),
                             QPointF(head.x() + i * 7, head.y() + 32));
            painter.drawLine(QPointF(head.x() - 24, head.y() + i * 8),
                             QPointF(head.x() + 24, head.y() + i * 8));
        }

        painter.setPen(Qt::NoPen);
        painter.setBrush(racket_.accentColor);
        for (int i = 0; i < racket_.starLevel; ++i) {
            painter.drawEllipse(QPointF(head.x() - 18 + i * 7, head.y() - 51), 2.2, 2.2);
        }
        painter.restore();
    }

    OutfitItem outfit_;
    RacketItem racket_;
    bool isRacket_ = false;
    std::function<bool(int)> isOwned_;
    std::function<int(int)> coinsFor_;
    std::function<bool(int)> buyFor_;
    QPushButton* p1Button_ = nullptr;
    QPushButton* p2Button_ = nullptr;
};

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("Tennis Duel - Qt"));
    resize(1280, 720);

    initializeStores();

    pages_ = new QStackedWidget(this);
    setCentralWidget(pages_);

    menuPage_ = createMenuPage();
    singleSetupPage_ = createSingleSetupPage();
    setupPage_ = createSetupPage();
    shopPage_ = createShopPage();
    helpPage_ = createHelpPage();
    gamePage_ = new GameWidget;
    resultPage_ = createResultPage();

    pages_->addWidget(menuPage_);
    pages_->addWidget(singleSetupPage_);
    pages_->addWidget(setupPage_);
    pages_->addWidget(shopPage_);
    pages_->addWidget(helpPage_);
    pages_->addWidget(gamePage_);
    pages_->addWidget(resultPage_);

    gamePage_->setMatchFinishedHandler([this](int winner, const QString& summary) {
        showResult(winner, summary);
    });

    showMenu();
    initializeMusic();
}

void MainWindow::initializeMusic() {
    audioOutput_ = new QAudioOutput(this);
    audioOutput_->setVolume(0.36);

    musicPlayer_ = new QMediaPlayer(this);
    musicPlayer_->setAudioOutput(audioOutput_);
    musicPlayer_->setSource(QUrl(QStringLiteral("qrc:/assets/upbeat_tennis_loop.ogg")));
    musicPlayer_->setLoops(QMediaPlayer::Infinite);

    if (soundEnabled_) {
        musicPlayer_->play();
    }
}

void MainWindow::setSoundEnabled(bool enabled) {
    soundEnabled_ = enabled;
    if (!musicPlayer_) {
        return;
    }

    if (enabled) {
        musicPlayer_->play();
    } else {
        musicPlayer_->stop();
    }
}

QWidget* MainWindow::createMenuPage() {
    auto* page = new MenuBackdropPage;
    auto* soundButton = new SoundToggleButton(page);
    soundButton->move(10, 10);
    soundButton->setChecked(soundEnabled_);

    auto* root = new QHBoxLayout(page);
    root->setContentsMargins(0, 0, 34, 0);
    root->setSpacing(0);

    auto* leftFill = new QWidget;
    leftFill->setStyleSheet(QStringLiteral("background:transparent;"));
    auto* menuRail = new QWidget;
    menuRail->setStyleSheet(QStringLiteral("background:transparent;"));
    menuRail->setMinimumWidth(420);
    menuRail->setMaximumWidth(500);

    auto* railLayout = new QVBoxLayout(menuRail);
    railLayout->setContentsMargins(18, 72, 0, 30);
    railLayout->setSpacing(10);

    auto* coins = new QLabel(page);
    coins->setPixmap(QPixmap(QStringLiteral(":/assets/select_coin_bar.png")));
    coins->setScaledContents(true);
    coins->setFixedSize(410, 67);
    coins->setStyleSheet(QStringLiteral("background:transparent;"));
    railLayout->addWidget(coins, 0, Qt::AlignCenter);
    railLayout->addStretch(1);

    auto* singleButton = makeMenuImageButton(QStringLiteral("单人模式"), QStringLiteral(":/assets/select_button_single.png"));
    auto* playButton = makeMenuImageButton(QStringLiteral("双人模式"), QStringLiteral(":/assets/select_button_double.png"));
    auto* shopButton = makeMenuImageButton(QStringLiteral("商店"), QStringLiteral(":/assets/select_button_shop_ui.png"));
    auto* helpButton = makeMenuImageButton(QStringLiteral("操作说明"), QStringLiteral(":/assets/select_button_help.png"));
    auto* quitButton = makeMenuImageButton(QStringLiteral("退出"), QStringLiteral(":/assets/select_button_exit_ui.png"));

    singleButton->setFixedHeight(88);
    playButton->setFixedHeight(88);
    shopButton->setFixedHeight(88);
    helpButton->setFixedHeight(88);
    quitButton->setFixedHeight(88);

    for (QPushButton* button : {singleButton, playButton, shopButton, helpButton, quitButton}) {
        button->setMinimumWidth(350);
        button->setMaximumWidth(414);
        railLayout->addWidget(button, 0, Qt::AlignCenter);
    }
    railLayout->addStretch(1);

    root->addWidget(leftFill, 1);
    root->addWidget(menuRail, 0);

    connect(singleButton, &QPushButton::clicked, this, &MainWindow::showSingleSetup);
    connect(playButton, &QPushButton::clicked, this, &MainWindow::showSetup);
    connect(shopButton, &QPushButton::clicked, this, &MainWindow::showShop);
    connect(helpButton, &QPushButton::clicked, this, &MainWindow::showHelp);
    connect(quitButton, &QPushButton::clicked, qApp, &QApplication::quit);
    connect(soundButton, &QPushButton::toggled, this, [this, soundButton](bool checked) {
        setSoundEnabled(checked);
        soundButton->setToolTip(checked ? QStringLiteral("声音开启") : QStringLiteral("声音关闭"));
        soundButton->update();
    });
    soundButton->raise();

    return page;
}

QWidget* MainWindow::createSingleSetupPage() {
    auto* page = new CharacterSelectPage;

    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(18, 8, 18, 22);
    root->setSpacing(0);

    auto* topBar = new QHBoxLayout;
    topBar->setSpacing(12);
    auto* backButton = makeSelectBackButton();
    auto* coins = createCoinLabel();
    coins->setStyleSheet(QStringLiteral(
        "font-size:15px;font-weight:900;color:#fff7d6;background:rgba(19,38,70,170);"
        "border:3px solid rgba(255,255,255,170);border-radius:6px;padding:8px 14px;"));
    topBar->addWidget(backButton, 0, Qt::AlignLeft | Qt::AlignTop);
    topBar->addStretch(1);
    topBar->addWidget(coins, 0, Qt::AlignRight | Qt::AlignTop);
    root->addLayout(topBar);

    singleP1GenderCombo_ = new QComboBox(page);
    singleP1GenderCombo_->addItem(QStringLiteral("男生"), QVariant::fromValue(0));
    singleP1GenderCombo_->addItem(QStringLiteral("女生"), QVariant::fromValue(1));
    singleP1OutfitCombo_ = new QComboBox(page);
    singleP1RacketCombo_ = new QComboBox(page);
    aiDifficultyCombo_ = new QComboBox(page);
    for (QComboBox* combo : {singleP1GenderCombo_, singleP1OutfitCombo_, singleP1RacketCombo_, aiDifficultyCombo_}) {
        styleSelectCombo(combo);
    }

    aiDifficultyCombo_->addItem(QStringLiteral("简单：反应慢，接球概率低，回球偏保守"), QVariant::fromValue(0));
    aiDifficultyCombo_->addItem(QStringLiteral("普通：反应中等，回球有一定变化"), QVariant::fromValue(1));
    aiDifficultyCombo_->addItem(QStringLiteral("困难：反应快，接球概率高，回球更难预判"), QVariant::fromValue(2));
    aiDifficultyCombo_->setCurrentIndex(1);

    fillOutfitCombo(singleP1OutfitCombo_, genderFromCombo(singleP1GenderCombo_), 1);
    fillRacketCombo(singleP1RacketCombo_, 1);

    auto* p1Preview = new SelectPlayerPreview(true);
    p1Preview->setProviders(
        [this] { return outfitByCatalogIndex(currentDataOrZero(singleP1OutfitCombo_)); },
        [this] { return racketByIndex(currentDataOrZero(singleP1RacketCombo_)); });

    auto* aiPreview = new SelectPlayerPreview(false);
    aiPreview->setProviders(
        [this] {
            const int difficulty = currentDataOrZero(aiDifficultyCombo_);
            return outfitByCatalogIndex(difficulty == 0 ? 0 : (difficulty == 1 ? 2 : 5));
        },
        [this] {
            const int difficulty = currentDataOrZero(aiDifficultyCombo_);
            return racketByIndex(difficulty == 0 ? 0 : (difficulty == 1 ? 2 : 4));
        });

    singleP1GenderCombo_->hide();
    singleP1OutfitCombo_->hide();
    singleP1RacketCombo_->hide();

    auto* outfitChoice = new OutfitChoiceWidget(QStringLiteral("人物服装"));
    auto* racketChoice = new RacketChoiceStrip(QStringLiteral("点击选择球拍"));
    racketChoice->setCombo(singleP1RacketCombo_);

    auto ownedOutfitsFor = [this](int playerId) {
        std::vector<int> indexes;
        const auto& items = outfitCatalog();
        for (int i = 0; i < static_cast<int>(items.size()); ++i) {
            if (storeFor(playerId).ownedOutfits.contains(items[i].id)) {
                indexes.push_back(i);
            }
        }
        return indexes;
    };

    auto selectOutfit = [this, p1Preview, outfitChoice](int catalogIndex) {
        const OutfitItem& outfit = outfitByCatalogIndex(catalogIndex);
        {
            const QSignalBlocker blockGender(singleP1GenderCombo_);
            singleP1GenderCombo_->setCurrentIndex(outfit.gender == Gender::Female ? 1 : 0);
        }
        fillOutfitCombo(singleP1OutfitCombo_, outfit.gender, 1);
        for (int i = 0; i < singleP1OutfitCombo_->count(); ++i) {
            if (singleP1OutfitCombo_->itemData(i).toInt() == catalogIndex) {
                singleP1OutfitCombo_->setCurrentIndex(i);
                break;
            }
        }
        p1Preview->update();
        outfitChoice->update();
    };

    outfitChoice->setProviders(
        [ownedOutfitsFor] { return ownedOutfitsFor(1); },
        [this] { return currentDataOrZero(singleP1OutfitCombo_); },
        selectOutfit);

    auto* rightPanel = new QFrame;
    rightPanel->setStyleSheet(QStringLiteral(
        "QFrame{background:rgba(255,255,255,218);border:3px solid rgba(30,40,67,210);border-radius:10px;}"));
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(18, 14, 18, 16);
    rightLayout->setSpacing(9);
    auto* difficultyTitle = new QLabel(QStringLiteral("电脑难度"));
    difficultyTitle->setAlignment(Qt::AlignCenter);
    difficultyTitle->setStyleSheet(QStringLiteral("font-size:17px;font-weight:900;color:#253050;"));
    rightLayout->addWidget(difficultyTitle);
    rightLayout->addWidget(aiDifficultyCombo_);
    auto* aiNote = new QLabel(QStringLiteral("电脑会根据难度自动选择外观和球拍"));
    aiNote->setWordWrap(true);
    aiNote->setAlignment(Qt::AlignCenter);
    aiNote->setStyleSheet(QStringLiteral("font-size:12px;font-weight:800;color:#4a5368;"));
    rightLayout->addWidget(aiNote);

    auto* content = new QHBoxLayout;
    content->setContentsMargins(36, 16, 36, 0);
    content->setSpacing(46);

    auto* leftColumn = new QVBoxLayout;
    leftColumn->setSpacing(8);
    leftColumn->addWidget(p1Preview, 1);
    leftColumn->addWidget(outfitChoice, 0);
    leftColumn->addWidget(racketChoice, 0);

    auto* centerColumn = new QVBoxLayout;
    centerColumn->addStretch(1);
    auto* startButton = makeArcadeButton(QStringLiteral("PLAY"), QColor(36, 204, 96),
                                         QColor(57, 225, 118), QColor(21, 119, 58));
    startButton->setMinimumWidth(290);
    startButton->setMaximumWidth(340);
    centerColumn->addWidget(startButton, 0, Qt::AlignCenter);
    centerColumn->addStretch(0);

    auto* rightColumn = new QVBoxLayout;
    rightColumn->setSpacing(8);
    rightColumn->addWidget(aiPreview, 1);
    rightColumn->addWidget(rightPanel, 0);

    content->addLayout(leftColumn, 1);
    content->addLayout(centerColumn, 0);
    content->addLayout(rightColumn, 1);
    root->addLayout(content, 1);

    connect(backButton, &QPushButton::clicked, this, &MainWindow::showMenu);
    connect(startButton, &QPushButton::clicked, this, &MainWindow::startSingleMatch);
    connect(singleP1GenderCombo_, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this] {
        fillOutfitCombo(singleP1OutfitCombo_, genderFromCombo(singleP1GenderCombo_), 1);
    });
    connect(singleP1GenderCombo_, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            p1Preview, [p1Preview] { p1Preview->update(); });
    connect(singleP1OutfitCombo_, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            p1Preview, [p1Preview, outfitChoice] {
                p1Preview->update();
                outfitChoice->update();
            });
    connect(singleP1RacketCombo_, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            p1Preview, [p1Preview, racketChoice] {
                p1Preview->update();
                racketChoice->update();
            });
    connect(aiDifficultyCombo_, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            aiPreview, [aiPreview] { aiPreview->update(); });

    return page;
}

QWidget* MainWindow::createSetupPage() {
    auto* page = new CharacterSelectPage;

    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(18, 8, 18, 22);
    root->setSpacing(0);

    auto* topBar = new QHBoxLayout;
    topBar->setSpacing(12);
    auto* backButton = makeSelectBackButton();
    auto* coins = createCoinLabel();
    coins->setStyleSheet(QStringLiteral(
        "font-size:15px;font-weight:900;color:#fff7d6;background:rgba(19,38,70,170);"
        "border:3px solid rgba(255,255,255,170);border-radius:6px;padding:8px 14px;"));
    topBar->addWidget(backButton, 0, Qt::AlignLeft | Qt::AlignTop);
    topBar->addStretch(1);
    topBar->addWidget(coins, 0, Qt::AlignRight | Qt::AlignTop);
    root->addLayout(topBar);

    p1GenderCombo_ = new QComboBox(page);
    p1GenderCombo_->addItem(QStringLiteral("男生"), QVariant::fromValue(0));
    p1GenderCombo_->addItem(QStringLiteral("女生"), QVariant::fromValue(1));
    p1OutfitCombo_ = new QComboBox(page);
    p1RacketCombo_ = new QComboBox(page);

    p2GenderCombo_ = new QComboBox(page);
    p2GenderCombo_->addItem(QStringLiteral("男生"), QVariant::fromValue(0));
    p2GenderCombo_->addItem(QStringLiteral("女生"), QVariant::fromValue(1));
    p2OutfitCombo_ = new QComboBox(page);
    p2RacketCombo_ = new QComboBox(page);
    for (QComboBox* combo : {p1GenderCombo_, p1OutfitCombo_, p1RacketCombo_,
                             p2GenderCombo_, p2OutfitCombo_, p2RacketCombo_}) {
        styleSelectCombo(combo);
    }

    p2GenderCombo_->setCurrentIndex(1);
    refreshOutfitCombos();
    p2RacketCombo_->setCurrentIndex(0);

    auto* p1Preview = new SelectPlayerPreview(true);
    p1Preview->setProviders(
        [this] { return outfitByCatalogIndex(currentDataOrZero(p1OutfitCombo_)); },
        [this] { return racketByIndex(currentDataOrZero(p1RacketCombo_)); });

    auto* p2Preview = new SelectPlayerPreview(false);
    p2Preview->setProviders(
        [this] { return outfitByCatalogIndex(currentDataOrZero(p2OutfitCombo_)); },
        [this] { return racketByIndex(currentDataOrZero(p2RacketCombo_)); });

    for (QComboBox* combo : {p1GenderCombo_, p1OutfitCombo_, p1RacketCombo_,
                             p2GenderCombo_, p2OutfitCombo_, p2RacketCombo_}) {
        combo->hide();
    }

    auto ownedOutfitsFor = [this](int playerId) {
        std::vector<int> indexes;
        const auto& items = outfitCatalog();
        for (int i = 0; i < static_cast<int>(items.size()); ++i) {
            if (storeFor(playerId).ownedOutfits.contains(items[i].id)) {
                indexes.push_back(i);
            }
        }
        return indexes;
    };

    auto* p1OutfitChoice = new OutfitChoiceWidget(QStringLiteral("P1 人物服装"));
    auto* p2OutfitChoice = new OutfitChoiceWidget(QStringLiteral("P2 人物服装"));
    auto* p1RacketChoice = new RacketChoiceStrip(QStringLiteral("P1 点击选择球拍"));
    auto* p2RacketChoice = new RacketChoiceStrip(QStringLiteral("P2 点击选择球拍"));
    p1RacketChoice->setCombo(p1RacketCombo_);
    p2RacketChoice->setCombo(p2RacketCombo_);

    auto selectOutfitFor = [this](int catalogIndex, int playerId, QComboBox* genderCombo, QComboBox* outfitCombo,
                                  SelectPlayerPreview* preview, OutfitChoiceWidget* outfitChoice) {
        const OutfitItem& outfit = outfitByCatalogIndex(catalogIndex);
        {
            const QSignalBlocker blockGender(genderCombo);
            genderCombo->setCurrentIndex(outfit.gender == Gender::Female ? 1 : 0);
        }
        fillOutfitCombo(outfitCombo, outfit.gender, playerId);
        for (int i = 0; i < outfitCombo->count(); ++i) {
            if (outfitCombo->itemData(i).toInt() == catalogIndex) {
                outfitCombo->setCurrentIndex(i);
                break;
            }
        }
        preview->update();
        outfitChoice->update();
    };

    p1OutfitChoice->setProviders(
        [ownedOutfitsFor] { return ownedOutfitsFor(1); },
        [this] { return currentDataOrZero(p1OutfitCombo_); },
        [=](int catalogIndex) {
            selectOutfitFor(catalogIndex, 1, p1GenderCombo_, p1OutfitCombo_, p1Preview, p1OutfitChoice);
        });
    p2OutfitChoice->setProviders(
        [ownedOutfitsFor] { return ownedOutfitsFor(2); },
        [this] { return currentDataOrZero(p2OutfitCombo_); },
        [=](int catalogIndex) {
            selectOutfitFor(catalogIndex, 2, p2GenderCombo_, p2OutfitCombo_, p2Preview, p2OutfitChoice);
        });

    auto* content = new QHBoxLayout;
    content->setContentsMargins(36, 16, 36, 0);
    content->setSpacing(46);

    auto* leftColumn = new QVBoxLayout;
    leftColumn->setSpacing(8);
    leftColumn->addWidget(p1Preview, 1);
    leftColumn->addWidget(p1OutfitChoice, 0);
    leftColumn->addWidget(p1RacketChoice, 0);

    auto* centerColumn = new QVBoxLayout;
    centerColumn->addStretch(1);
    auto* startButton = makeArcadeButton(QStringLiteral("PLAY"), QColor(36, 204, 96),
                                         QColor(57, 225, 118), QColor(21, 119, 58));
    startButton->setMinimumWidth(290);
    startButton->setMaximumWidth(340);
    centerColumn->addWidget(startButton, 0, Qt::AlignCenter);
    centerColumn->addStretch(0);

    auto* rightColumn = new QVBoxLayout;
    rightColumn->setSpacing(8);
    rightColumn->addWidget(p2Preview, 1);
    rightColumn->addWidget(p2OutfitChoice, 0);
    rightColumn->addWidget(p2RacketChoice, 0);

    content->addLayout(leftColumn, 1);
    content->addLayout(centerColumn, 0);
    content->addLayout(rightColumn, 1);
    root->addLayout(content, 1);

    connect(backButton, &QPushButton::clicked, this, &MainWindow::showMenu);
    connect(startButton, &QPushButton::clicked, this, &MainWindow::startDoubleMatch);
    connect(p1GenderCombo_, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this, p1Preview] {
        fillOutfitCombo(p1OutfitCombo_, genderFromCombo(p1GenderCombo_), 1);
        p1Preview->update();
    });
    connect(p2GenderCombo_, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this, p2Preview] {
        fillOutfitCombo(p2OutfitCombo_, genderFromCombo(p2GenderCombo_), 2);
        p2Preview->update();
    });
    connect(p1OutfitCombo_, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            p1Preview, [p1Preview, p1OutfitChoice] {
                p1Preview->update();
                p1OutfitChoice->update();
            });
    connect(p1RacketCombo_, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            p1Preview, [p1Preview, p1RacketChoice] {
                p1Preview->update();
                p1RacketChoice->update();
            });
    connect(p2OutfitCombo_, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            p2Preview, [p2Preview, p2OutfitChoice] {
                p2Preview->update();
                p2OutfitChoice->update();
            });
    connect(p2RacketCombo_, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            p2Preview, [p2Preview, p2RacketChoice] {
                p2Preview->update();
                p2RacketChoice->update();
            });

    return page;
}

QWidget* MainWindow::createShopPage() {
    shopRefreshers_.clear();

    auto* page = new ShopBackgroundPage;

    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(42, 36, 42, 32);
    root->setSpacing(14);

    auto* titleImage = new QLabel;
    titleImage->setPixmap(QPixmap(QStringLiteral(":/assets/shop_title_1.png")));
    titleImage->setScaledContents(true);
    titleImage->setFixedSize(260, 142);
    titleImage->setStyleSheet(QStringLiteral("background:transparent;"));
    root->addWidget(titleImage, 0, Qt::AlignHCenter);
    root->addWidget(createCoinLabel());

    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet(QStringLiteral("QScrollArea{background:transparent;}"));

    auto* content = new QWidget;
    content->setStyleSheet(QStringLiteral("background:transparent;"));
    auto* sections = new QVBoxLayout(content);
    sections->setContentsMargins(0, 0, 0, 0);
    sections->setSpacing(16);

    auto addSection = [&](const QString& title, const QVector<QWidget*>& cards) {
        auto* panel = makePanel();
        auto* layout = new QVBoxLayout(panel);
        layout->setContentsMargins(22, 16, 22, 18);
        layout->setSpacing(10);

        auto* label = new QLabel(title);
        label->setStyleSheet(QStringLiteral("font-size:19px;font-weight:800;color:#f6fbfb;"));
        layout->addWidget(label);

        auto* row = new QHBoxLayout;
        row->setSpacing(14);
        for (QWidget* card : cards) {
            row->addWidget(card);
        }
        row->addStretch();
        layout->addLayout(row);
        sections->addWidget(panel);
    };

    QVector<QWidget*> femaleCards;
    for (int index : outfitIndexesForGender(Gender::Female)) {
        const OutfitItem& outfit = outfitByCatalogIndex(index);
        auto* card = new ProductCard(
            outfit,
            [this, id = outfit.id](int playerId) { return storeFor(playerId).ownedOutfits.contains(id); },
            [this](int playerId) { return storeFor(playerId).coins; },
            [this, index](int playerId) { return buyOutfit(playerId, index); });
        shopRefreshers_.push_back([card] { card->refreshButtons(); });
        femaleCards.append(card);
    }
    addSection(QStringLiteral("女生服装"), femaleCards);

    QVector<QWidget*> maleCards;
    for (int index : outfitIndexesForGender(Gender::Male)) {
        const OutfitItem& outfit = outfitByCatalogIndex(index);
        auto* card = new ProductCard(
            outfit,
            [this, id = outfit.id](int playerId) { return storeFor(playerId).ownedOutfits.contains(id); },
            [this](int playerId) { return storeFor(playerId).coins; },
            [this, index](int playerId) { return buyOutfit(playerId, index); });
        shopRefreshers_.push_back([card] { card->refreshButtons(); });
        maleCards.append(card);
    }
    addSection(QStringLiteral("男生服装"), maleCards);

    QVector<QWidget*> racketCards;
    const auto& rackets = racketCatalog();
    for (int i = 0; i < static_cast<int>(rackets.size()); ++i) {
        const RacketItem& racket = rackets[i];
        auto* card = new ProductCard(
            racket,
            [this, id = racket.id](int playerId) { return storeFor(playerId).ownedRackets.contains(id); },
            [this](int playerId) { return storeFor(playerId).coins; },
            [this, i](int playerId) { return buyRacket(playerId, i); });
        shopRefreshers_.push_back([card] { card->refreshButtons(); });
        racketCards.append(card);
    }
    addSection(QStringLiteral("网球拍"), racketCards);

    scroll->setWidget(content);
    root->addWidget(scroll, 1);

    auto* backButton = makeButton(QStringLiteral("返回主菜单"));
    root->addWidget(backButton, 0, Qt::AlignHCenter);
    backButton->setFixedWidth(260);
    connect(backButton, &QPushButton::clicked, this, &MainWindow::showMenu);

    return page;
}

QWidget* MainWindow::createHelpPage() {
    auto* page = new HelpImagePage(QStringLiteral(":/assets/help_page.jpg"));
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(18, 8, 18, 22);
    root->setSpacing(0);

    auto* topBar = new QHBoxLayout;
    topBar->setContentsMargins(0, 0, 0, 0);
    topBar->setSpacing(0);

    auto* backButton = makeSelectBackButton();
    topBar->addWidget(backButton, 0, Qt::AlignLeft | Qt::AlignTop);
    topBar->addStretch(1);

    root->addLayout(topBar);
    root->addStretch(1);

    connect(backButton, &QPushButton::clicked, this, &MainWindow::showMenu);
    return page;
}

QWidget* MainWindow::createResultPage() {
    auto* page = new QWidget;
    page->setStyleSheet(QStringLiteral("background:#172e35;"));
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(100, 80, 100, 80);
    root->setSpacing(18);

    resultTitle_ = makeTitle(QStringLiteral("Match Finished"), 32);
    resultSummary_ = makeBody(QStringLiteral("Result"));

    auto* buttonRow = new QHBoxLayout;
    auto* againButton = makeButton(QStringLiteral("再来一局"));
    auto* menuButton = makeButton(QStringLiteral("返回主菜单"));
    buttonRow->addStretch();
    buttonRow->addWidget(againButton);
    buttonRow->addWidget(menuButton);
    buttonRow->addStretch();

    root->addStretch(1);
    root->addWidget(resultTitle_);
    root->addWidget(resultSummary_);
    root->addLayout(buttonRow);
    root->addStretch(2);

    connect(againButton, &QPushButton::clicked, this, [this] {
        if (currentPlayMode_ == PlayMode::SinglePlayer) {
            showSingleSetup();
        } else {
            showSetup();
        }
    });
    connect(menuButton, &QPushButton::clicked, this, &MainWindow::showMenu);
    return page;
}

void MainWindow::showMenu() {
    pages_->setCurrentWidget(menuPage_);
}

void MainWindow::showSingleSetup() {
    pages_->setCurrentWidget(singleSetupPage_);
}

void MainWindow::showSetup() {
    pages_->setCurrentWidget(setupPage_);
}

void MainWindow::showShop() {
    pages_->setCurrentWidget(shopPage_);
}

void MainWindow::showHelp() {
    pages_->setCurrentWidget(helpPage_);
}

void MainWindow::startSingleMatch() {
    const OutfitItem& p1Outfit = outfitByCatalogIndex(currentDataOrZero(singleP1OutfitCombo_));
    const RacketItem& p1Racket = racketByIndex(currentDataOrZero(singleP1RacketCombo_));
    const AiDifficulty difficulty = aiDifficultyFromCombo(aiDifficultyCombo_);

    int aiOutfitIndex = 0;
    int aiRacketIndex = 0;
    if (difficulty == AiDifficulty::Easy) {
        aiOutfitIndex = 0;
        aiRacketIndex = 0;
    } else if (difficulty == AiDifficulty::Medium) {
        aiOutfitIndex = 2;
        aiRacketIndex = 2;
    } else {
        aiOutfitIndex = 4;
        aiRacketIndex = 4;
    }

    const OutfitItem& aiOutfit = outfitByCatalogIndex(aiOutfitIndex);
    const RacketItem& aiRacket = racketByIndex(aiRacketIndex);
    currentPlayMode_ = PlayMode::SinglePlayer;
    gamePage_->startMatch(p1Outfit, p1Racket, aiOutfit, aiRacket,
                          PlayMode::SinglePlayer, difficulty);
    pages_->setCurrentWidget(gamePage_);
    gamePage_->setFocus();
}

void MainWindow::startDoubleMatch() {
    const OutfitItem& p1Outfit = outfitByCatalogIndex(currentDataOrZero(p1OutfitCombo_));
    const OutfitItem& p2Outfit = outfitByCatalogIndex(currentDataOrZero(p2OutfitCombo_));
    const RacketItem& p1Racket = racketByIndex(currentDataOrZero(p1RacketCombo_));
    const RacketItem& p2Racket = racketByIndex(currentDataOrZero(p2RacketCombo_));
    currentPlayMode_ = PlayMode::DoublePlayer;
    gamePage_->startMatch(p1Outfit, p1Racket, p2Outfit, p2Racket,
                          PlayMode::DoublePlayer, AiDifficulty::Medium);
    pages_->setCurrentWidget(gamePage_);
    gamePage_->setFocus();
}

void MainWindow::showResult(int winner, const QString& summary) {
    if (currentPlayMode_ == PlayMode::SinglePlayer) {
        if (winner == 1) {
            p1Store_.coins += 20;
            resultTitle_->setText(QStringLiteral("玩家获胜"));
            resultSummary_->setText(QStringLiteral("%1\nP1 获得 20 金币。").arg(summary));
        } else {
            resultTitle_->setText(QStringLiteral("电脑获胜"));
            resultSummary_->setText(QStringLiteral("%1\n单人模式中只有玩家获胜才获得金币。").arg(summary));
        }
        updateCoinLabels();
        refreshShopControls();
        pages_->setCurrentWidget(resultPage_);
        return;
    }

    storeFor(winner).coins += 20;
    updateCoinLabels();
    refreshShopControls();
    resultTitle_->setText(QStringLiteral("P%1 获胜").arg(winner));
    resultSummary_->setText(QStringLiteral("%1\nP%2 获得 20 金币。").arg(summary).arg(winner));
    pages_->setCurrentWidget(resultPage_);
}

Gender MainWindow::genderFromCombo(const QComboBox* combo) {
    return combo && combo->currentIndex() == 1 ? Gender::Female : Gender::Male;
}

AiDifficulty MainWindow::aiDifficultyFromCombo(const QComboBox* combo) {
    const int value = currentDataOrZero(combo);
    if (value == 0) {
        return AiDifficulty::Easy;
    }
    if (value == 2) {
        return AiDifficulty::Hard;
    }
    return AiDifficulty::Medium;
}

void MainWindow::initializeStores() {
    auto init = [](PlayerStore& store) {
        store.coins = 2000;
        store.ownedOutfits = {
            QStringLiteral("M_UI_1"),
            QStringLiteral("F_UI_1")
        };
        store.ownedRackets = {
            QStringLiteral("R_TRAINING")
        };
    };
    init(p1Store_);
    init(p2Store_);
}

PlayerStore& MainWindow::storeFor(int playerId) {
    return playerId == 1 ? p1Store_ : p2Store_;
}

const PlayerStore& MainWindow::storeFor(int playerId) const {
    return playerId == 1 ? p1Store_ : p2Store_;
}

bool MainWindow::buyOutfit(int playerId, int catalogIndex) {
    const OutfitItem& item = outfitByCatalogIndex(catalogIndex);
    PlayerStore& store = storeFor(playerId);
    if (store.ownedOutfits.contains(item.id)) {
        return false;
    }
    if (store.coins < item.price) {
        QMessageBox::information(this, QStringLiteral("金币不足"),
                                 QStringLiteral("P%1 金币不足，无法购买 %2。")
                                     .arg(playerId)
                                     .arg(item.name));
        refreshShopControls();
        return false;
    }

    store.coins -= item.price;
    store.ownedOutfits.insert(item.id);
    updateCoinLabels();
    refreshOutfitCombos();
    refreshShopControls();
    QMessageBox::information(this, QStringLiteral("购买成功"),
                             QStringLiteral("P%1 已购买 %2，花费 %3 金币。")
                                 .arg(playerId)
                                 .arg(item.name)
                                 .arg(item.price));
    return true;
}

bool MainWindow::buyRacket(int playerId, int racketIndex) {
    const RacketItem& item = racketByIndex(racketIndex);
    PlayerStore& store = storeFor(playerId);
    if (store.ownedRackets.contains(item.id)) {
        return false;
    }
    if (store.coins < item.price) {
        QMessageBox::information(this, QStringLiteral("金币不足"),
                                 QStringLiteral("P%1 金币不足，无法购买 %2。")
                                     .arg(playerId)
                                     .arg(item.name));
        refreshShopControls();
        return false;
    }

    store.coins -= item.price;
    store.ownedRackets.insert(item.id);
    updateCoinLabels();
    refreshOutfitCombos();
    refreshShopControls();
    QMessageBox::information(this, QStringLiteral("购买成功"),
                             QStringLiteral("P%1 已购买 %2，花费 %3 金币。")
                                 .arg(playerId)
                                 .arg(item.name)
                                 .arg(item.price));
    return true;
}

QLabel* MainWindow::createCoinLabel() {
    auto* label = new QLabel;
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet(QStringLiteral(
        "font-size:16px;font-weight:800;color:#f8fbfb;"
        "background:rgba(7,22,28,150);border:1px solid rgba(255,255,255,35);"
        "border-radius:7px;padding:8px 14px;"));
    coinLabels_.push_back(label);
    updateCoinLabels();
    return label;
}

void MainWindow::updateCoinLabels() {
    const QString text = QStringLiteral("P1 金币：%1        P2 金币：%2")
                             .arg(p1Store_.coins)
                             .arg(p2Store_.coins);
    for (QLabel* label : coinLabels_) {
        if (label) {
            label->setText(text);
        }
    }
}

void MainWindow::refreshShopControls() {
    for (const auto& refresh : shopRefreshers_) {
        if (refresh) {
            refresh();
        }
    }
}

void MainWindow::refreshOutfitCombos() {
    fillOutfitCombo(singleP1OutfitCombo_, genderFromCombo(singleP1GenderCombo_), 1);
    fillRacketCombo(singleP1RacketCombo_, 1);
    fillOutfitCombo(p1OutfitCombo_, genderFromCombo(p1GenderCombo_), 1);
    fillOutfitCombo(p2OutfitCombo_, genderFromCombo(p2GenderCombo_), 2);
    fillRacketCombo(p1RacketCombo_, 1);
    fillRacketCombo(p2RacketCombo_, 2);
}

void MainWindow::fillOutfitCombo(QComboBox* combo, Gender gender, int playerId) {
    if (!combo) {
        return;
    }
    const int oldData = currentDataOrZero(combo);
    combo->clear();
    int selectedIndex = 0;
    const auto& items = outfitCatalog();
    for (int i = 0; i < static_cast<int>(items.size()); ++i) {
        const OutfitItem& item = items[i];
        if (item.gender != gender) {
            continue;
        }
        if (!storeFor(playerId).ownedOutfits.contains(item.id)) {
            continue;
        }
        combo->addItem(QStringLiteral("%1  |  %2 金币  |  %3")
                           .arg(item.name)
                           .arg(item.price)
                           .arg(item.description),
                       i);
        if (i == oldData) {
            selectedIndex = combo->count() - 1;
        }
    }
    if (combo->count() > 0) {
        combo->setCurrentIndex(selectedIndex);
    }
}

void MainWindow::fillRacketCombo(QComboBox* combo, int playerId) {
    if (!combo) {
        return;
    }
    const int oldData = currentDataOrZero(combo);
    combo->clear();
    const auto& items = racketCatalog();
    int selectedIndex = 0;
    for (int i = 0; i < static_cast<int>(items.size()); ++i) {
        const RacketItem& item = items[i];
        if (!storeFor(playerId).ownedRackets.contains(item.id)) {
            continue;
        }
        combo->addItem(QStringLiteral("%1  |  %2 金币  |  命中+%3%  范围+%4")
                           .arg(item.name)
                           .arg(item.price)
                           .arg(static_cast<int>(item.hitBonus * 100.0))
                           .arg(item.rangeBonus, 0, 'f', 2),
                       i);
        if (i == oldData) {
            selectedIndex = combo->count() - 1;
        }
    }
    if (combo->count() > 0) {
        combo->setCurrentIndex(selectedIndex);
    }
}

int MainWindow::currentIndexOrZero(const QComboBox* combo) {
    if (!combo || combo->currentIndex() < 0) {
        return 0;
    }
    return combo->currentIndex();
}

int MainWindow::currentDataOrZero(const QComboBox* combo) {
    if (!combo || combo->currentIndex() < 0) {
        return 0;
    }
    bool ok = false;
    const int value = combo->currentData().toInt(&ok);
    return ok ? value : 0;
}
