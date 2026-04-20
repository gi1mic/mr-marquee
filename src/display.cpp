#include "display.h"
#include "include.h"
#include "JpegFunc.h"
#include "MjpegClass.h"
#include <WiFi.h>
#include "filesystem.h"
#include "network.h"
#include "FileFetcher.h"

static const char *TAG = "DISP";

String currentCore = ""; // Corename
int currentRotation = DEFAULT_ROTATION;
bool videoPlay = false;

String folderjpg = "/jpg/";
String foldermjpeg = "/mjpeg/";
String dirletter = "";
//bool PORTRAIT_SCREEN = true;
//#define  PORTRAIT_SCREEN true

Arduino_DataBus *bus = new Arduino_ESP32SPI(GFX_NOT_DEFINED, 0, 2 /* SCK */, 1 /* MOSI */, GFX_NOT_DEFINED /* MISO */, FSPI /* spi_num */);

Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
    40 /* DE */, 39 /* VSYNC */, 38 /* HSYNC */, 41 /* PCLK */,
    17 /* R0 */, 46 /* R1 */, 03 /* R2 */, 8 /* R3 */, 18 /* R4 */,
    14 /* G0 */, 13 /* G1 */, 12 /* G2 */, 11 /* G3 */, 10 /* G4 */, 9 /* G5 */,
    21 /* B0 */, 5 /* B1 */, 45 /* B2 */, 48 /* B3 */, 47 /* B4 */,
    1 /* hsync_polarity */, 10 /* hsync_front_porch */, 8 /* hsync_pulse_width */, 50 /* hsync_back_porch */,
    1 /* vsync_polarity */, 10 /* vsync_front_porch */, 8 /* vsync_pulse_width */, 20 /* vsync_back_porch */);

Arduino_RGB_Display *tft = new Arduino_RGB_Display(
    TFT_WIDTH /* width */, TFT_HEIGHT /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
    bus, 16 /* RST */, st7701_type10_init_operations, sizeof(st7701_type10_init_operations));

static int jpegDrawCallback(JPEGDRAW *pDraw);
static MjpegClass mjpeg;

int DispWidth = 0;
int DispHeight = 0;

//----------------------------------------------
void tftInit()
{
    tft->begin();
    tft->setRotation(DEFAULT_ROTATION);
    tft->invertDisplay(true);
    tft->fillScreen(BLACK);
    DispWidth = tft->width();
    DispHeight = tft->height();
    tft->setFont(TFT_FONT_NORMAL);
    mjpeg_buf = (uint8_t *)malloc(MJPEG_BUFFER_SIZE); // Video buffer
    ESP_LOGI(TAG, "Display width: %d, height: %d\n", DispWidth, DispHeight);
}

//----------------------------------------------
void writetext(String text, int fixedpos, int textposX, int textposY, const uint8_t *fontname, int textrotation, int fontcolor, int backcolor, String clear)
{
    if (clear == "clear")
        tft->fillScreen(BLACK);
    if (fixedpos == 1)
        tft->setCursor(textposX, textposY); // fixed position
    if (fixedpos == 0)
    {
        int16_t tempX, tempY;
        uint16_t w, h;
        tft->getTextBounds(text, textposX, textposY, &tempX, &tempY, &w, &h); // calc width of new string
        tft->setCursor(textposX - w / 2, textposY);
    }
    if (backcolor == false)
    {
        tft->setTextColor(fontcolor); // Transparent background
    }
    else
    {
        tft->setTextColor(fontcolor, backcolor); // Background color
    }
    tft->print(text);
}

//----------------------------------------------
void writetextcentered(String text, int textposY, const uint8_t *fontname, int textrotation, int fontcolor, int backcolor, String clear)
{
    if (clear == "clear")
        tft->fillScreen(BLACK);

    int16_t tempX, tempY;
    uint16_t w, h;
    tft->getTextBounds(text, 0, 0, &tempX, &tempY, &w, &h); // calc width of text

    // Calculate centered position horizontally only
    int centerX = (DispWidth - w) / 2;

    tft->setCursor(centerX, textposY);

    if (backcolor == false)
    {
        tft->setTextColor(fontcolor); // Transparent background
    }
    else
    {
        tft->setTextColor(fontcolor, backcolor); // Background color
    }
    tft->print(text);
}

//----------------------------------------------
void footbanner(String bannertext)
{
    writetext(bannertext, 1, DispWidth / 2 - ((bannertext.length() * 20) / 2), 270, TFT_FONT_LARGE, 0, RED, false, "");
}

//----------------------------------------------
void showJpegImage(String core, int pictureposX, int pictureposY, int scale)
{
    ESP_LOGD(TAG, "Play pic: %s", core.c_str());
    jpegDraw(core.c_str(), jpegDrawCallback, true, pictureposX, pictureposY, tft->width(), tft->height(), scale);
    ESP_LOGD(TAG, "Jpeg decode status %i", _jpeg.getLastError());
}

//----------------------------------------------
bool showLocalImage(String core)
{
    String fqn = "/" + core + ".jpg";
    ESP_LOGI(TAG, "Showing local pic: %s", fqn.c_str());
#ifdef USE_SPIFFS
    if (SPIFFS.exists(fqn))
#else
    if (SD_MMC.exists(fqn))
#endif
    {
        showJpegImage(fqn, -1, -1, 0);
        return true;
    };
    return false;
}

//----------------------------------------------
bool showCorenameURL(String core)
{
    String fetchBaseURL = MISTER_WEBSERVER;
    String prefix = core.substring(0, 1) + "/";

    if (PORTRAIT_SCREEN == true)
    {
        ESP_LOGD(TAG, "Portrait :%s", PORTRAIT_SCREEN ? "True" : "False");
        fetchBaseURL = fetchBaseURL + "/" + TFT_HEIGHT + "x" + TFT_WIDTH;
    }
    else
    {
        ESP_LOGD(TAG, "Portrait :%s", PORTRAIT_SCREEN ? "True" : "False");
        fetchBaseURL = fetchBaseURL + "/" + TFT_WIDTH + "x" + TFT_HEIGHT;
    }
    ESP_LOGD(TAG, "fetchBaseURL: %s", fetchBaseURL.c_str());

    fetchBaseURL = fetchBaseURL + addPathAndExtension(core, "jpg");
    ESP_LOGD(TAG, "fetchBaseURL+ext: %s", fetchBaseURL.c_str());

    char charArray[200];
    fetchBaseURL.toCharArray(charArray, sizeof(charArray));
    ESP_LOGD(TAG, "Showing URL: %s", fetchBaseURL.c_str());

    return (showImageURL(charArray));
}

//----------------------------------------------
bool showImageURL(char *imageUrl)
{
    ESP_LOGI(TAG, "Showing URL: %s", imageUrl);

    uint8_t *imageFile; // pointer that the library will store the image at (uses malloc)
    int imageSize;      // library will update the size of the image
    bool gotImage = fileFetcher.getFile(imageUrl, &imageFile, &imageSize);

    if (gotImage) // imageFile is now a pointer to memory that contains the image file
    {
        ESP_LOGD(TAG, "Got Image. Size %i", imageSize);
        int rc = _jpeg.openRAM(imageFile, imageSize, jpegDrawCallback);
        _jpeg.setPixelType(1);
        int decodeStatus = _jpeg.decode(0, 0, 0);
        ESP_LOGD(TAG, "Jpeg decode status %i", _jpeg.getLastError());
        _jpeg.close();
        free(imageFile); // Make sure to free the memory!
    }
    return gotImage;
}

//----------------------------------------------
bool showVideo(String core, int videoposX, int videoposY)
{
    String fqn = addPathAndExtension(core, "mjpeg");
    ESP_LOGI(TAG, "Play video: %s", fqn.c_str());
    File filehandle;
#ifdef USE_SPIFFS
    filehandle = SPIFFS.open(fqn, FILE_READ);
#else
    filehandle = SD_MMC.open(fqn, FILE_READ);
#endif
    if (!filehandle || filehandle.isDirectory())
    {
        ESP_LOGI(TAG, "ERROR: Failed to open %s file for reading", fqn);
        footbanner("ERROR: Failed to open " + fqn + " file for reading");
        return false;
    }
    else
    {
        if (!mjpeg_buf)
        {
            ESP_LOGI(TAG, "mjpeg_buf malloc failed!");
            footbanner("mjpeg_buf malloc failed!");
            return false;
        }
        else
        {

            ESP_LOGD(TAG, "MJPEG start");

            mjpeg.setup(&filehandle, mjpeg_buf, jpegDrawCallback, true, 0, 0, tft->width(), tft->height());
            while (filehandle.available())
            {
                mjpeg.readMjpegBuf(); // Read video
                mjpeg.drawJpg();      // Play video
            }
            ESP_LOGD(TAG, "MJPEG end");
            filehandle.close();
            return true;
        }
    }
    return false;
}

//----------------------------------------------
// pixel drawing callback
static int jpegDrawCallback(JPEGDRAW *pDraw)
{
    tft->draw16bitBeRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
    return 1;
}

//----------------------------------------------
void rectfill(int posX, int posY, int rectwidth, int rectheight, int fillcolor)
{
    tft->fillRect(posX, posY, rectwidth, rectheight, fillcolor);
}

//----------------------------------------------
bool fetchfile(String fetchBaseURL, String core)
{
    int filesize;
    ESP_LOGI(TAG, "Showing downloading image");
    showJpegImage(addPathAndExtension(PIC_DOWNLOADING, "jpg"), 0, 0, 0);

    String prefix = core.substring(0, 1) + "/";
    String targetFilename = "/jpg/";
    prefix.toUpperCase();
    if (isDigit(prefix.charAt(0)))
        prefix = "numeric/"; // Renamed to 'numeric' to avoid issues with web based file management using #
    targetFilename = targetFilename + prefix;

    if (PORTRAIT_SCREEN == true)
    {
        ESP_LOGD(TAG, "Portrait :%s", PORTRAIT_SCREEN ? "True" : "False");
        fetchBaseURL = fetchBaseURL + TFT_HEIGHT + "x" + TFT_WIDTH + "/jpg/" + prefix;
    }
    else
    {
        ESP_LOGD(TAG, "Portrait :%s", PORTRAIT_SCREEN ? "True" : "False");
        fetchBaseURL = fetchBaseURL + TFT_WIDTH + "x" + TFT_HEIGHT + "/jpg/" + prefix;
    }

    fetchBaseURL = fetchBaseURL + addPathAndExtension(core, "mjpg");
    targetFilename = targetFilename + addPathAndExtension(core, "mjpg");

    filesize = getFile(fetchBaseURL, targetFilename);
    ESP_LOGD(TAG, "Get file size: %d for %s", filesize, core.c_str());
    if (filesize != 0)
    {
        ESP_LOGD(TAG, "Video found, showing video");
        return showVideo(core, 0, 0);
    }
    else
    { // Try for image if video not found
        fetchBaseURL = fetchBaseURL.substring(0, fetchBaseURL.length() - 5) + ".jpg";
        targetFilename = targetFilename.substring(0, targetFilename.length() - 5) + ".jpg";
        filesize = getFile(fetchBaseURL, targetFilename);
        ESP_LOGD(TAG, "Get file size: %d for %s", filesize, core.c_str());

        // Write received file to SD card
        if (filesize == 0)
        { // No video or image found on server, show error
            ESP_LOGI(TAG, "Missing picture or another error");
            tft->fillScreen(BLACK);
            showJpegImage(addPathAndExtension(PIC_ERROR, "jpg"), 0, 0, 0);
            footbanner("ERROR: Failed to fetch " + core + " from server");
            return false;
        }
        else
        {
            tft->fillScreen(BLACK);
            ESP_LOGI(TAG, "Image found, showing image from URL: %s", fetchBaseURL.c_str());
            showJpegImage(addPathAndExtension(core, "jpg"), 190, 120, 0);
            return true;
        }
    }
    return false;
}

//----------------------------------------------
// Add path and extension to core name
String addPathAndExtension(String core, String fileext)
{
    String prefix = core.substring(0, 1) + "/";
    prefix.toLowerCase();
    if (isDigit(prefix.charAt(0)))
        prefix = "numeric/";
    if (fileext == "jpg")
        return (folderjpg + prefix + core + "." + fileext);
    if (fileext == "mjpeg")
        return (foldermjpeg + prefix + core + "." + fileext);
    return (core + "." + fileext);
}

//----------------------------------------------
void showCore(const String &core)
{
    int result = false;

    if (videoPlay)
    { // Check if we should try playing videos (see settings.json)
        //    addPathAndExtension(core, "mjpg");
        // tbc: try playing video url
        ESP_LOGD(TAG, "Trying mjpg file: %s", core.c_str());
    }
    else
    {
        ESP_LOGD(TAG, "Trying jpg file: %s", core.c_str());
        if (!showCorenameURL(core))
        {
            if (!showLocalImage(PIC_ERROR))
            {
                ESP_LOGI(TAG, "Core image not found on server or locally: %s", core.c_str());
            }
            else
            {
                footbanner("Image for " + core + " not found");
                currentCore = core;
            }
        }
    }
}

//----------------------------------------------
void screenOn(void)
{
    ESP_LOGD(TAG, "Turn screen backlight on");
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
}

//----------------------------------------------
void screenOff(void)
{
    ESP_LOGD(TAG, "Turn screen backlight off");
    digitalWrite(TFT_BL, TFT_BACKLIGHT_OFF);
}

//----------------------------------------------
void cmdHWInfo(void)
{
    ESP_LOGI(TAG, "Hardware Information requested");
    char msg[128];
    sprintf(msg, "TFTESP32; %s; %02X:%02X:%02X:%02X:%02X:%02X", BUILD_VERSION, WiFi.macAddress()[0], WiFi.macAddress()[1], WiFi.macAddress()[2], WiFi.macAddress()[3], WiFi.macAddress()[4], WiFi.macAddress()[5]);
}

//----------------------------------------------
void screenRotation(int rotation)
{
    ESP_LOGD("CMD", "Rotation command received %d", rotation);
    if (rotation != currentRotation)
    {
        currentRotation = rotation;
        tft->setRotation(rotation);
        showLocalImage(currentCore);
    }
}

//----------------------------------------------
void screenText(char *sParam)
{
    ESP_LOGD("CMD", "Text Command received with text: %s", sParam);
    if (sParam == NULL)
    {
        return;
    }
    writetext(sParam, 1, DispWidth / 2 - ((strlen(sParam) * 10) / 2), 200, TFT_FONT_LARGE, 0, WHITE, false, "clear");
}
