#include "MainWindow.h"

#include <QApplication>
#include <QFrame>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
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
    setupPage_ = createSetupPage();
    shopPage_ = createShopPage();
    helpPage_ = createHelpPage();
    gamePage_ = new GameWidget;
    resultPage_ = createResultPage();

    pages_->addWidget(menuPage_);
    pages_->addWidget(setupPage_);
    pages_->addWidget(shopPage_);
    pages_->addWidget(helpPage_);
    pages_->addWidget(gamePage_);
    pages_->addWidget(resultPage_);

    gamePage_->setMatchFinishedHandler([this](int winner, const QString& summary) {
        showResult(winner, summary);
    });

    showMenu();
}

QWidget* MainWindow::createMenuPage() {
    auto* page = new QWidget;
    page->setStyleSheet(QStringLiteral("background:#152a31;"));

    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(90, 70, 90, 70);
    root->setSpacing(24);

    auto* title = makeTitle(QStringLiteral("Tennis Duel"));
    auto* subtitle = makeBody(QStringLiteral("Qt 双人网球对决。先完成本地双人、基础物理、计分和 UI；商店、签到、人机模式后续再扩展。"));
    auto* coins = createCoinLabel();

    auto* panel = makePanel();
    auto* panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(42, 34, 42, 34);
    panelLayout->setSpacing(16);

    auto* playButton = makeButton(QStringLiteral("双人模式"));
    auto* shopButton = makeButton(QStringLiteral("商店"));
    auto* helpButton = makeButton(QStringLiteral("操作说明"));
    auto* quitButton = makeButton(QStringLiteral("退出"));

    panelLayout->addWidget(playButton);
    panelLayout->addWidget(shopButton);
    panelLayout->addWidget(helpButton);
    panelLayout->addWidget(quitButton);

    root->addStretch(1);
    root->addWidget(title);
    root->addWidget(subtitle);
    root->addWidget(coins);
    root->addSpacing(12);
    root->addWidget(panel, 0, Qt::AlignHCenter);
    root->addStretch(2);

    panel->setFixedWidth(420);

    connect(playButton, &QPushButton::clicked, this, &MainWindow::showSetup);
    connect(shopButton, &QPushButton::clicked, this, &MainWindow::showShop);
    connect(helpButton, &QPushButton::clicked, this, &MainWindow::showHelp);
    connect(quitButton, &QPushButton::clicked, qApp, &QApplication::quit);

    return page;
}

QWidget* MainWindow::createSetupPage() {
    auto* page = new QWidget;
    page->setStyleSheet(QStringLiteral("background:#19323a;"));

    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(100, 64, 100, 64);
    root->setSpacing(20);

    root->addWidget(makeTitle(QStringLiteral("选择双人角色与商品"), 30));
    root->addWidget(makeBody(QStringLiteral("先选择男生或女生，再选择该性别下已拥有的网球服。球拍仍按玩家各自拥有情况显示。")));
    root->addWidget(createCoinLabel());

    auto* panel = makePanel();
    auto* form = new QVBoxLayout(panel);
    form->setContentsMargins(44, 32, 44, 32);
    form->setSpacing(18);

    auto makeComboRow = [&](const QString& name, QComboBox** combo) {
        auto* row = new QHBoxLayout;
        auto* label = new QLabel(name);
        label->setStyleSheet(QStringLiteral("font-size:18px;font-weight:700;color:#f7fbfb;"));
        *combo = new QComboBox;
        (*combo)->setMinimumHeight(38);
        (*combo)->setMinimumWidth(360);
        (*combo)->setStyleSheet(QStringLiteral("font-size:16px;padding:5px;"));
        row->addWidget(label);
        row->addStretch();
        row->addWidget(*combo, 0);
        form->addLayout(row);
    };

    makeComboRow(QStringLiteral("P1 角色"), &p1GenderCombo_);
    p1GenderCombo_->addItem(QStringLiteral("男生"), QVariant::fromValue(0));
    p1GenderCombo_->addItem(QStringLiteral("女生"), QVariant::fromValue(1));
    makeComboRow(QStringLiteral("P1 服装"), &p1OutfitCombo_);
    makeComboRow(QStringLiteral("P1 球拍"), &p1RacketCombo_);

    form->addSpacing(8);

    makeComboRow(QStringLiteral("P2 角色"), &p2GenderCombo_);
    p2GenderCombo_->addItem(QStringLiteral("男生"), QVariant::fromValue(0));
    p2GenderCombo_->addItem(QStringLiteral("女生"), QVariant::fromValue(1));
    makeComboRow(QStringLiteral("P2 服装"), &p2OutfitCombo_);
    makeComboRow(QStringLiteral("P2 球拍"), &p2RacketCombo_);

    p2GenderCombo_->setCurrentIndex(1);
    refreshOutfitCombos();
    p2RacketCombo_->setCurrentIndex(0);

    auto* buttonRow = new QHBoxLayout;
    auto* backButton = makeButton(QStringLiteral("返回"));
    auto* startButton = makeButton(QStringLiteral("开始比赛"));
    buttonRow->addWidget(backButton);
    buttonRow->addWidget(startButton);
    form->addSpacing(8);
    form->addLayout(buttonRow);

    root->addStretch(1);
    root->addWidget(panel, 0, Qt::AlignHCenter);
    root->addStretch(2);
    panel->setFixedWidth(720);

    connect(backButton, &QPushButton::clicked, this, &MainWindow::showMenu);
    connect(startButton, &QPushButton::clicked, this, &MainWindow::startDoubleMatch);
    connect(p1GenderCombo_, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this] {
        fillOutfitCombo(p1OutfitCombo_, genderFromCombo(p1GenderCombo_), 1);
    });
    connect(p2GenderCombo_, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, [this] {
        fillOutfitCombo(p2OutfitCombo_, genderFromCombo(p2GenderCombo_), 2);
    });

    return page;
}

QWidget* MainWindow::createShopPage() {
    shopRefreshers_.clear();

    auto* page = new QWidget;
    page->setStyleSheet(QStringLiteral("background:#173038;"));

    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(42, 36, 42, 32);
    root->setSpacing(14);

    root->addWidget(makeTitle(QStringLiteral("商店"), 30));
    root->addWidget(makeBody(QStringLiteral("P1 和 P2 金币相互独立。每张商品卡都可以分别给 P1 或 P2 购买。")));
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
    auto* page = new QWidget;
    page->setStyleSheet(QStringLiteral("background:#13282f;"));
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(110, 68, 110, 68);
    root->setSpacing(22);

    root->addWidget(makeTitle(QStringLiteral("操作说明"), 30));

    auto* panel = makePanel();
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(40, 34, 40, 34);
    layout->setSpacing(18);

    layout->addWidget(makeBody(QStringLiteral(
        "P1：W / A / S / D 移动，Space 击球。\n"
        "P2：方向键移动，J 击球。\n\n"
        "比赛采用三局两胜；局内使用 0 / 15 / 30 / 40 / Deuce / Advantage 的网球计分。"
        "网球使用 x/y/z 坐标和重力模拟抛物线，落地阴影显示球的水平位置。")));

    auto* backButton = makeButton(QStringLiteral("返回主菜单"));
    layout->addWidget(backButton);
    root->addWidget(panel);
    root->addStretch();

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

    connect(againButton, &QPushButton::clicked, this, &MainWindow::showSetup);
    connect(menuButton, &QPushButton::clicked, this, &MainWindow::showMenu);
    return page;
}

void MainWindow::showMenu() {
    pages_->setCurrentWidget(menuPage_);
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

void MainWindow::startDoubleMatch() {
    const OutfitItem& p1Outfit = outfitByCatalogIndex(currentDataOrZero(p1OutfitCombo_));
    const OutfitItem& p2Outfit = outfitByCatalogIndex(currentDataOrZero(p2OutfitCombo_));
    const RacketItem& p1Racket = racketByIndex(currentDataOrZero(p1RacketCombo_));
    const RacketItem& p2Racket = racketByIndex(currentDataOrZero(p2RacketCombo_));
    gamePage_->startMatch(p1Outfit, p1Racket, p2Outfit, p2Racket);
    pages_->setCurrentWidget(gamePage_);
    gamePage_->setFocus();
}

void MainWindow::showResult(int winner, const QString& summary) {
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

void MainWindow::initializeStores() {
    auto init = [](PlayerStore& store) {
        store.coins = 3000;
        store.ownedOutfits = {
            QStringLiteral("M_SKY_ACADEMY"),
            QStringLiteral("F_PINK_BUNNY")
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
