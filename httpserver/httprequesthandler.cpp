#include "httprequesthandler.h"

HttpRequestHandler::HttpRequestHandler(QSettings *settings, QObject* parent) : QObject(parent) {
    this->settings = settings;
} //endconstructor

void HttpRequestHandler::service(HttpRequest&, HttpResponse& response, QString) {
    response.setStatus(501,"not implemented");
    response.write("501 not implemented",true);
} //endfunction service

void HttpRequestHandler::stop() {
    active = false;
} //endfunction stop
