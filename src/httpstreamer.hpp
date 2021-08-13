#ifndef HTTPSTREAMER_HPP
#define HTTPSTREAMER_HPP

#include <QObject>
#include <QString>
#include <QTcpServer>
#include <QHostAddress>
#include <gst/gst.h>
#include <string.h>

class HttpStreamer
: QObject
{
    Q_OBJECT
public:
    HttpStreamer();
    ~HttpStreamer();

    void start(QHostAddress address, int port);
    void start(QString ipAddress, int port);
    void start();
    void stop();

signals:

private slots:
    void onNewTcpConnection();

private:
    char *pullFrame();
    void saveToFile(char* buf, int sz, QString filename);
    void saveToFile(QByteArray buf, QString filename);

private:
    QTcpServer *mpTcpServer;
    GstElement *mpPipeline;
};

#endif // HTTPSTREAMER_HPP
