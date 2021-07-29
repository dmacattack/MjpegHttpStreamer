#include "httpstreamer.hpp"
#include <QTcpSocket>
#include <QDebug>
#include <QImage>
#include <QThread>
#include <QFile>
#include <QDataStream>

// anonymous namespace
namespace
{
    const int DEFAULT_PORT = 8080;
    const QByteArray HTTP_HEADER = "HTTP/1.1 200 OK\r\n";
    const QByteArray CONTENT_TYPE_TEXT = "Content-Type: text/html\r\n\r\n";
    const QByteArray CONTENT_TYPE_MJPEG = "Server: inspectrontools.com example server \r\n"
                                          "Cache-Control: no-cache\r\n"
                                          "Cache-Control: private\r\n"
                                          "Content-Type: multipart/x-mixed-replace;boundary=--boundary\r\n\r\n";
    const QByteArray CONTENT_BOUNDARY = "--boundary\r\n"
                                        "Content-Type: image/jpeg\r\n"
                                        "Content-Length: ";

}

/**
 * @brief HttpStreamer::HttpStreamer - ctor
 */
HttpStreamer::HttpStreamer()
: mpTcpServer(new QTcpServer())
{

}

/**
 * @brief HttpStreamer::~HttpStreamer - dtor
 */
HttpStreamer::~HttpStreamer()
{
    stop();
    delete mpTcpServer;
}

/**
 * @brief HttpStreamer::start - overloaded function to start the http server
 * @param address
 * @param port
 */
void HttpStreamer::start(QHostAddress address, int port)
{
    if ( mpTcpServer->listen(address, port) )
    {
        QObject::connect(mpTcpServer, SIGNAL(newConnection()), this, SLOT(onNewTcpConnection()));

        qDebug() << "server started listening";
    }
    else
    {
        qWarning() << "could not start the tcp server";
    }
}

/**
 * @brief HttpStreamer::start - overloaded function to start the http server
 * @param ipAddress - ip address to serve
 * @param port - port to serve
 */
void HttpStreamer::start(QString ipAddress, int port)
{
    start(QHostAddress(ipAddress), port);
}

/**
 * @brief HttpStreamer::start - overloaded function to start the http server with default settings
 */
void HttpStreamer::start()
{
    start(QHostAddress::Any, DEFAULT_PORT);
}

/**
 * @brief HttpStreamer::stop - stop the http server
 */
void HttpStreamer::stop()
{
    if (mpTcpServer->isListening())
    {
        QObject::disconnect(mpTcpServer, SIGNAL(newConnection()), this, SLOT(onNewTcpConnection()));
        mpTcpServer->close();
    }
}

/**
 * @brief HttpStreamer::onNewTcpConnection - slot callback when a new tcp connection is received
 */
void HttpStreamer::onNewTcpConnection()
{
    // next available connection
    QTcpSocket *pSocket = mpTcpServer->nextPendingConnection();

    // delete the socket after its done
    QObject::connect(pSocket, &QTcpSocket::disconnected, pSocket, &QTcpSocket::deleteLater);

    qDebug() << "content in the socket:" << pSocket->readAll();

#if 0
    // add the header & content type
    pSocket->write(HTTP_HEADER);
    pSocket->write(CONTENT_TYPE_TEXT);
    pSocket->write("hello world \r\n");

    pSocket->close();
#else

    static bool a = true;

    qDebug() << "stream jpegs !!";
    pSocket->write(HTTP_HEADER);
    pSocket->write(CONTENT_TYPE_MJPEG);

    while (pSocket->isOpen())
    {
        // get the image
        QString src = a ? "/home/inspectron/Desktop/a.jpg" : "/home/inspectron/Desktop/b.jpg";
        a = !a;

        QFile f(src);
        if (!f.open(QIODevice::ReadOnly))
        {
            qCritical() << "could not open the device " << src;
            break;
        }

        // load the image to a byte array
        QByteArray imgBytes = f.readAll();

        f.close();

        qDebug() << "sending src " << src << "of size " << imgBytes.length();

        QByteArray boundary = QString("--boundary\r\n"
                               "Content-Type: image/jpeg\r\n"
                               "Content-Length: %1 \r\n\r\n").arg(QString::number(imgBytes.length())).toLocal8Bit();
        qDebug() << "isSocketConn ? " << isSocketConnected ;
        pSocket->write(boundary);
        pSocket->write(imgBytes);
        pSocket->flush();

        QThread::msleep(500);
    }



#endif

}
