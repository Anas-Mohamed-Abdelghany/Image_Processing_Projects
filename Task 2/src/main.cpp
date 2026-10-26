#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    // Modern Dark Theme styling for clean UI presentation
    app.setStyle("Fusion");

    MainWindow window;
    window.show();

    return app.exec();
}