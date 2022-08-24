#include "mainwindow.h"

#include <QApplication>
#include <QTranslator>
#include <QtGlobal>

MainWindow *w;

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator t;
    if (t.load(QLocale(), QLatin1String("nbe"), QLatin1String("_"), QLatin1String(":/resources/i18n")))
        a.installTranslator(&t);

    w = new MainWindow();
    w->show();
    return a.exec();
}
