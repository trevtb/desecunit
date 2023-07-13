#include "httpconnectionhandlerpool.h"

HttpConnectionHandlerPool::HttpConnectionHandlerPool(QSettings *settings, QObject *runtime) : QObject() {
    this->settings = settings;
    this->runtime = runtime;
    cleanupTimer.start(this->settings->value("MjpgStreamer/cleanupinterval", 1000).toInt());
    connect(&cleanupTimer, SIGNAL(timeout()), SLOT(cleanup()));
} //endconstructor

HttpConnectionHandlerPool::~HttpConnectionHandlerPool() {
    // delete all connection handlers and wait until their threads are closed
    foreach(HttpConnectionHandler* handler, pool) {
       handler->deleteLater();
    } //endforeach
} //enddeconstructor

HttpConnectionHandler* HttpConnectionHandlerPool::getConnectionHandler() {   
    HttpConnectionHandler* freeHandler=0;
    mutex.lock();
    // find a free handler in pool
    foreach(HttpConnectionHandler* handler, pool) {
        if (!handler->isBusy()) {
            freeHandler = handler;
            freeHandler->setBusy();
            break;
        } //endif
    } //endforeach
    // create a new handler, if necessary
    if (!freeHandler) {
        int maxConnectionHandlers = settings->value("MjpgStreamer/maxthreads", 100).toInt();
        if (pool.count()<maxConnectionHandlers) {
            freeHandler = new HttpConnectionHandler(settings, runtime);
            freeHandler->setBusy();
            pool.append(freeHandler);
        } //endif
    } //endif
    mutex.unlock();
    return freeHandler;
} //endfunction getConnectionHandler

void HttpConnectionHandlerPool::cleanup() {
    int maxIdleHandlers = settings->value("MjpgStreamer/minthreads", 1).toInt();
    int idleCounter = 0;
    mutex.lock();
    foreach(HttpConnectionHandler* handler, pool) {
        if (!handler->isBusy()) {
            if (++idleCounter > maxIdleHandlers) {
                pool.removeOne(handler);                
                delete handler;
                break; // remove only one handler in each interval
            } //endif
        } //endif
    } //endforeach
    mutex.unlock();
} //endfunction cleanup
