#ifndef __HTTP_DOWNLOAD_H_
#define __HTTP_DOWNLOAD_H_

// #include "spark_wiring_tcpclient.h"
// #include "spark_wiring_usbserial.h"

#include "application.h"

#define LOGGING_LEVEL_NONE 0
#define LOGGING_LEVEL_ERROR 1
#define LOGGING_LEVEL_INFO 2
#define LOGGING_LEVEL_DEBUG 3

/**
 * This struct is used to pass additional HTTP headers such as API-keys.
 * Normally you pass this as an array. The last entry must have NULL as key.
 */
struct HttpDownloadHeader
{
  const char* header;
  const char* value;
};

/**
 * HTTP Request struct.
 * hostname request host
 * path  request path
 * port     request port
 */
struct HttpDownloadRequest
{
  char* hostname;
  IPAddress ip;
  char* path;
  int port;
};

/**
 * HTTP Response struct.
 * status  response status code.
 */
struct HttpDownloadResponse
{
  int status;
};


typedef enum {
    RESPONSE_STATE_HEADERS,
    RESPONSE_STATE_BODY
} read_response_state_t;

class HttpDownload {
public:

    /**
    * Constructor.
    */
    HttpDownload(int logLevel, int chunkSize, int retryAttempts, int retryTimeout);
    HttpDownload();

    //Download an Http object to a file on the SD card.
    void download(HttpDownloadRequest &aRequest, HttpDownloadResponse &response, char* filename);

    //Download an Http object.  For each "chunk" of data that is received, your callback will be called with:
    //int - HTTP status code
    //byte* - the bytes received
    //int - the number of bytes received
    //void* - the object you passed in as callbackParam.
    //See the implementation of writeToFile for an example.
    void download(HttpDownloadRequest &aRequest, HttpDownloadResponse &aResponse, HttpDownloadHeader headers[]);

    bool process(read_response_state_t *state, int *responseStatus, byte *chunk, int *length);
    void finish();

private:

    //TCPClient client;

    int mBytesPerChunk;
    int mLoggingLevel;
    int mRetryAttempts;
    int mRetryTimeout;

    void request(HttpDownloadRequest &aRequest, HttpDownloadResponse &aResponse, HttpDownloadHeader headers[]);

    void out(const char* writeToClient);
    void sendHeader(const char* aHeaderName, const char* aHeaderValue);
    void sendHeader(const char* aHeaderName, const int aHeaderValue);
    void sendHeader(const char* aHeaderName);
    void sendHeaders(HttpDownloadRequest &request, HttpDownloadHeader headers[], int byteRangeStart);
    void receiveResponse(HttpDownloadResponse &response);
    void sendRequest(HttpDownloadRequest &request, HttpDownloadHeader headers[], int byteRangeStart);

    static void writeToFile(byte* fromResponse, int size, void* fileObject);

    void log(int loggingLevel, const char* message);
    void logLine(int loggingLevel, const char* message);
    void log(int loggingLevel, const int message);
    void logLine(int loggingLevel, const int message);
};

#endif /* __HTTP_DOWNLOAD_H_ */
