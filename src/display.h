#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino_GFX_Library.h> 
   
#define DEFAULT_ROTATION 3

#define WHITE RGB565_WHITE
#define BLACK RGB565_BLACK
#define RED RGB565_RED
#define GREEN RGB565_GREEN
#define BLUE RGB565_BLUE
#define YELLOW RGB565_YELLOW
#define CYAN RGB565_CYAN
#define MAGENTA RGB565_MAGENTA
#define ORANGE RGB565_ORANGE
#define PINK RGB565_PINK
#define GREY RGB565_GREY
#define DARKGREY RGB565_DARKGREY
#define LIGHTGREY RGB565_LIGHTGREY
#define DARKRED RGB565_DARKRED
#define DARKGREEN RGB565_DARKGREEN
#define DARKBLUE RGB565_DARKBLUE

// TFT

#define MJPEG_BUFFER_SIZE (TFT_WIDTH * TFT_HEIGHT * 2 / 4) // memory for a single JPEG frame

#ifdef WAVESHARE
// Init sequence for Waveshare 3.16" 820x320 ST7701S display
static const uint8_t st7701_type10_init_operations[] = {
  BEGIN_WRITE,
  WRITE_COMMAND_8, 0xFF, WRITE_BYTES, 5, 0x77, 0x01, 0x00, 0x00, 0x13,
  WRITE_C8_D8, 0xEF, 0x08,
  WRITE_COMMAND_8, 0xFF, WRITE_BYTES, 5, 0x77, 0x01, 0x00, 0x00, 0x10,
  WRITE_C8_D16, 0xC0, 0xE5, 0x02,
  WRITE_C8_D16, 0xC1, 0x15, 0x0A,
  WRITE_C8_D16, 0xC2, 0x07, 0x02,
  WRITE_C8_D8,  0xCC, 0x10,
  WRITE_COMMAND_8, 0xB0, WRITE_BYTES, 16, 0x00, 0x08, 0x51, 0x0D, 0xCE, 0x06, 0x00, 0x08, 0x08, 0x24, 0x05, 0xD0, 0x0F, 0x6F, 0x36, 0x1F,
  WRITE_COMMAND_8, 0xB1, WRITE_BYTES, 16, 0x00, 0x10, 0x4F, 0x0C, 0x11, 0x05, 0x00, 0x07, 0x07, 0x18, 0x02, 0xD3, 0x11, 0x6E, 0x34, 0x1F,
  WRITE_COMMAND_8, 0xFF, WRITE_BYTES,  5, 0x77, 0x01, 0x00, 0x00, 0x11,
  WRITE_C8_D8, 0xB0, 0x4D,
  WRITE_C8_D8, 0xB1, 0x37,
  WRITE_C8_D8, 0xB2, 0x87,
  WRITE_C8_D8, 0xB3, 0x80,
  WRITE_C8_D8, 0xB5, 0x4A,
  WRITE_C8_D8, 0xB7, 0x85,
  WRITE_C8_D8, 0xB8, 0x21,
  WRITE_C8_D16, 0xB9, 0x00, 0x13,
  WRITE_C8_D8, 0xC0, 0x09,
  WRITE_C8_D8, 0xC1, 0x78,
  WRITE_C8_D8, 0xC2, 0x78,
  WRITE_C8_D8, 0xD0, 0x88,
  WRITE_COMMAND_8, 0xE0, WRITE_BYTES,  3, 0x80, 0x00, 0x02,
  DELAY, 100,
  WRITE_COMMAND_8, 0xE1, WRITE_BYTES, 11, 0x0F, 0xA0, 0x00, 0x00, 0x10, 0xA0, 0x00, 0x00, 0x00, 0x60, 0x60,
  WRITE_COMMAND_8, 0xE2, WRITE_BYTES, 13, 0x30, 0x30, 0x60, 0x60, 0x45, 0xA0, 0x00, 0x00, 0x46, 0xA0, 0x00, 0x00, 0x00,
  WRITE_COMMAND_8, 0xE3, WRITE_BYTES,  4, 0x00, 0x00, 0x33, 0x33,
  WRITE_C8_D16, 0xE4, 0x44, 0x44,
  WRITE_COMMAND_8, 0xE5, WRITE_BYTES, 16, 0x0F, 0x4A, 0xA0, 0xA0, 0x11, 0x4A, 0xA0, 0xA0, 0x13, 0x4A, 0xA0, 0xA0, 0x15, 0x4A, 0xA0, 0xA0,
  WRITE_COMMAND_8, 0xE6, WRITE_BYTES,  4, 0x00, 0x00, 0x33, 0x33,
  WRITE_C8_D16, 0xE7, 0x44, 0x44,
  WRITE_COMMAND_8, 0xE8, WRITE_BYTES, 16, 0x10, 0x4A, 0xA0, 0xA0, 0x12, 0x4A, 0xA0, 0xA0, 0x14, 0x4A, 0xA0, 0xA0, 0x16, 0x4A, 0xA0, 0xA0,
  WRITE_COMMAND_8, 0xEB, WRITE_BYTES,  7, 0x02, 0x00, 0x4E, 0x4E, 0xEE, 0x44, 0x00,
  WRITE_COMMAND_8, 0xED, WRITE_BYTES, 16, 0xFF, 0xFF, 0x04, 0x56, 0x72, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x27, 0x65, 0x40, 0xFF, 0xFF,
  WRITE_COMMAND_8, 0xEF, WRITE_BYTES,  6, 0x08, 0x08, 0x08, 0x40, 0x3F, 0x64,
  WRITE_COMMAND_8, 0xFF, WRITE_BYTES,  5, 0x77, 0x01, 0x00, 0x00, 0x13,
  WRITE_C8_D16, 0xE8, 0x00, 0x0E,
  WRITE_COMMAND_8, 0xFF, WRITE_BYTES,  5, 0x77, 0x01, 0x00, 0x00, 0x00,
  WRITE_C8_D8, 0x11, 0x00,
  DELAY, 120,
  WRITE_COMMAND_8, 0xFF, WRITE_BYTES,  5, 0x77, 0x01, 0x00, 0x00, 0x13,
  WRITE_COMMAND_8, 0xE8, WRITE_BYTES,  2, 0x00, 0x0C,
  DELAY, 10,
  WRITE_COMMAND_8, 0xE8, WRITE_BYTES,  2, 0x00, 0x00,
  WRITE_COMMAND_8, 0xFF, WRITE_BYTES,  5, 0x77, 0x01, 0x00, 0x00, 0x00,
  WRITE_C8_D8, 0x3A, 0x55,
  WRITE_C8_D8, 0x36, 0x00,
  WRITE_C8_D8, 0x35, 0x00,
  WRITE_C8_D8, 0x29, 0x00,
  DELAY, 20,
  END_WRITE,
};

#endif

#ifdef ILI9341_2_DRIVER
extern Arduino_GFX *tft;
#define TFT_FONT_SMALL  u8g2_font_6x12_tn 
#define TFT_FONT_NORMAL u8g2_font_helvB12_te
#define TFT_FONT_LARGE  u8g2_font_inr21_mf
#else
extern Arduino_RGB_Display *tft;
#define TFT_FONT_SMALL  u8g2_font_helvB12_te
#define TFT_FONT_NORMAL u8g2_font_inr21_mf
#define TFT_FONT_LARGE  u8g2_font_fub30_tf
#endif


extern int DispWidth;
extern int DispHeight;
extern const uint8_t *DEFAULT_FONT;
extern String currentCore;
extern bool videoPlay;

bool showLocalImage(String core);
bool showVideo(String actCorename, int videoposX, int videoposY);
void writetext(String text, int fixedpos, int textposX, int textposY, const uint8_t *fontname, int textrotation, int fontcolor, int backcolor, String clear);

void showCore(const String& coreName);
void screenOn(void);
void screenOff(void);
void cmdHWInfo(void);
void screenRotation(int rotation);
void screenText(char *sParam);
void clearScreen();


void tftInit();
void writetext(String text, int fixedpos, int textposX, int textposY, const uint8_t *fontname, int textrotation, int fontcolor, int backcolor, String clear);
void writetextcentered(String text, int textposY, const uint8_t *fontname, int textrotation, int fontcolor, int backcolor, String clear);
void footbanner(String bannertext);

bool showVideo(String actCorename, int videoposX, int videoposY);
void showJpegImage(String actCorename, int pictureposX, int pictureposY, int scale);
void rectfill(int posX, int posY, int rectwidth, int rectheight, int fillcolor);
bool showLocalImage(String core);
bool fetchfile(String fetchURL, String fetchfilename);
bool showImageURL(char *imageUrl);
bool showCorenameURL(String core);


String addPathAndExtension(String core, String fileext);

#endif