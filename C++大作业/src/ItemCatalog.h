#pragma once

#include "GameTypes.h"

#include <QColor>
#include <QString>

#include <vector>

struct OutfitItem {
    QString id;
    QString name;
    QString description;
    Gender gender = Gender::Male;
    int price = 0;

    QColor hatColor;
    QColor hairColor;
    QColor shirtMain;
    QColor shirtAccent;
    QColor sleeveColor;
    QColor collarColor;
    QColor bottomMain;
    QColor bottomAccent;
    QColor shoeColor;
    QColor stripeColor;
    QString chestMark;
    QString imagePath;
};

struct RacketItem {
    QString id;
    QString name;
    QString description;
    int price = 0;
    double hitBonus = 0.0;
    double rangeBonus = 0.0;
    double controlError = 1.2;

    QColor frameColor;
    QColor accentColor;
    QColor stringColor;
    QColor gripColor;
    int starLevel = 1;
    QString imagePath;
};

const std::vector<OutfitItem>& outfitCatalog();
const std::vector<RacketItem>& racketCatalog();

std::vector<int> outfitIndexesForGender(Gender gender);
const OutfitItem& outfitByCatalogIndex(int index);
const OutfitItem& outfitByGenderIndex(Gender gender, int genderIndex);
const RacketItem& racketByIndex(int index);
