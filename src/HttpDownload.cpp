#include "HttpDownload.h"

#define TIMEOUT_INITIAL_RESPONSE 5000
#define TIMEOUT_RESPONSE_READ 5000
#define DEFAULT_CHUNK_SIZE 900
#define DEFAULT_RETRY_ATTEMPTS 3
#define DEFAULT_RETRY_TIMEOUT 500

#define HTTP_DOWNLOAD_METHOD "GET"

extern char* itoa(int a, char* buffer, unsigned char radix);


typedef struct {
  TCPClient client;
//    byte availBuff[512];
//  void(*completionCallback)(int, byte*, int);
  int currentResponseStatus;
  read_response_state_t responseState;
} http_local_t;

http_local_t _http_local;

/**
* Constructor.
*/
HttpDownload::HttpDownload() {
  mBytesPerChunk = DEFAULT_CHUNK_SIZE;
  mLoggingLevel = LOGGING_LEVEL_NONE;
  mRetryAttempts = DEFAULT_RETRY_ATTEMPTS;
  mRetryTimeout = DEFAULT_RETRY_TIMEOUT;
}

HttpDownload::HttpDownload(int logLevel, int bytesPerChunk, int retryAttempts, int retryTimeout) {
  mBytesPerChunk = bytesPerChunk;
  mLoggingLevel = logLevel;
  mRetryAttempts = retryAttempts;
  mRetryTimeout = retryTimeout;
}


/**
* Method to send an HTTP Request. Allocate variables in your application code
* in the aResponse struct and set the headers and the options in the aRequest
* struct.
*/
void HttpDownload::request(HttpDownloadRequest &aRequest, HttpDownloadResponse &aResponse, HttpDownloadHeader headers[])
{

  int byteRangeStart = 0;
//  int lastHttpStatus = 206;
//  int retryCount = 0;

//  TCPClient client;

//  while (lastHttpStatus == 206 || lastHttpStatus == -2) { //-2 is a retry request
//    lastHttpStatus = -1; //assume failure


    sendRequest(aRequest, headers, byteRangeStart);

    receiveResponse(aResponse);
//    Serial.println("Received response.");
//    Serial.flush();
//    if (lastHttpStatus == -1) {
//        Serial.println("Response was -1, retrying...");
//        Serial.flush();
//        retryCount++;
//        if (retryCount > mRetryAttempts) {
//          //Give up.
//          logLine(LOGGING_LEVEL_ERROR, "Giving up after max retries.");
//        } else {
//          //Delay, and ask for a retry
//          logLine(LOGGING_LEVEL_INFO, "Last connection failed.  Retrying.");
//          delay(mRetryTimeout);
//          lastHttpStatus = -2;
//        }
//    } else {
//        //Good response.  Clear the retry count
//        retryCount = 0;
//        byteRangeStart += mBytesPerChunk;
//        Serial.println("Good response, clearing retry count.");
//        Serial.flush();
//    }
//  }
}

void HttpDownload::sendRequest(HttpDownloadRequest &request, HttpDownloadHeader headers[], int byteRangeStart) {

  // NOTE: The default port tertiary statement is unpredictable if the request structure is not initialised
  // http_request_t request = {0} or memset(&request, 0, sizeof(http_request_t)) should be used
  // to ensure all fields are zero

  bool connected = false;
  if(request.hostname!=NULL) {
        logLine(LOGGING_LEVEL_DEBUG, "sendRequest(), connecting to hostname: ");
        logLine(LOGGING_LEVEL_DEBUG, request.hostname);
        logLine(LOGGING_LEVEL_DEBUG, request.port);
    //connected = client.connect(server, (request.port) ? request.port : 80 );
    connected = _http_local.client.connect(request.hostname, 80 );
    logLine(LOGGING_LEVEL_DEBUG, "Connection attempt started.");
  } else {
        log(LOGGING_LEVEL_DEBUG, "sendRequest(), connecting to ip: ");
        log(LOGGING_LEVEL_DEBUG, request.ip[0]);
        log(LOGGING_LEVEL_DEBUG, ".");
        log(LOGGING_LEVEL_DEBUG, request.ip[1]);
        log(LOGGING_LEVEL_DEBUG, ".");
        log(LOGGING_LEVEL_DEBUG, request.ip[2]);
        log(LOGGING_LEVEL_DEBUG, ".");
        logLine(LOGGING_LEVEL_DEBUG, request.ip[3]);
    connected = _http_local.client.connect(request.ip, request.port);
  }


  if (connected) {
    logLine(LOGGING_LEVEL_DEBUG, "Connected.");
  } else {
    log(LOGGING_LEVEL_DEBUG, "Connection failed.");
  }

  if (!connected) {
    _http_local.client.stop();
    // If TCP Client can't connect to host, exit here.
    return;
  }

  sendHeaders(request, headers, byteRangeStart);

  logLine(LOGGING_LEVEL_DEBUG, "End of HTTP Request.");

}


void HttpDownload::sendHeaders(HttpDownloadRequest &request, HttpDownloadHeader headers[], int byteRangeStart) {
  //
  // Send HTTP Headers
  //

  // Send initial headers (only HTTP 1.0 is supported for now).
  out(HTTP_DOWNLOAD_METHOD);
  out(" ");
  out(request.path);
  out(" HTTP/1.1\r\n");

  logLine(LOGGING_LEVEL_DEBUG, "Start of HTTP Request.");
  logLine(LOGGING_LEVEL_DEBUG, "");
  log(LOGGING_LEVEL_DEBUG, HTTP_DOWNLOAD_METHOD);
  log(LOGGING_LEVEL_DEBUG, " ");
  log(LOGGING_LEVEL_DEBUG, request.path);
  logLine(LOGGING_LEVEL_DEBUG, " HTTP/1.1");

  // Send General and Request Headers.
  sendHeader("Connection", "close"); // Not supporting keep-alive for now.
  sendHeader("accept-encoding", "gzip, deflate");
  char rangeValue[64];
  char itoaBuffer[32];
  strcpy(rangeValue, "bytes=");
  itoa(byteRangeStart, itoaBuffer, 10);
  strcat(rangeValue, itoaBuffer);
  strcat(rangeValue, "-");
  itoa(byteRangeStart + mBytesPerChunk - 1, itoaBuffer, 10);
  strcat(rangeValue, itoaBuffer);
  //sendHeader("Range", rangeValue);
  if(request.hostname!=NULL) {
    sendHeader("Host", request.hostname);
  }

  //Send custom headers
  if (headers != NULL) {
    int i = 0;
    while (headers[i].header != NULL) {
      if (headers[i].value != NULL) {
        sendHeader(headers[i].header, headers[i].value);
      } else {
        sendHeader(headers[i].header);
      }
      i++;
    }
  }

  // Empty line to finish headers
  out("\r\n");
  _http_local.client.flush();


  logLine(LOGGING_LEVEL_DEBUG, "");
}

bool HttpDownload::process(read_response_state_t *state, int *responseStatus, byte *chunk, int *length)
{
  *state = _http_local.responseState;
  *responseStatus = _http_local.currentResponseStatus;
  *length = 0;

  if (!_http_local.client.connected()){
    return false;
  }

  if (!_http_local.client.available()){
    return true;
  }


    int numAvail = _http_local.client.available();
    if (numAvail > 512){
      Serial.printlnf("Error, received chunk larger than static buffer: %i", numAvail);
      return false;
    }


    if (numAvail > 0) {
      _http_local.client.read(chunk, numAvail);

      if (_http_local.currentResponseStatus == -1) {
        char httpStatus[4];
        httpStatus[0] = chunk[9];
        httpStatus[1] = chunk[10];
        httpStatus[2] = chunk[11];
        httpStatus[3] = 0;
        _http_local.currentResponseStatus = atoi(httpStatus);

        if (_http_local.currentResponseStatus == 404) {
          logLine(LOGGING_LEVEL_ERROR, "Request resulted in HTTP 404.");
        }
      }

      byte* callbackData = chunk;

      if (_http_local.responseState == RESPONSE_STATE_HEADERS) {

        char* headerEnd = strstr((const char*)chunk, "\r\n\r\n");

        if (headerEnd != NULL) {

          logLine(LOGGING_LEVEL_DEBUG, "Found header end");
          headerEnd += 4;// \r\n\r\n from above

          callbackData = (byte*)headerEnd;
          numAvail -= (byte*)headerEnd - chunk;

          _http_local.responseState = RESPONSE_STATE_BODY;

        } else {
          //Still haven't found headers.  Don't send anything to the callback.

          logLine(LOGGING_LEVEL_DEBUG, "Header end not yet found.");
          numAvail = 0;
        }

      }

      if (_http_local.responseState == RESPONSE_STATE_HEADERS){
        // still working on headers...
        return true;
      }

      // Send the data to our callback
      if (numAvail != 0) {

//        Serial.printf("na: %i, ", numAvail);

        *responseStatus = _http_local.currentResponseStatus;
          memcpy(chunk, callbackData, numAvail);
//        *chunk = callbackData;
        *length = numAvail;
        *state = _http_local.responseState;

//        if (_http_local.completionCallback != NULL) {
//          Serial.println("callback...");
////          _http_local.completionCallback(_http_local.currentResponseStatus, callbackData, numAvail);
//        }
        return true;
      } else {
          logLine(LOGGING_LEVEL_DEBUG, "numAvail == 0, end of response detected.");
          return true;
      }

    } else {
      return true;
    }


}

void HttpDownload::finish()
{

    _http_local.client.flush();
    _http_local.client.stop();
    logLine(LOGGING_LEVEL_DEBUG, "Connection closed");
}

void HttpDownload::receiveResponse(HttpDownloadResponse &response) {
  _http_local.currentResponseStatus = -1;
  _http_local.responseState = RESPONSE_STATE_HEADERS;
}

/*void HttpDownload::download(HttpDownloadRequest &aRequest, HttpDownloadResponse &response, char* filename) {
    File outFile;
    outFile = SD.open(filename, FILE_WRITE);

    request(aRequest, response, NULL, writeToFile, &outFile);
    outFile.close();
}*/

void HttpDownload::download(HttpDownloadRequest &aRequest, HttpDownloadResponse &aResponse, HttpDownloadHeader headers[]) {

//  _http_local.completionCallback = callback;
    request(aRequest, aResponse, headers);
}

/*void HttpDownload::writeToFile(byte* fromResponse, int size, void* fileObject) {
  File* outFile = (File*)fileObject;
  outFile->write(fromResponse, size);
}*/

//_http_local.client.print is extremely slow.  Use this instead.
void HttpDownload::out(const char *s) {
  _http_local.client.write( (const uint8_t*)s, strlen(s) );
}

/**
* Method to send a header, should only be called from within the class.
*/
void HttpDownload::sendHeader(const char* aHeaderName, const char* aHeaderValue)
{
  out(aHeaderName);
  out(": ");
  out(aHeaderValue);
  out("\r\n");

  log(LOGGING_LEVEL_DEBUG, aHeaderName);
  log(LOGGING_LEVEL_DEBUG, ": ");
  logLine(LOGGING_LEVEL_DEBUG, aHeaderValue);
}

void HttpDownload::sendHeader(const char* aHeaderName, const int aHeaderValue)
{
  char buffer[16];
  itoa(aHeaderValue, buffer, 10);
  sendHeader(aHeaderName, buffer);
}

void HttpDownload::sendHeader(const char* aHeaderName)
{
  out(aHeaderName);

  logLine(LOGGING_LEVEL_DEBUG, aHeaderName);
}

void HttpDownload::log(int loggingLevel, const char* message) {
  if (mLoggingLevel >= loggingLevel) {
    Serial.print(message);
    Serial.flush();
    //delay(1000);
  }
}

void HttpDownload::log(int loggingLevel, const int message) {
  if (mLoggingLevel >= loggingLevel) {
    char itoaBuffer[16];
    itoa(message, itoaBuffer, 10);
    Serial.print(itoaBuffer);
    Serial.flush();
    //delay(1000);
  }
}

void HttpDownload::logLine(int loggingLevel, const char* message) {
  log(loggingLevel, message);
  log(loggingLevel, "\r\n");
}

void HttpDownload::logLine(int loggingLevel, const int message) {
  log(loggingLevel, message);
  log(loggingLevel, "\r\n");
}
