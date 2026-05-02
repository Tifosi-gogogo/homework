#include "MainWindow.h"

#include <QApplication>
#include <QFrame>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVariant>
#include <QVBoxLayout>

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

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("Tennis Duel - Qt"));
    resize(1280, 720);

    pages_ = new QStackedWidget(this);
    setCentralWidget(pages_);

    menuPage_ = createMenuPage();
    setupPage_ = createSetupPage();
    helpPage_ = createHelpPage();
    gamePage_ = new GameWidget;
    resultPage_ = createResultPage();

    pages_->addWidget(menuPage_);
    pages_->addWidget(setupPage_);
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

    auto* panel = makePanel();
    auto* panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(42, 34, 42, 34);
    panelLayout->setSpacing(16);

    auto* playButton = makeButton(QStringLiteral("双人模式"));
    auto* helpButton = makeButton(QStringLiteral("操作说明"));
    auto* quitButton = makeButton(QStringLiteral("退出"));

    panelLayout->addWidget(playButton);
    panelLayout->addWidget(helpButton);
    panelLayout->addWidget(quitButton);

    root->addStretch(1);
    root->addWidget(title);
    root->addWidget(subtitle);
    root->addSpacing(12);
    root->addWidget(panel, 0, Qt::AlignHCenter);
    root->addStretch(2);

    panel->setFixedWidth(420);

    connect(playButton, &QPushButton::clicked, this, &MainWindow::showSetup);
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

    root->addWidget(makeTitle(QStringLiteral("选择双人角色"), 30));
    root->addWidget(makeBody(QStringLiteral("第一版提供免费基础外观。P1 与 P2 可选择男/女角色，比赛内会显示不同服装轮廓。")));

    auto* panel = makePanel();
    auto* form = new QVBoxLayout(panel);
    form->setContentsMargins(44, 32, 44, 32);
    form->setSpacing(18);

    auto makeRow = [&](const QString& name, QComboBox** combo) {
        auto* row = new QHBoxLayout;
        auto* label = new QLabel(name);
        label->setStyleSheet(QStringLiteral("font-size:18px;font-weight:700;color:#f7fbfb;"));
        *combo = new QComboBox;
        (*combo)->addItem(QStringLiteral("男"), QVariant::fromValue(0));
        (*combo)->addItem(QStringLiteral("女"), QVariant::fromValue(1));
        (*combo)->setMinimumHeight(38);
        (*combo)->setStyleSheet(QStringLiteral("font-size:16px;padding:5px;"));
        row->addWidget(label);
        row->addStretch();
        row->addWidget(*combo);
        form->addLayout(row);
    };

    makeRow(QStringLiteral("P1 角色"), &p1GenderCombo_);
    makeRow(QStringLiteral("P2 角色"), &p2GenderCombo_);
    p2GenderCombo_->setCurrentIndex(1);

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
    panel->setFixedWidth(520);

    connect(backButton, &QPushButton::clicked, this, &MainWindow::showMenu);
    connect(startButton, &QPushButton::clicked, this, &MainWindow::startDoubleMatch);

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
        "P1：W / A / S / D 移动，J 击球。\n"
        "P2：方向键移动，Enter 击球。\n"
        "Space：全局击球键，系统会自动选择当前最适合接球的一方。\n\n"
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

void MainWindow::showHelp() {
    pages_->setCurrentWidget(helpPage_);
}

void MainWindow::startDoubleMatch() {
    gamePage_->startMatch(genderFromCombo(p1GenderCombo_), genderFromCombo(p2GenderCombo_));
    pages_->setCurrentWidget(gamePage_);
    gamePage_->setFocus();
}

void MainWindow::showResult(int winner, const QString& summary) {
    resultTitle_->setText(QStringLiteral("P%1 获胜").arg(winner));
    resultSummary_->setText(QStringLiteral("%1\n本阶段暂不结算金币，商店、签到和单人人机会在后续版本实现。").arg(summary));
    pages_->setCurrentWidget(resultPage_);
}

Gender MainWindow::genderFromCombo(const QComboBox* combo) {
    return combo->currentIndex() == 0 ? Gender::Male : Gender::Female;
}
