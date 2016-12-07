#include "fml.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    FML w;
    w.show();
    return a.exec();
}
