# Tennis Duel

Qt 6 Widgets + C++17 实现的 2D 网球双人对战原型。

当前版本先完成基础可玩内容：

- 本地双人模式。
- P1 使用 `W/A/S/D` 移动，`J` 击球。
- P2 使用方向键移动，`Enter` 击球。
- `Space` 是全局击球键，系统会自动选择当前可接球的一方。
- 网球使用 `x/y/z` 坐标、重力和初速度反推实现抛物线。
- 绘制网球阴影，表现球的水平位置和高度。
- 支持下网、出界、二次落地判分。
- 局内使用 `0 / 15 / 30 / 40 / Deuce / Advantage` 计分。
- 比赛采用三局两胜，先赢 2 局者获胜。
- 基础 UI：主菜单、角色选择、操作说明、比赛界面、结算界面。
- 美术暂用 `QPainter` 自绘球场、人物、球拍、网球和 UI，避免外部素材版权问题。

暂未实现：

- 单人人机模式。
- 商店和球拍购买。
- 每日签到。
- 金币结算。
- 存档系统。

## 开发环境

- Qt 6.11.0
- CMake
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
$env:Path='C:\Qt\6.11.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;C:\Qt\Tools\Ninja;' + $env:Path
& C:\Qt\Tools\CMake_64\bin\cmake.exe -S . -B build_qt -G Ninja -DCMAKE_PREFIX_PATH=C:/Qt/6.11.0/mingw_64 -DCMAKE_CXX_COMPILER=C:/Qt/Tools/mingw1310_64/bin/g++.exe -DCMAKE_MAKE_PROGRAM=C:/Qt/Tools/Ninja/ninja.exe
& C:\Qt\Tools\CMake_64\bin\cmake.exe --build build_qt
```

运行：

```powershell
$env:Path='C:\Qt\6.11.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;' + $env:Path
.\build_qt\TennisDuel.exe
```

离屏冒烟测试：

```powershell
$env:Path='C:\Qt\6.11.0\mingw_64\bin;C:\Qt\Tools\mingw1310_64\bin;' + $env:Path
$env:QT_QPA_PLATFORM='offscreen'
.\build_qt\TennisDuel.exe --smoke-test
```

## 代码结构

```text
src/
  main.cpp
  MainWindow.h/.cpp       主窗口、菜单、角色选择、结算页
  GameWidget.h/.cpp       比赛页面、键盘事件、QTimer 刷新
  MatchController.h/.cpp  网球物理、计分、判分、绘制
  GameTypes.h             基础枚举、向量、输入状态
```

## 备注

当前工程不使用自定义 Qt 信号和 `moc`，而是用普通 C++ 回调处理比赛结束事件。这样在包含中文字符的项目路径下构建更稳定。
