#include "ItemCatalog.h"

#include <algorithm>

const std::vector<OutfitItem>& outfitCatalog() {
    static const std::vector<OutfitItem> items = {
        {
            QStringLiteral("M_UI_1"),
            QStringLiteral("凌风"),
            QStringLiteral("UI 文件夹中的男1蓝绿色网球服。"),
            Gender::Male,
            200,
            QColor(108, 205, 204),
            QColor(99, 55, 58),
            QColor(255, 253, 226),
            QColor(79, 158, 181),
            QColor(255, 253, 226),
            QColor(255, 255, 255),
            QColor(66, 157, 190),
            QColor(255, 255, 255),
            QColor(66, 157, 190),
            QColor(48, 85, 95),
            QStringLiteral("1"),
            QStringLiteral(":/assets/male1.png")
        },
        {
            QStringLiteral("M_UI_2"),
            QStringLiteral("曜辰"),
            QStringLiteral("UI 文件夹中的男2粉色网球服。"),
            Gender::Male,
            400,
            QColor(236, 118, 169),
            QColor(99, 55, 58),
            QColor(255, 237, 247),
            QColor(229, 94, 158),
            QColor(255, 237, 247),
            QColor(255, 255, 255),
            QColor(226, 83, 145),
            QColor(255, 255, 255),
            QColor(226, 83, 145),
            QColor(118, 50, 82),
            QStringLiteral("2"),
            QStringLiteral(":/assets/male2.png")
        },
        {
            QStringLiteral("M_UI_3"),
            QStringLiteral("擎苍"),
            QStringLiteral("UI 文件夹中的男3橙色网球服。"),
            Gender::Male,
            650,
            QColor(244, 151, 42),
            QColor(99, 55, 58),
            QColor(255, 239, 204),
            QColor(238, 133, 38),
            QColor(255, 239, 204),
            QColor(255, 255, 255),
            QColor(238, 133, 38),
            QColor(255, 255, 255),
            QColor(238, 133, 38),
            QColor(128, 64, 42),
            QStringLiteral("3"),
            QStringLiteral(":/assets/male3.png")
        },
        {
            QStringLiteral("M_UI_4"),
            QStringLiteral("锐锋"),
            QStringLiteral("UI 文件夹中的男4绿色网球服。"),
            Gender::Male,
            900,
            QColor(83, 186, 107),
            QColor(99, 55, 58),
            QColor(231, 250, 220),
            QColor(73, 171, 104),
            QColor(231, 250, 220),
            QColor(255, 255, 255),
            QColor(73, 171, 104),
            QColor(255, 255, 255),
            QColor(73, 171, 104),
            QColor(50, 105, 69),
            QStringLiteral("4"),
            QStringLiteral(":/assets/male4.png")
        },
        {
            QStringLiteral("M_UI_5"),
            QStringLiteral("跃动"),
            QStringLiteral("UI 文件夹中的男5深灰网球服。"),
            Gender::Male,
            1200,
            QColor(97, 103, 112),
            QColor(99, 55, 58),
            QColor(238, 239, 241),
            QColor(74, 80, 89),
            QColor(238, 239, 241),
            QColor(255, 255, 255),
            QColor(74, 80, 89),
            QColor(255, 255, 255),
            QColor(74, 80, 89),
            QColor(45, 53, 62),
            QStringLiteral("5"),
            QStringLiteral(":/assets/male5.png")
        },
        {
            QStringLiteral("F_UI_1"),
            QStringLiteral("绯影"),
            QStringLiteral("UI 文件夹中的女1蓝绿色网球服。"),
            Gender::Female,
            200,
            QColor(96, 169, 174),
            QColor(116, 76, 84),
            QColor(250, 239, 212),
            QColor(82, 158, 164),
            QColor(250, 239, 212),
            QColor(255, 255, 255),
            QColor(82, 158, 164),
            QColor(56, 91, 97),
            QColor(82, 158, 164),
            QColor(57, 92, 100),
            QStringLiteral("1"),
            QStringLiteral(":/assets/female1.png")
        },
        {
            QStringLiteral("F_UI_2"),
            QStringLiteral("云汐"),
            QStringLiteral("UI 文件夹中的女2粉色网球服。"),
            Gender::Female,
            400,
            QColor(96, 169, 174),
            QColor(116, 76, 84),
            QColor(255, 204, 228),
            QColor(229, 96, 164),
            QColor(255, 210, 232),
            QColor(255, 255, 255),
            QColor(229, 96, 164),
            QColor(119, 52, 83),
            QColor(224, 82, 143),
            QColor(132, 54, 88),
            QStringLiteral("2"),
            QStringLiteral(":/assets/female2.png")
        },
        {
            QStringLiteral("F_UI_3"),
            QStringLiteral("星芒"),
            QStringLiteral("UI 文件夹中的女3橙色网球服。"),
            Gender::Female,
            650,
            QColor(96, 169, 174),
            QColor(116, 76, 84),
            QColor(255, 220, 168),
            QColor(241, 134, 45),
            QColor(255, 225, 184),
            QColor(255, 255, 255),
            QColor(241, 134, 45),
            QColor(119, 63, 45),
            QColor(237, 124, 39),
            QColor(132, 58, 45),
            QStringLiteral("3"),
            QStringLiteral(":/assets/female3.png")
        },
        {
            QStringLiteral("F_UI_4"),
            QStringLiteral("风吟"),
            QStringLiteral("UI 文件夹中的女4绿色网球服。"),
            Gender::Female,
            900,
            QColor(96, 169, 174),
            QColor(116, 76, 84),
            QColor(202, 236, 195),
            QColor(75, 174, 103),
            QColor(212, 244, 204),
            QColor(255, 255, 255),
            QColor(75, 174, 103),
            QColor(55, 101, 69),
            QColor(75, 174, 103),
            QColor(50, 113, 76),
            QStringLiteral("4"),
            QStringLiteral(":/assets/female4.png")
        },
        {
            QStringLiteral("F_UI_5"),
            QStringLiteral("极光"),
            QStringLiteral("UI 文件夹中的女5深灰网球服。"),
            Gender::Female,
            1200,
            QColor(96, 169, 174),
            QColor(116, 76, 84),
            QColor(126, 132, 139),
            QColor(74, 80, 89),
            QColor(137, 143, 151),
            QColor(232, 236, 239),
            QColor(74, 80, 89),
            QColor(45, 52, 61),
            QColor(74, 80, 89),
            QColor(45, 53, 62),
            QStringLiteral("5"),
            QStringLiteral(":/assets/female5.png")
        }
    };
    return items;
}

const std::vector<RacketItem>& racketCatalog() {
    static const std::vector<RacketItem> items = {
        {
            QStringLiteral("R_TRAINING"),
            QStringLiteral("训练球拍"),
            QStringLiteral("粉色入门拍，稳定但加成较少。"),
            1000,
            0.03,
            0.05,
            1.00,
            QColor(238, 105, 164),
            QColor(255, 213, 232),
            QColor(255, 244, 250),
            QColor(88, 50, 70),
            1,
            QStringLiteral(":/assets/racket_pink.png")
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
            2,
            QStringLiteral(":/assets/racket_green.png")
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
            3,
            QStringLiteral(":/assets/racket_black.png")
        },
        {
            QStringLiteral("R_PRO"),
            QStringLiteral("专业球拍"),
            QStringLiteral("金黄色专业拍，适合更主动的进攻回球。"),
            2500,
            0.10,
            0.15,
            0.55,
            QColor(239, 185, 54),
            QColor(255, 238, 128),
            QColor(255, 245, 220),
            QColor(98, 70, 30),
            4,
            QStringLiteral(":/assets/racket_yellow.png")
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
            5,
            QStringLiteral(":/assets/racket_blue.png")
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
