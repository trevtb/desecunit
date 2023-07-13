#include "httprequest.h"

HttpRequest::HttpRequest(QSettings *settings) {
    this->settings = settings;
    status = waitForRequest;
    currentSize = 0;
    expectedBodySize = 0;
    maxSize = this->settings->value("MjpgStreamer/maxrequestsize", 16000).toInt();
} //endconstructor

void HttpRequest::readRequest(QTcpSocket *socket) {
    int toRead = maxSize-currentSize+1; // allow one byte more to be able to detect overflow
    lineBuffer.append(socket->readLine(toRead));
    currentSize += lineBuffer.size();
    if (!lineBuffer.contains('\r') && !lineBuffer.contains('\n')) {
        return;
    } //endif
    QByteArray newData = lineBuffer.trimmed();
    lineBuffer.clear();
    if (!newData.isEmpty()) {
        QList<QByteArray> list = newData.split(' ');
        if (list.count()!=3 || !list.at(2).contains("HTTP")) {
            logger.log("MJPG streamer received broken HTTP request, invalid first line.", Logger::WARNING);
            status = abort;
        } else {
            method = list.at(0).trimmed();
            path = list.at(1);
            version = list.at(2);
            status = waitForHeader;
        } //endif
    } //endif
} //endfunction readRequest

void HttpRequest::readHeader(QTcpSocket* socket) {
    int toRead = maxSize-currentSize+1; // allow one byte more to be able to detect overflow
    lineBuffer.append(socket->readLine(toRead));
    currentSize += lineBuffer.size();
    if (!lineBuffer.contains('\r') && !lineBuffer.contains('\n')) {
        return;
    } //endif
    QByteArray newData = lineBuffer.trimmed();
    lineBuffer.clear();
    int colon = newData.indexOf(':');
    if (colon > 0) {
        // Received a line with a colon - a header
        currentHeader = newData.left(colon);
        QByteArray value = newData.mid(colon+1).trimmed();
        headers.insert(currentHeader,value);
    } else if (!newData.isEmpty()) {
        // received another line - belongs to the previous header
        // Received additional line of previous header
        if (headers.contains(currentHeader)) {
            headers.insert(currentHeader,headers.value(currentHeader)+" "+newData);
        } //endif
    } else {
        // received an empty line - end of headers reached
        // Empty line received, that means all headers have been received
        QByteArray contentLength = getHeader("Content-Length");
        if (!contentLength.isEmpty()) {
            expectedBodySize = contentLength.toInt();
        } //endif
        if (expectedBodySize == 0) {
            status = complete;
        } else if (boundary.isEmpty() && expectedBodySize+currentSize>maxSize) {
            logger.log("MJPG streamer expected body is too large.", Logger::WARNING);
            status = abort;
        } else {
            status = waitForBody;
        } //endif
    } //endif
} //endfunction readHeader

void HttpRequest::readBody(QTcpSocket *socket) {
    if (boundary.isEmpty()) {
        int toRead = expectedBodySize-bodyData.size();
        QByteArray newData = socket->read(toRead);
        currentSize += newData.size();
        bodyData.append(newData);
        if (bodyData.size() >= expectedBodySize) {
            status = complete;
        } //endif
    } //endif
} //endfunction readBody

void HttpRequest::decodeRequestParams() {
    // Get URL parameters
    QByteArray rawParameters;
    int questionMark = path.indexOf('?');
    if (questionMark >= 0) {
        rawParameters = path.mid(questionMark+1);
        path = path.left(questionMark);
    } //endif
    // Get request body parameters
    QByteArray contentType = headers.value("Content-Type");
    if (!bodyData.isEmpty() && (contentType.isEmpty() || contentType.startsWith("application/x-www-form-urlencoded"))) {
        if (!rawParameters.isEmpty()) {
            rawParameters.append('&');
            rawParameters.append(bodyData);
        } else {
            rawParameters = bodyData;
        } //endif
    } //endif
    // Split the parameters into pairs of value and name
    QList<QByteArray> list = rawParameters.split('&');
    foreach (QByteArray part, list) {
        int equalsChar = part.indexOf('=');
        if (equalsChar >= 0) {
            QByteArray name = part.left(equalsChar).trimmed();
            QByteArray value = part.mid(equalsChar+1).trimmed();
            parameters.insert(urlDecode(name),urlDecode(value));
        } else if (!part.isEmpty()){
            // Name without value
            parameters.insert(urlDecode(part), "");
        } //endif
    } //endforeach
} //endfunction decodeRequestParams

void HttpRequest::readFromSocket(QTcpSocket* socket) {
    if (status == waitForRequest) {
        readRequest(socket);
    } else if (status == waitForHeader) {
        readHeader(socket);
    } else if (status == waitForBody) {
        readBody(socket);
    } //endif
    if ((boundary.isEmpty() && currentSize>maxSize)) {
        logger.log("MJPG streamer received too many bytes.", Logger::WARNING);
        status = abort;
    } //endif
    if (status == complete) {
        // Extract and decode request parameters from url and body
        decodeRequestParams();
    } //endif
} //endfunction readFromSocket

HttpRequest::RequestStatus HttpRequest::getStatus() const {
    return status;
} //endfunction getStatus

QByteArray HttpRequest::getMethod() const {
    return method;
} //endfunction getMethod

QByteArray HttpRequest::getPath() const {
    return urlDecode(path);
} //endfunction getPath

QByteArray HttpRequest::getVersion() const {
    return version;
} //endfunction getVersion

QByteArray HttpRequest::getHeader(const QByteArray& name) const {
    return headers.value(name);
} //endfunction getHeader

QList<QByteArray> HttpRequest::getHeaders(const QByteArray& name) const {
    return headers.values(name);
} //endfunction getHeaders

QMultiMap<QByteArray,QByteArray> HttpRequest::getHeaderMap() const {
    return headers;
} //endfunction getHeaderMap

QByteArray HttpRequest::getParameter(const QByteArray& name) const {
    return parameters.value(name);
} //endfunction getParameter

QList<QByteArray> HttpRequest::getParameters(const QByteArray& name) const {
    return parameters.values(name);
} //endfunction getParameters

QMultiMap<QByteArray,QByteArray> HttpRequest::getParameterMap() const {
    return parameters;
} //endfunction getParameterMap

QByteArray HttpRequest::getBody() const {
    return bodyData;
} //endfunction getBody

QByteArray HttpRequest::urlDecode(const QByteArray source) {
    QByteArray buffer(source);
    buffer.replace('+',' ');
    int percentChar = buffer.indexOf('%');
    while (percentChar >= 0) {
        bool ok;
        char byte = buffer.mid(percentChar+1,2).toInt(&ok,16);
        if (ok) {
            buffer.replace(percentChar,3,(char*)&byte,1);
        } //endif
        percentChar = buffer.indexOf('%',percentChar+1);
    } //endwhile
    return buffer;
} //endfunction urlDecode
