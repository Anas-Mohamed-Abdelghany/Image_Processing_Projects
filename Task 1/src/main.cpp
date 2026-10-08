#include <QApplication>

#include "ui/MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QApplication::setApplicationName(QStringLiteral("CV Assignment"));
    QApplication::setOrganizationName(QStringLiteral("CV Assignment"));

    MainWindow window;
    window.show();

    return app.exec();
}
