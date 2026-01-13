#include "BCGP.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    BCGP window;
    window.show();
    return app.exec();
}
