#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
        app.setApplicationName("Jingle Palette Pro");
        app.setApplicationVersion("0.1");
        app.setOrganizationName("Jingle Palette Pro");

    // Set working directory to the executable directory
    QDir::setCurrent(QCoreApplication::applicationDirPath());

    MainWindow w;
    w.show();

    return app.exec();
}
