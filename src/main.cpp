#include "mainwindow.h"

#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("Stratovise");
    QApplication::setApplicationVersion("0.2.0");
    QApplication::setOrganizationName("Blastmaster-Fixup-and-Kleanup-Crew");
    app.setStyle(QStyleFactory::create("Fusion"));

    MainWindow window;
    window.show();
    return app.exec();
}
