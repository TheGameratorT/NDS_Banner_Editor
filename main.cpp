#include "mainwindow.h"

#include <QApplication>
#include <QTranslator>
#include <QtGlobal>

MainWindow *w;
QTranslator *translator;
bool translationLoaded = false;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName("TheGameratorT");
    QCoreApplication::setApplicationName("NDS_Banner_Editor");

    translator = new QTranslator(&a);

    w = new MainWindow();
    w->show();
    return a.exec();
}
