#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Jingle Palette Pro");
    app.setApplicationVersion("4.4.5");
    app.setOrganizationName("Jingle Palette Pro");
    app.setWindowIcon(QIcon(":/logo.png"));

    // Set working directory to the executable directory
    QDir::setCurrent(QCoreApplication::applicationDirPath());

    MainWindow w;
    w.show();

    return app.exec();
}
