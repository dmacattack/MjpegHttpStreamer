#include "httpstreamer.hpp"
#include <QTcpSocket>
#include <QDebug>
#include <QImage>
#include <QThread>
#include <QFile>
#include <QDataStream>
#include <QTimer>
#include <QDateTime>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>

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

    const QString APPSINK_NAME = "jpegSink";

}

/**
 * @brief HttpStreamer::HttpStreamer - ctor
 */
HttpStreamer::HttpStreamer()
: mpTcpServer(new QTcpServer())
, mpPipeline(NULL)
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

    qDebug() << "----------start the appsink example ----------";
    QString launchString = "";
    QTextStream(&launchString) << "videotestsrc pattern=ball ! \
                                   video/x-raw,width=1280,height=720,framerate=15/1 ! \
                                   clockoverlay ! \
                                   videoconvert ! \
                                   jpegenc ! \
                                   appsink max-buffers=1 drop=true name=" << APPSINK_NAME;

    mpPipeline = gst_parse_launch(launchString.toStdString().c_str(), NULL);
    gst_element_set_state (mpPipeline, GST_STATE_PLAYING);

    qDebug() << "pipeline launched at " << QDateTime::currentDateTime().toString("hh:mm:ss");
    QTimer::singleShot(3000, [&]
    {
        qDebug() << "pull a frame at " << QDateTime::currentDateTime().toString("hh:mm:ss");
        pullFrame();

        qFatal("exit");
    });

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
        pSocket->write(boundary);
        pSocket->write(imgBytes);
        pSocket->flush();

        QThread::msleep(500);
    }



#endif

}

/**
 * @brief HttpStreamer::pullFrame - pull a frame from the appsink
 * @returns a frame from the appsink
 */
char* HttpStreamer::pullFrame()
{
    // set the raw tee object for the baseclass
    GstElement *pJpegSink = gst_bin_get_by_name(GST_BIN(mpPipeline), APPSINK_NAME.toStdString().c_str());

    // get the sample
    GstSample *pSample = gst_app_sink_pull_sample(GST_APP_SINK(pJpegSink));

    if (pSample == NULL)
    {
        qCritical() << "appsink sample is null";
        return NULL;
    }

    // extract the image inside the sample
    GstBuffer *pBuf = gst_sample_get_buffer(pSample);
    GstMapInfo map;
    gst_buffer_map(pBuf, &map, GST_MAP_READ);

    QByteArray img = QByteArray(reinterpret_cast<char*>(map.data), map.size);
    saveToFile(img, "/home/inspectron/Desktop/frame.jpg");

    // cleanup memory
    gst_buffer_unmap(pBuf, &map);
    gst_sample_unref(pSample);

    return NULL;
}

/**
 * @brief HttpStreamer::saveToFile - save a buffer to a file
 * @param buf - buffer to save
 * @param sz - size of the buffer
 * @param filename - filename to save
 */
void HttpStreamer::saveToFile(char *buf, int sz, QString filename)
{
    qDebug() << "saving buffer to a " << filename;
    QFile f(filename);
    if (f.open(QIODevice::WriteOnly| QIODevice::Text))
    {

        f.write(buf, sz);

        f.close();
    }
    else
    {
        qWarning() << "could not save frame to file";
    }
}

/**
 * @brief HttpStreamer::saveToFile - save a buffer to a file
 * @param buf - qbytearray
 * @param filename - filename to save
 */
void HttpStreamer::saveToFile(QByteArray buf, QString filename)
{
    qDebug() << "saving buffer to a " << filename;
    QFile f(filename);
    if (f.open(QIODevice::WriteOnly| QIODevice::Text))
    {
        f.write(buf);
        f.close();
    }
    else
    {
        qWarning() << "could not save frame to file";
    }
}







