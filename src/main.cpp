#include <QCoreApplication>
#include "httpstreamer.hpp"
#include <gst/gst.h>

int main(int argc, char *argv[])
{
    gst_init(&argc, &argv);

    QCoreApplication a(argc, argv);
    HttpStreamer *pStreamer = new HttpStreamer();
    pStreamer->start("192.168.4.129", 8000);

    return a.exec();
}
