# Tennis Duel

Qt 6 Widgets + C++17 实现的 2D 网球对战游戏。

## 当前功能

- 单人模式：P1 对战电脑。
- 单人模式有三档难度：简单、普通、困难。
- 难度会影响电脑跑位速度、接球反应、命中概率和回球变化。
- 双人模式：本地双人对战。
- P1 使用 `W/A/S/D` 移动，`Space` 发球和击球。
- P2 使用方向键移动，`J` 发球和击球。
- 网球使用 `x/y/z` 坐标、重力和初速度反推实现抛物线。
- 绘制网球阴影，表现球的水平位置和高度。
- 支持下网、出界、二次落地判分。
- 局内使用 `0 / 15 / 30 / 40 / Deuce / Advantage` 计分。
- 比赛采用三局两胜，先赢 2 局者获胜。
- 主页面包含单人模式、双人模式、商店、操作说明、退出。
- 商店按三行展示：女生服装、男生服装、网球拍。
- 男女人物各 5 套卡通网球服，网球拍 5 副。
- P1/P2 金币分开计算，默认各 2000 金币。
- 每个玩家初始拥有男/女基础套装各 1 套和训练球拍 1 副，其余商品需要在商店购买。
- 商店支持分别给 P1/P2 购买商品，购买后扣除对应玩家金币。
- 角色选择页只显示该玩家已拥有的套装和球拍。
- 赢得比赛的玩家获得 20 金币；单人模式只有 P1 战胜电脑时获得金币。
- 使用本地 `save.json` 保存金币和已拥有商品，重启游戏后会自动读取。
- 每局比赛开场会随机选择不同球场配色，增加场景变化。
- 美术暂用 `QPainter` 自绘球场、人物、球拍、网球和 UI，避免外部素材版权问题。

## 暂未实现

- 每日签到。

## 开发环境

- Qt 6.11.0
- CMake
- Ninja
- MinGW 64-bit
- C++17

本机可用路径：

- Qt: `C:\Qt\6.11.0\mingw_64`
- CMake: `C:\Qt\Tools\CMake_64\bin\cmake.exe`
- Ninja: `C:\Qt\Tools\Ninja\ninja.exe`
- g++: `C:\Qt\Tools\mingw1310_64\bin\g++.exe`

## 构建方式

使用 Qt Creator：

1. 打开 `CMakeLists.txt`。
2. 选择 Qt 6 MinGW 64-bit Kit。
3. Configure。
4. Build。
5. Run。

命令行构建：

```powershell
cd "C:\Users\50463\OneDrive\Desktop\还原糖の大一下\C++大作业"
$env:Path='C:\Qt\6.11.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\Ninja;' + $env:Path
& C:\Qt\Tools\CMake_64\bin\cmake.exe -S . -B build_qt -G Ninja -DCMAKE_PREFIX_PATH=C:/Qt/6.11.0/mingw_64 -DCMAKE_CXX_COMPILER=C:/Qt/Tools/mingw1310_64/bin/g++.exe -DCMAKE_MAKE_PROGRAM=C:/Qt/Tools/Ninja/ninja.exe
& C:\Qt\Tools\CMake_64\bin\cmake.exe --build build_qt
```

运行：

```powershell
cd "C:\Users\50463\OneDrive\Desktop\还原糖の大一下\C++大作业"
$env:Path='C:\Qt\6.11.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;' + $env:Path
.\build_qt\TennisDuel.exe
```

离屏冒烟测试：

```powershell
cd "C:\Users\50463\OneDrive\Desktop\还原糖の大一下\C++大作业"
$env:Path='C:\Qt\6.11.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;' + $env:Path
$env:QT_QPA_PLATFORM='offscreen'
$env:QT_QPA_FONTDIR='C:\Windows\Fonts'
.\build_qt\TennisDuel.exe --smoke-test
```

## 代码结构

```text
src/
  main.cpp
  MainWindow.h/.cpp       主窗口、菜单、单人/双人设置、商店、结算页
  GameWidget.h/.cpp       比赛页面、键盘事件、QTimer 刷新
  MatchController.h/.cpp  网球物理、AI、计分、判分、绘制
  GameTypes.h             基础枚举、向量、输入状态
  ItemCatalog.h/.cpp      服装和球拍商品目录
```

## 备注

当前工程不使用自定义 Qt 信号和 `moc`，而是用普通 C++ 回调处理比赛结束事件。这样在包含中文字符的项目路径下构建更稳定。
