#include "MainWindow.h"

#include <QApplication>
#include <QFont>
#include <QTimer>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Tennis Duel"));
    app.setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 10));

    MainWindow window;
    const bool smokeTest = app.arguments().contains(QStringLiteral("--smoke-test"));
    if (smokeTest) {
        QTimer::singleShot(120, &app, &QApplication::quit);
    } else {
        window.show();
    }

    return app.exec();
}
