#pragma once

#include "GameTypes.h"
#include "GameWidget.h"
#include "ItemCatalog.h"

#include <QComboBox>
#include <QLabel>
#include <QMainWindow>
#include <QSet>
#include <QStackedWidget>

#include <functional>
#include <vector>

struct PlayerStore {
    int coins = 3000;
    QSet<QString> ownedOutfits;
    QSet<QString> ownedRackets;
};

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    QWidget* createMenuPage();
    QWidget* createSetupPage();
    QWidget* createShopPage();
    QWidget* createHelpPage();
    QWidget* createResultPage();

    void showMenu();
    void showSetup();
    void showShop();
    void showHelp();
    void startDoubleMatch();
    void showResult(int winner, const QString& summary);
    void initializeStores();
    PlayerStore& storeFor(int playerId);
    const PlayerStore& storeFor(int playerId) const;
    bool buyOutfit(int playerId, int catalogIndex);
    bool buyRacket(int playerId, int racketIndex);
    QLabel* createCoinLabel();
    void updateCoinLabels();
    void refreshShopControls();
    void refreshOutfitCombos();
    void fillOutfitCombo(QComboBox* combo, Gender gender, int playerId);
    void fillRacketCombo(QComboBox* combo, int playerId);

    static Gender genderFromCombo(const QComboBox* combo);
    static int currentIndexOrZero(const QComboBox* combo);
    static int currentDataOrZero(const QComboBox* combo);

    QStackedWidget* pages_ = nullptr;
    QWidget* menuPage_ = nullptr;
    QWidget* setupPage_ = nullptr;
    QWidget* shopPage_ = nullptr;
    QWidget* helpPage_ = nullptr;
    QWidget* resultPage_ = nullptr;
    GameWidget* gamePage_ = nullptr;
    QComboBox* p1GenderCombo_ = nullptr;
    QComboBox* p2GenderCombo_ = nullptr;
    QComboBox* p1OutfitCombo_ = nullptr;
    QComboBox* p2OutfitCombo_ = nullptr;
    QComboBox* p1RacketCombo_ = nullptr;
    QComboBox* p2RacketCombo_ = nullptr;
    QLabel* resultTitle_ = nullptr;
    QLabel* resultSummary_ = nullptr;
    PlayerStore p1Store_;
    PlayerStore p2Store_;
    std::vector<QLabel*> coinLabels_;
    std::vector<std::function<void()>> shopRefreshers_;
};
