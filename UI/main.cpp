#include <QApplication>
#include <QScreen>
#include <QFont>
#include "windows/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("BookClub");
    app.setOrganizationName("BookClub");

    // Crisp rendering on high-DPI displays
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);

    // Bold-leaning base font — stylesheet overrides individual weights
    QFont baseFont("Segoe UI", 10, QFont::Normal);
    baseFont.setStyleStrategy(
        QFont::StyleStrategy(QFont::PreferAntialias | QFont::PreferQuality));
    app.setFont(baseFont);

    MainWindow window;

    // Center on screen
    const QRect screenGeometry = app.primaryScreen()->availableGeometry();
    window.move(
        screenGeometry.center().x() - window.width()  / 2,
        screenGeometry.center().y() - window.height() / 2
    );

    window.show();
    return app.exec();
}
