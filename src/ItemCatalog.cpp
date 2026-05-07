#include "ItemCatalog.h"

#include <algorithm>

const std::vector<OutfitItem>& outfitCatalog() {
    static const std::vector<OutfitItem> items = {
        {
            QStringLiteral("M_SKY_ACADEMY"),
            QStringLiteral("男款 云朵学院套装"),
            QStringLiteral("蓝白配色的学院风网球服，带小云朵胸标和白色短裤。"),
            Gender::Male,
            200,
            QColor(232, 246, 255),
            QColor(73, 45, 30),
            QColor(80, 168, 236),
            QColor(248, 253, 255),
            QColor(154, 212, 247),
            QColor(255, 255, 255),
            QColor(245, 249, 252),
            QColor(92, 166, 231),
            QColor(255, 255, 255),
            QColor(42, 105, 190),
            QStringLiteral("☁")
        },
        {
            QStringLiteral("M_MINT_SERVE"),
            QStringLiteral("男款 薄荷发球套装"),
            QStringLiteral("薄荷绿上衣、海军蓝短裤和运动头带，清爽可爱。"),
            Gender::Male,
            600,
            QColor(121, 225, 186),
            QColor(52, 37, 28),
            QColor(119, 218, 184),
            QColor(255, 255, 255),
            QColor(182, 244, 222),
            QColor(240, 255, 249),
            QColor(42, 76, 124),
            QColor(127, 215, 188),
            QColor(245, 250, 255),
            QColor(20, 116, 98),
            QStringLiteral("T")
        },
        {
            QStringLiteral("M_STAR_CAPTAIN"),
            QStringLiteral("男款 星星队长套装"),
            QStringLiteral("红白职业感网球服，金色星星细节，偏珍稀款。"),
            Gender::Male,
            1200,
            QColor(255, 219, 95),
            QColor(44, 32, 28),
            QColor(232, 76, 87),
            QColor(255, 244, 224),
            QColor(250, 136, 142),
            QColor(255, 252, 238),
            QColor(255, 255, 255),
            QColor(232, 76, 87),
            QColor(255, 245, 225),
            QColor(245, 190, 62),
            QStringLiteral("★")
        },
        {
            QStringLiteral("F_PINK_BUNNY"),
            QStringLiteral("女款 樱桃兔兔套装"),
            QStringLiteral("粉白网球裙、兔耳遮阳帽和樱桃胸标，甜美风。"),
            Gender::Female,
            200,
            QColor(255, 195, 216),
            QColor(105, 73, 48),
            QColor(255, 136, 179),
            QColor(255, 248, 251),
            QColor(255, 190, 213),
            QColor(255, 255, 255),
            QColor(255, 229, 239),
            QColor(255, 116, 166),
            QColor(255, 255, 255),
            QColor(229, 71, 132),
            QStringLiteral("♡")
        },
        {
            QStringLiteral("F_LEMON_SPIN"),
            QStringLiteral("女款 柠檬旋风套装"),
            QStringLiteral("柠檬黄运动上衣、百褶裙和白色发带，活力轻快。"),
            Gender::Female,
            650,
            QColor(255, 234, 105),
            QColor(70, 46, 28),
            QColor(255, 218, 73),
            QColor(255, 255, 255),
            QColor(255, 239, 140),
            QColor(255, 255, 246),
            QColor(255, 252, 210),
            QColor(88, 196, 158),
            QColor(250, 255, 245),
            QColor(243, 178, 42),
            QStringLiteral("L")
        },
        {
            QStringLiteral("F_RAINBOW_SMASH"),
            QStringLiteral("女款 彩虹扣杀套装"),
            QStringLiteral("浅紫职业网球裙，彩虹侧边条纹和星光帽檐，珍稀款。"),
            Gender::Female,
            1300,
            QColor(207, 184, 255),
            QColor(42, 32, 50),
            QColor(167, 133, 245),
            QColor(255, 255, 255),
            QColor(214, 196, 255),
            QColor(255, 255, 255),
            QColor(235, 225, 255),
            QColor(255, 174, 207),
            QColor(255, 251, 255),
            QColor(113, 224, 226),
            QStringLiteral("✦")
        }
    };
    return items;
}

const std::vector<RacketItem>& racketCatalog() {
    static const std::vector<RacketItem> items = {
        {
            QStringLiteral("R_TRAINING"),
            QStringLiteral("训练球拍"),
            QStringLiteral("轻便入门拍，稳定但加成较少。"),
            1000,
            0.03,
            0.05,
            1.00,
            QColor(86, 145, 220),
            QColor(230, 244, 255),
            QColor(238, 246, 250),
            QColor(42, 57, 77),
            1
        },
        {
            QStringLiteral("R_FEATHER"),
            QStringLiteral("轻羽球拍"),
            QStringLiteral("浅绿轻量拍，击球更容易，控球略有提升。"),
            1500,
            0.05,
            0.08,
            0.85,
            QColor(93, 204, 159),
            QColor(244, 255, 246),
            QColor(240, 250, 244),
            QColor(48, 88, 72),
            2
        },
        {
            QStringLiteral("R_CARBON"),
            QStringLiteral("碳素球拍"),
            QStringLiteral("黑银碳素框，击球范围和命中率更好。"),
            2000,
            0.07,
            0.11,
            0.70,
            QColor(56, 62, 72),
            QColor(190, 205, 219),
            QColor(234, 239, 244),
            QColor(35, 39, 46),
            3
        },
        {
            QStringLiteral("R_PRO"),
            QStringLiteral("专业球拍"),
            QStringLiteral("红金专业拍，适合更主动的进攻回球。"),
            2500,
            0.10,
            0.15,
            0.55,
            QColor(224, 61, 77),
            QColor(255, 211, 94),
            QColor(255, 245, 220),
            QColor(84, 38, 42),
            4
        },
        {
            QStringLiteral("R_LEGEND"),
            QStringLiteral("传说球拍"),
            QStringLiteral("星蓝传说拍，最高命中加成和最佳控球。"),
            5000,
            0.15,
            0.22,
            0.35,
            QColor(76, 92, 238),
            QColor(121, 239, 255),
            QColor(244, 252, 255),
            QColor(34, 38, 82),
            5
        }
    };
    return items;
}

std::vector<int> outfitIndexesForGender(Gender gender) {
    std::vector<int> indexes;
    const auto& items = outfitCatalog();
    for (int i = 0; i < static_cast<int>(items.size()); ++i) {
        if (items[i].gender == gender) {
            indexes.push_back(i);
        }
    }
    return indexes;
}

const OutfitItem& outfitByCatalogIndex(int index) {
    const auto& items = outfitCatalog();
    index = std::clamp(index, 0, static_cast<int>(items.size()) - 1);
    return items[index];
}

const OutfitItem& outfitByGenderIndex(Gender gender, int genderIndex) {
    const auto indexes = outfitIndexesForGender(gender);
    if (indexes.empty()) {
        return outfitCatalog().front();
    }
    genderIndex = std::clamp(genderIndex, 0, static_cast<int>(indexes.size()) - 1);
    return outfitByCatalogIndex(indexes[genderIndex]);
}

const RacketItem& racketByIndex(int index) {
    const auto& items = racketCatalog();
    index = std::clamp(index, 0, static_cast<int>(items.size()) - 1);
    return items[index];
}
