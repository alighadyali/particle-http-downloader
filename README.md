# particle-http-downloader

This is a downloading utility based on https://github.com/rsteckler/http-download

It still needs some work, but I'm publishing it to share.

This downloader is structured to be asynchronous so you can continue to run your `loop()` function while the download runs.

Use it like so:

```

HttpDownload http(LOGGING_LEVEL_ERROR, 1000, 5, 500);
byte downloadBuffer[512];

void downloadTickLoop(){

  read_response_state_t responseState;
	int code;
	int length;

  bool result = http.process(&responseState, &code, downloadBuffer, &length);
  
  // Now do something with downloadBuffer

}

void startDownload(){

	HttpDownloadRequest request;
	HttpDownloadResponse response;
  
  request.hostname = "sample.com";
	request.port = 80;
	request.path = "/some/path;
  
  client.get(request, response, headers);
  
  http.download(request, response, headers);
  
  }
  
  
  
  // From your main loop()...
  void loop(){
    downloadTickLoop();
  }
  
```
