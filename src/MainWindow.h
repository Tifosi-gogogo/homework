#pragma once

#include "GameTypes.h"
#include "GameWidget.h"

#include <QComboBox>
#include <QLabel>
#include <QMainWindow>
#include <QStackedWidget>

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    QWidget* createMenuPage();
    QWidget* createSetupPage();
    QWidget* createHelpPage();
    QWidget* createResultPage();

    void showMenu();
    void showSetup();
    void showHelp();
    void startDoubleMatch();
    void showResult(int winner, const QString& summary);

    static Gender genderFromCombo(const QComboBox* combo);

    QStackedWidget* pages_ = nullptr;
    QWidget* menuPage_ = nullptr;
    QWidget* setupPage_ = nullptr;
    QWidget* helpPage_ = nullptr;
    QWidget* resultPage_ = nullptr;
    GameWidget* gamePage_ = nullptr;
    QComboBox* p1GenderCombo_ = nullptr;
    QComboBox* p2GenderCombo_ = nullptr;
    QLabel* resultTitle_ = nullptr;
    QLabel* resultSummary_ = nullptr;
};
