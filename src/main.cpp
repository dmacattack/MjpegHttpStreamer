#include <QCoreApplication>
#include "httpstreamer.hpp"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    HttpStreamer *pStreamer = new HttpStreamer();
    pStreamer->start("192.168.4.126", 8000);

    return a.exec();
}
