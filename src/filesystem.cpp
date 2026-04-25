#include "filesystem.h"
#include "include.h"
#include <HTTPClient.h>
#include "display.h"

static const char *TAG = "FILE";

extern const uint8_t *DEFAULT_FONT;

const word filemanagerport = FILE_MANAGER_PORT;
ESPFMfGK filemgr(filemanagerport);

//----------------------------------------------
void addFileSystems(void)
{
    // set configTzTime() in setup() to get valid file dates. Otherwise they are kaputt[tm].

    // This adds the Storage into the Filemanager. You have to at least call one of those.
    // If you don't, begin() will fail. Because a Filemanager without files is useless.
#ifdef USE_INTERNAL_SPIFFS
    if (!filemgr.AddFS(SPIFFS, "SPIFFS", false))
    {
        Serial.println(F("Adding SPIFFS failed."));
    }
#else
    if (!filemgr.AddFS(SD_MMC, "SD-MMC-Card", false))
    {
        Serial.println(F("Adding SD_MMC failed."));
    }
#endif
}

//----------------------------------------------
uint32_t checkFileFlags(fs::FS &fs, String filename, uint32_t flags)
{
    // Show file/path in Lists
    // filenames start without "/", pathnames start with "/"
    if (flags & (ESPFMfGK::flagCheckIsFilename | ESPFMfGK::flagCheckIsPathname))
    {
        ESP_LOGI(TAG, "flagCheckIsFilename || flagCheckIsPathname check: %s", filename.c_str());
        if (flags | ESPFMfGK::flagCheckIsFilename)
        {
            if (filename.startsWith("."))
            {
                ESP_LOGI(TAG, "flagIsNotVisible: %s", filename.c_str());
                return ESPFMfGK::flagIsNotVisible;
            }
        }
        /*
           this will catch a pathname like /.test, but *not* /foo/.test
           so you might use .indexOf()
        */
        if (flags | ESPFMfGK::flagCheckIsPathname)
        {
            if (filename.startsWith("/."))
            {
                ESP_LOGI(TAG, "flagIsNotVisible: %s", filename.c_str());
                return ESPFMfGK::flagIsNotVisible;
            }
        }
    }

    // Checks if target file name is valid for action. This will simply allow everything by returning the queried flag
    if (flags & ESPFMfGK::flagIsValidAction)
    {
        return flags & (~ESPFMfGK::flagIsValidAction);
    }

    // Checks if target file name is valid for action.
    if (flags & ESPFMfGK::flagIsValidTargetFilename)
    {
        return flags & (~ESPFMfGK::flagIsValidTargetFilename);
    }

    // Default actions
    uint32_t defaultflags = ESPFMfGK::flagCanDelete | ESPFMfGK::flagCanRename | ESPFMfGK::flagCanGZip |
                            ESPFMfGK::flagCanDownload | ESPFMfGK::flagCanUpload | ESPFMfGK::flagCanEdit |
                            ESPFMfGK::flagAllowPreview;

    return defaultflags;
}

//----------------------------------------------
void setupFilemanager(void)
{
    // See above.
    filemgr.checkFileFlags = checkFileFlags;

    filemgr.WebPageTitle = "FileManager";
    filemgr.BackgroundColor = "white";
    filemgr.textareaCharset = "accept-charset=\"utf-8\"";

    // If you want authentication
    // filemgr.HttpUsername = "my";
    // filemgr.HttpPassword = "secret";

    // display the file date? change here. does not work well if you never set configTzTime()
    filemgr.FileDateDisplay = ESPFMfGK::fddNone;

    if ((WiFi.status() == WL_CONNECTED) && (filemgr.begin()))
    {
        ESP_LOGI(TAG, "Open Filemanager with http://%s:%d/", WiFi.localIP().toString().c_str(), filemanagerport);
    }
    else
    {
        Serial.print(F("Filemanager: did not start"));
    }
}

//----------------------------------------------
// Fetch a file from the URL given and save it to SD
// Return filesize or 0 if file already exists or error
int getFile(String url, String targetFilename)
{
    ESP_LOGI(TAG, "Downloading from %s", url.c_str());
    ESP_LOGI(TAG, "Target = %s", targetFilename.c_str());
    int filesize = 0;

    clearScreen();
    footbanner("Downloading... ");
    if ((WiFi.status() == WL_CONNECTED))
    { // Check WiFi connection
        ESP_LOGI(TAG, "[HTTP] begin...");
        File filehandle;
        HTTPClient http;
        http.begin(url);                                        // Configure server and url
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS); // Follow automatically after a "302 redirect"

        int httpCode = http.GET(); // Start connection and send HTTP header
        if (httpCode > 0)
        { // HTTP header has been send and Server response header has been handled
            ESP_LOGI(TAG, "[HTTP] GET... code: %d", httpCode);

#ifdef USE_INTERNAL_SPIFFS
            filehandle = SPIFFS.open(targetFilename, "w+");
#else
            filehandle = SD_MMC.open(targetFilename, "w+");
#endif
            if (!filehandle)
            {
                Serial.println("file open failed");
                return 0;
            }
            ESP_LOGI(TAG, "File opened for writing...", httpCode);

            if (httpCode == HTTP_CODE_OK)
            {                                             // File found at server
                int len = http.getSize();                 // Get length of document (is -1 when Server sends no Content-Length header)
                uint8_t buff[1024] = {0};                 // Create buffer for read
                WiFiClient *stream = http.getStreamPtr(); // Get tcp stream

                while (http.connected() && (len > 0 || len == -1))
                {                                      // Read all data from server
                    size_t size = stream->available(); // Get available data size
                    if (size)
                    {
                        rectfill(250, 205, 15, 15, BLACK);
                        int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size)); // Read up to 128 bytes
                        filehandle.write(buff, c);                                                      // Write it to file
                        if (len > 0)
                            len -= c; // Calculate remaining bytes
                    }
                    yield();
                }
                ESP_LOGI(TAG, "[HTTP] connection closed or file end.");
            }
            filehandle.close();

#ifdef USE_INTERNAL_SPIFFS
            filehandle = SPIFFS.open(targetFilename, FILE_READ);
#else
            filehandle = SD_MMC.open(targetFilename, FILE_READ);
#endif
            filesize = filehandle.size(); // Check filesize
            filehandle.close();
            if (filesize == 0)
#ifdef USE_INTERNAL_SPIFFS
            SPIFFS.remove(targetFilename); // If filesize = 0 delete it
#else
            SD_MMC.remove(targetFilename); // If filesize = 0 delete it
#endif
        }
        else
        {
            ESP_LOGI(TAG, "[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end();
    }
    return (filesize);
}
