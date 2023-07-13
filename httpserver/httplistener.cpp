#include "httplistener.h"

HttpListener::HttpListener(QSettings *settings, QObject *parent) : QTcpServer(parent) {
    // Reqister type of socketDescriptor for signal/slot handling
    qRegisterMetaType<tSocketDescriptor>("tSocketDescriptor");
    // Create connection handler pool
    this->settings=settings;
    pool = new HttpConnectionHandlerPool(settings, parent);
    // Start listening
    QString host = this->settings->value("MjpgStreamer/host", "").toString();
    int port = this->settings->value("MjpgStreamer/port", 8080).toInt();
    listen(host.isEmpty() ? QHostAddress::Any : QHostAddress(host), port);
    if (!isListening()) {
        logger.log("MJPG streamer can't bind to desired port.", Logger::ERROR);
    } //endif
} //endconstructor

HttpListener::~HttpListener() {
    close();
    pool->deleteLater();
} //enddeconstructor

void HttpListener::stop() {
    close();
    pool->deleteLater();
} //endfunction stop

void HttpListener::incomingConnection(tSocketDescriptor socketDescriptor) {
    HttpConnectionHandler* freeHandler = pool->getConnectionHandler();

    // Let the handler process the new connection.
    if (freeHandler) {
        // The descriptor is passed via signal/slot because the handler lives in another
        // thread and cannot open the socket when called by another thread.
        connect(this,SIGNAL(handleConnection(tSocketDescriptor)),freeHandler,SLOT(handleConnection(tSocketDescriptor)));
        emit handleConnection(socketDescriptor);
        disconnect(this,SIGNAL(handleConnection(tSocketDescriptor)),freeHandler,SLOT(handleConnection(tSocketDescriptor)));
    } else {
        // Reject the connection
        logger.log("MJPG streamer too many incoming connections.", Logger::WARNING);
        QTcpSocket* socket = new QTcpSocket(this);
        socket->setSocketDescriptor(socketDescriptor);
        connect(socket, SIGNAL(disconnected()), socket, SLOT(deleteLater()));
        socket->write("HTTP/1.1 503 too many connections\r\nConnection: close\r\n\r\nToo many connections\r\n");
        socket->disconnectFromHost();
    } //endif
} //endfunction incomingConnection
