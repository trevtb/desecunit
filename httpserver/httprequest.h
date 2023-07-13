#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include <QByteArray>
#include <QTcpSocket>
#include <QMap>
#include <QSettings>
#include <QMultiMap>
#include <QUuid>
#include <QList>
#include <QDir>

#include "logger.h"

/**
  This object represents a single HTTP request. It reads the request
  from a TCP socket and provides getters for the individual parts
  of the request.
  <p>
  The follwing config settings are required:
  <code><pre>
  maxRequestSize=16000
  </pre></code>
  <p>
  MaxRequestSize is the maximum size of a HTTP request.
*/
class HttpRequest {
    Q_DISABLE_COPY(HttpRequest)
public:
    /** Values for getStatus() */
    enum RequestStatus {waitForRequest, waitForHeader, waitForBody, complete, abort};
    /**
      Constructor.
      @param settings Configuration settings
    */
    HttpRequest(QSettings *settings);
    /**
      Read the request from a socket. This method must be called repeatedly
      until the status is RequestStatus::complete or RequestStatus::abort.
      @param socket Source of the data
    */
    void readFromSocket(QTcpSocket *socket);
    /**
      Get the status of this reqeust.
      @see RequestStatus
    */
    RequestStatus getStatus() const;
    /** Get the method of the HTTP request  (e.g. "GET") */
    QByteArray getMethod() const;
    /** Get the decoded path of the HTPP request (e.g. "/index.html") */
    QByteArray getPath() const;
    /** Get the version of the HTPP request (e.g. "HTTP/1.1") */
    QByteArray getVersion() const;
    /**
      Get the value of a HTTP request header.
      @param name Name of the header
      @return If the header occurs multiple times, only the last
      one is returned.
    */
    QByteArray getHeader(const QByteArray& name) const;
    /**
      Get the values of a HTTP request header.
      @param name Name of the header
    */
    QList<QByteArray> getHeaders(const QByteArray& name) const;
    /** Get all HTTP request headers */
    QMultiMap<QByteArray,QByteArray> getHeaderMap() const;
    /**
      Get the value of a HTTP request parameter.
      @param name Name of the parameter
      @return If the parameter occurs multiple times, only the last
      one is returned.
    */
    QByteArray getParameter(const QByteArray& name) const;
    /**
      Get the values of a HTTP request parameter.
      @param name Name of the parameter
    */
    QList<QByteArray> getParameters(const QByteArray& name) const;
    /** Get all HTTP request parameters */
    QMultiMap<QByteArray,QByteArray> getParameterMap() const;
    /** Get the HTTP request body  */
    QByteArray getBody() const;
    /**
      Decode an URL parameter.
      E.g. replace "%23" by '#' and replace '+' by ' '.
      @param source The url encoded strings
      @see QUrl::toPercentEncoding for the reverse direction
    */
    static QByteArray urlDecode(const QByteArray source);
    QSettings *settings;
private:
    /** Request headers */
    QMultiMap<QByteArray,QByteArray> headers;
    /** Parameters of the request */
    QMultiMap<QByteArray,QByteArray> parameters;
    /** Storage for raw body data */
    QByteArray bodyData;
    /** Request method */
    QByteArray method;
    /** Request path (in raw encoded format) */
    QByteArray path;
    /** Request protocol version */
    QByteArray version;
    /**
      Status of this request.
      @see RequestStatus
    */
    RequestStatus status;
    /** Maximum size of requests in bytes. */
    int maxSize;
    /** Current size */
    int currentSize;
    /** Expected size of body */
    int expectedBodySize;
    /** Name of the current header, or empty if no header is being processed */
    QByteArray currentHeader;
    /** Boundary of multipart/form-data body. Empty if there is no such header */
    QByteArray boundary;
    /** Sub-procedure of readFromSocket(), read the first line of a request. */
    void readRequest(QTcpSocket* socket);
    /** Sub-procedure of readFromSocket(), read header lines. */
    void readHeader(QTcpSocket* socket);
    /** Sub-procedure of readFromSocket(), read the request body. */
    void readBody(QTcpSocket* socket);
    /** Sub-procedure of readFromSocket(), extract and decode request parameters. */
    void decodeRequestParams();
    /** Buffer for collecting characters of request and header lines */
    QByteArray lineBuffer;
    /** Class for easily sending console messages with timestamps **/
    Logger logger;
};
#endif // HTTPREQUEST_H
