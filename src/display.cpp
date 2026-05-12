#include "include.h"
#include "display.h"
#include "JpegFunc.h"
#include "MjpegClass.h"
#include <WiFi.h>
#include "filesystem.h"
#include "network.h"
#include "FileFetcher.h"
#include <UrlEncode.h>
#include <Wire.h>

static const char *TAG = "DISP";

String currentCore = ""; // Corename
String currentGame = ""; // Current running game as provided by MiSTer Remote
int currentRotation = DEFAULT_ROTATION;
bool videoPlay = true;

String folderjpg = "/jpg/";
String foldermjpeg = "/mjpeg/";
String dirletter = "";

#ifdef USE_IMU
#include "FastIMU.h"
// #define PERFORM_CALIBRATION // Comment to disable IMU startup calibration
// #define IMU_SDA 15
// #define IMU_SCL 7
// #define IMU_ADDRESS 0x6b
QMI8658 imu; // Change to the name of any supported IMU!

calData calib = {0}; // Calibration data
AccelData accelData; // Sensor data
GyroData gyroData;
MagData magData;

#endif

#ifdef WAVESHARE

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
#endif

#ifdef ILI9341_2_DRIVER
Arduino_DataBus *bus = new Arduino_HWSPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO);
Arduino_GFX *tft = new Arduino_ILI9341(bus, TFT_RST);
#endif

#ifdef HUB75
MatrixPanel_I2S_DMA *tft = nullptr;
#endif

static int jpegDrawCallback(JPEGDRAW *pDraw);
static MjpegClass mjpeg;

int DispWidth = 0;
int DispHeight = 0;

//----------------------------------------------
void tftInit()
{

#ifdef USE_IMU
    if (!Wire.begin(IMU_SDA, IMU_SCL))
    {
        Serial.println("WIRE ERROR !!");
    }
    Wire.setClock(400000); // 400khz clock

    int err = imu.init(calib, IMU_ADDRESS);
    if (err != 0)
    {
        Serial.print("Error initializing IMU: ");
        Serial.println(err);
        while (true)
        {
            ;
        }
    }

#ifdef PERFORM_CALIBRATION
    Serial.println("FastIMU calibration & data example");
    if (IMU.hasMagnetometer())
    {
        delay(1000);
        Serial.println("Move IMU in figure 8 pattern until done.");
        delay(3000);
        IMU.calibrateMag(&calib);
        Serial.println("Magnetic calibration done!");
    }
    else
    {
        delay(5000);
    }
#endif
#endif

#ifdef HUB75
    HUB75_I2S_CFG::i2s_pins _pins = {R1_PIN, G1_PIN, B1_PIN, R2_PIN, G2_PIN, B2_PIN, A_PIN, B_PIN, C_PIN, D_PIN, E_PIN, LAT_PIN, OE_PIN, CLK_PIN};

    // Module configuration
    HUB75_I2S_CFG mxconfig(
        TFT_HEIGHT,  // module width
        TFT_WIDTH,   // module height
        PANEL_CHAIN, // Chain length
        _pins        // pin mapping
    );

    // mxconfig.clkphase = false;
    // mxconfig.driver = HUB75_I2S_CFG::FM6126A;

    tft = new MatrixPanel_I2S_DMA(mxconfig);
    tft->begin();

    tft->setBrightness8(125); // 0-255
    tft->clearScreen();
    tft->setTextSize(1);     // size 1 == 8 pixels high
    tft->setTextWrap(false); // Don't wrap at end of line - will do ourselves
    tft->setCursor(5, 0);    // start at top left, with 8 pixel of spacing

    // HUB75 test code
    //    tft->fillScreen(RED);
    //    tft->setTextColor(tft->color444(15, 0, 0));
    //    tft->println("Mr Marquee!");
    //    delay(3000);
    //    tft->fillScreen(GREEN);
    //    tft->setTextColor(tft->color444(15, 0, 0));
    //    tft->println("Mr Marquee!");
    //    delay(3000);
    //    tft->fillScreen(BLUE);
    //    tft->setTextColor(tft->color444(0, 15, 0));
    //    tft->println("Mr Marquee!");
    //    delay(3000);
    //    tft->fillScreen(WHITE);
    //    tft->setTextColor(tft->color444(0, 0, 15));
    //    tft->println("Mr Marquee!");
    //    delay(3000);
    //    // draw an 'X' in red
    //    tft->drawLine(0, 0, tft->width() - 1, tft->height() - 1, tft->color444(15, 0, 0));
    //    tft->drawLine(tft->width() - 1, 0, 0, tft->height() - 1, tft->color444(15, 0, 0));
    //    delay(3000);
    //    tft->fillScreen(WHITE);
    //    // draw a blue circle
    //    tft->drawCircle(10, 10, 10, tft->color444(0, 0, 15));
    //    delay(3000);

#else
    tft->begin();
#endif
    tft->setRotation(DEFAULT_ROTATION);
    tft->invertDisplay(true);
    tft->fillScreen(BLACK);
    DispWidth = tft->width();
    DispHeight = tft->height();
#ifndef HUB75
    tft->setFont(TFT_FONT_NORMAL);
#endif
    mjpeg_buf = (uint8_t *)malloc(MJPEG_BUFFER_SIZE); // Video buffer
    ESP_LOGI(TAG, "Display width: %d, height: %d\n", DispWidth, DispHeight);
}

//----------------------------------------------
void clearScreen()
{
    tft->fillScreen(BLACK);
}

//----------------------------------------------
void writetext(String text, int fixedpos, int textposX, int textposY, int textrotation, int fontcolor, int backcolor, String clear)
{
    if (tft == nullptr)
    {
        ESP_LOGE(TAG, "Error: tft is not initialized!");
        return;
    }
    ESP_LOGD(TAG, "writetext: %s", text.c_str());
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
void writetextcentered(String text, int textposY, int textrotation, int fontcolor, int backcolor, String clear)
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
void showPayload(String core, String gameName)
{
    if (core == "")
        core = "MENU";
    ESP_LOGI(TAG, "Core = %s currentCore: %s | gameName = %s currentGame: %s ", core.c_str(), currentCore.c_str(), gameName.c_str(), currentGame.c_str());

    core.trim(); // Fix the issue of ScummVM adding a \r to the end of the payload

    if ((gameName != currentGame) || (core != currentCore))
    {
        currentCore = core;
        currentGame = gameName;
        if (core == "MENU")
        {
            ESP_LOGI(TAG, "Showing local: %s", core);
            clearScreen();
            showLocalFile(PIC_MENU);
        }
        else
        {
            ESP_LOGI(TAG, "Showing URL Core: %s ", core);
            clearScreen();
            showURLCore(core, urlEncode(gameName));
        }
    }
}

//----------------------------------------------
void showJpegImage(String core, int pictureposX, int pictureposY, int scale)
{
    if (tft == nullptr)
    {
        ESP_LOGE(TAG, "Error: tft is not initialized!");
        return;
    }
    ESP_LOGD(TAG, "Play pic: %s", core.c_str());
    jpegDraw(core.c_str(), jpegDrawCallback, true, pictureposX, pictureposY, DispWidth, DispHeight, scale);
    ESP_LOGD(TAG, "Jpeg decode status %i", _jpeg.getLastError());
}

//----------------------------------------------
bool showLocalFile(String core)
{
    String fqn = "/" + core + ".mjpg";

    if (videoPlay) // Check if we should try playing videos (see settings.json on the server)
    {
        if (showLocalVideo(core, 0, 0))
        {
            return true;
        }
    }

    fqn = "/" + core + ".jpg";
    ESP_LOGI(TAG, "Showing local pic: %s", fqn.c_str());
#ifdef USE_INTERNAL_SPIFFS
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
bool showURLCoreImage(String core)
{
    String fetchBaseURL = MISTER_WEBSERVER;
    String prefix = core.substring(0, 1) + "/";

    if (PORTRAIT_SCREEN == true)
    {
        ESP_LOGD(TAG, "Portrait :%s", PORTRAIT_SCREEN ? "True" : "False");
        fetchBaseURL = fetchBaseURL + "/" + DispHeight + "x" + DispWidth;
    }
    else
    {
        ESP_LOGD(TAG, "Portrait :%s", PORTRAIT_SCREEN ? "True" : "False");
        fetchBaseURL = fetchBaseURL + "/" + DispWidth + "x" + DispHeight;
    }
    fetchBaseURL = fetchBaseURL + addPathAndExtension(core, "jpg");

    char charArray[200];
    fetchBaseURL.toCharArray(charArray, sizeof(charArray));

    return (showImageURL(charArray));
}

//----------------------------------------------
bool showImageURL(char *imageUrl)
{
    ESP_LOGI(TAG, "Showing URL: %s", imageUrl);

    uint8_t *imageFile; // pointer that the library will store the image at (uses malloc)
    int imageSize;      // library will return the size of the image
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

//---------------------------------------------
bool showVideoURL(char *videoUrl)
{
    ESP_LOGI(TAG, "Showing video URL: %s", videoUrl);
    Stream *videoStream = nullptr; // Stream pointer that the library will use to read the video (uses malloc)

    videoStream = fileFetcher.getFileStream(videoUrl);

    if (videoStream == nullptr)
    {
        ESP_LOGE(TAG, "Failed to get video stream");
        return false;
    }
    mjpeg.setup(videoStream, mjpeg_buf, jpegDrawCallback, true, 0, 0, DispWidth, DispHeight);
    while (videoStream->available())
    {
        mjpeg.readMjpegBuf(); // Read video
        mjpeg.drawJpg();
    }
    return true;
}

//----------------------------------------------
bool showURLVideo(String core)
{
    String fetchBaseURL = MISTER_WEBSERVER;
    String prefix = core.substring(0, 1) + "/";

    if (PORTRAIT_SCREEN == true)
    {
        ESP_LOGD(TAG, "Portrait :%s", PORTRAIT_SCREEN ? "True" : "False");
        fetchBaseURL = fetchBaseURL + "/" + DispHeight + "x" + DispWidth;
    }
    else
    {
        ESP_LOGD(TAG, "Portrait :%s", PORTRAIT_SCREEN ? "True" : "False");
        fetchBaseURL = fetchBaseURL + "/" + DispWidth + "x" + DispHeight;
    }
    fetchBaseURL = fetchBaseURL + "/mjpg/" + prefix;
    fetchBaseURL = fetchBaseURL + addPathAndExtension(core, "mjpg");

    char charArray[200];
    fetchBaseURL.toCharArray(charArray, sizeof(charArray));

    return (showVideoURL(charArray));
}

//----------------------------------------------
bool showLocalVideo(String core, int videoposX, int videoposY)
{
    String fqn = "/" + core + ".mjpg";
    ESP_LOGI(TAG, "Play video: %s", fqn.c_str());
    File filehandle;
#ifdef USE_INTERNAL_SPIFFS
    filehandle = SPIFFS.open(fqn, FILE_READ);
#else
    filehandle = SD_MMC.open(fqn, FILE_READ);
#endif
    if (!filehandle || filehandle.isDirectory())
    {
        ESP_LOGI(TAG, "ERROR: Failed to open %s file for reading", fqn);
        // writetextcentered("ERROR: Failed to open " + fqn + " file for reading", FOOTER_LINE, 0, WHITE, false, "noclear");
        return false;
    }
    else
    {
        if (!mjpeg_buf)
        {
            ESP_LOGI(TAG, "mjpeg_buf malloc failed!");
            writetextcentered("ERROR: mjpeg_buf malloc failed!", FOOTER_LINE, 0, WHITE, false, "noclear");
            return false;
        }
        else
        {

            ESP_LOGD(TAG, "MJPEG start");

            mjpeg.setup(&filehandle, mjpeg_buf, jpegDrawCallback, true, 0, 0, DispWidth, DispHeight);
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

#ifdef HUB75
//----------------------------------------------
// This is a Big endian RGB565 bitmap drawing function, for JPEG decoder.
// The HUB75 library expects little endian, so we need to swap the bytes.
// This is used as the callback for JPEG drawing and video playback.
void drawRGBBeBitmap(int16_t x, int16_t y, uint16_t *bitmap, int16_t w, int16_t h)
{
    for (int16_t j = 0; j < h; j++, y++)
    {
        for (int16_t i = 0; i < w; i++)
        {
            tft->drawPixel(x + i, y, (bitmap[j * w + i] >> 8) | (bitmap[j * w + i] << 8));
        }
    }
}
#endif
//----------------------------------------------
// pixel drawing callback
static int jpegDrawCallback(JPEGDRAW *pDraw)
{
#ifdef HUB75
    drawRGBBeBitmap(pDraw->x, pDraw->y, (uint16_t *)pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
#else
    tft->draw16bitBeRGBBitmap(pDraw->x, pDraw->y, pDraw->pPixels, pDraw->iWidth, pDraw->iHeight);
#endif
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
        fetchBaseURL = fetchBaseURL + DispHeight + "x" + DispWidth + "/jpg/" + prefix;
    }
    else
    {
        ESP_LOGD(TAG, "Portrait :%s", PORTRAIT_SCREEN ? "True" : "False");
        fetchBaseURL = fetchBaseURL + DispWidth + "x" + DispHeight + "/jpg/" + prefix;
    }

    fetchBaseURL = fetchBaseURL + addPathAndExtension(core, "mjpg");
    targetFilename = targetFilename + addPathAndExtension(core, "mjpg");

    filesize = getFile(fetchBaseURL, targetFilename);
    ESP_LOGI(TAG, "Getting file %s", targetFilename.c_str());
    if (filesize != 0)
    {
        tft->fillScreen(BLACK);
        ESP_LOGD(TAG, "URL Video found stub");
        return true;
    }
    else
    { // Try for image if video not found
        fetchBaseURL = fetchBaseURL.substring(0, fetchBaseURL.length() - 5) + ".jpg";
        targetFilename = targetFilename.substring(0, targetFilename.length() - 5) + ".jpg";
        filesize = getFile(fetchBaseURL, targetFilename);
        ESP_LOGD(TAG, "Get file size: %d for %s", filesize, core.c_str());

        if (filesize == 0)
        { // No video or image found on server, show error image
            ESP_LOGI(TAG, "Missing picture or another error");
            tft->fillScreen(BLACK);
            showJpegImage(addPathAndExtension(PIC_ERROR, "jpg"), 0, 0, 0);
            writetextcentered("ERROR: Failed to fetch " + core + " from server", FOOTER_LINE, 0, WHITE, false, "noclear");
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
void showURLCore(const String &core, const String &gameName)
{
    int result = false;

    if (videoPlay) // Check if we should play video (see settings.json on the server)
    {
        if (showURLVideo(core)) // If video found and played, return else try for image
        {
            return;
        }
    }

    ESP_LOGI(TAG, "Trying jpg file: %s - %s", core.c_str(), gameName.c_str());
    if (!showURLCoreImage(core + "-" + gameName))
    {
        ESP_LOGI(TAG, "Trying jpg file: %s", core.c_str());
        if (!showURLCoreImage(core))
        {
            if (!showLocalFile(PIC_ERROR))
            {
                ESP_LOGI(TAG, "Core image not found on server or locally: %s", core.c_str());
            }
            else
            {
                writetextcentered("Image for " + core + " not found", FOOTER_LINE, 0, WHITE, false, "noclear");
                currentCore = core;
            }
        }
    }
}

//----------------------------------------------
void screenOn(void)
{
#ifndef HUB75
    ESP_LOGV(TAG, "Turn screen backlight on");
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
#endif
}

//----------------------------------------------
void screenOff(void)
{
    ESP_LOGV(TAG, "Turning screenoff");
#ifdef HUB75
    clearScreen();
#else
    digitalWrite(TFT_BL, TFT_BACKLIGHT_OFF);
#endif
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
    ESP_LOGV("CMD", "Rotation command received %d", rotation);
    if (rotation != currentRotation)
    {
        currentRotation = rotation;
        tft->setRotation(rotation);
        currentCore = "....."; // Force redraw
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
#ifdef HUB75
    writetext(sParam, 1, DispWidth / 2 - ((strlen(sParam) * 10) / 2), 200, 0, WHITE, false, "clear");
#else
    writetext(sParam, 1, DispWidth / 2 - ((strlen(sParam) * 10) / 2), 200, TFT_FONT_LARGE, 0, WHITE, false, "clear");
#endif
}

//-----------------------------------------------
void screenAutoRotation()
{
#ifdef USE_IMU
    imu.update(); // Read the sensor data
    imu.getAccel(&accelData);
    imu.getGyro(&gyroData);

    // Determine current orientation
    if (abs(accelData.accelX) > abs(accelData.accelY))
    {
        if (accelData.accelX > 0)
        {
            screenRotation(3);
            ESP_LOGV(TAG, "Landscape Right");
        }
        else
        {
            screenRotation(1);
            ESP_LOGV(TAG, "Landscape Left");
        }
    }
    else
    {
        if (accelData.accelY > 0)
        {
            screenRotation(0);
            ESP_LOGV(TAG, "Portrait");
        }
        else
        {
            screenRotation(2);
            ESP_LOGV(TAG, "Portrait Upside Down");
        }
    }
#endif
}