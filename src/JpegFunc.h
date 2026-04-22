/*******************************************************************************
 * JPEGDEC related function
 *
 * Dependent libraries:
 * JPEGDEC: https://github.com/bitbank2/JPEGDEC.git
 ******************************************************************************/
#ifndef _JPEGFUNC_H_
#define _JPEGFUNC_H_

#include <JPEGDEC.h>
#include "include.h"

// Increasing the buffer sizes does not seem to be necessary
// so commenting out for now .....
//
//#if JPEG_FILE_BUF_SIZE != 4096
//#warning "JPEG_FILE_BUF_SIZE not set to 4096 in JPEGDEC.h"
//#endif
//#if MAX_BUFFERED_PIXELS != 4096
//#warning "MAX_BUFFERED_PIXELS not set to 4096 in JPEGDEC.h"
//#endif

// JPEGDEC related variables
JPEGDEC _jpeg;
File _f;
int _x, _y, _x_bound, _y_bound;

// JPEGDEC callback functions
void *jpegOpenFile(const char *szFilename, int32_t *pFileSize);
void jpegCloseFile(void *pHandle);
int32_t jpegReadFile(JPEGFILE *pFile, uint8_t *pBuf, int32_t iLen);
int32_t jpegSeekFile(JPEGFILE *pFile, int32_t iPosition);
void jpegDraw(const char *filename, JPEG_DRAW_CALLBACK *jpegDrawCallback, bool useBigEndian, int x, int y, int widthLimit, int heightLimit, int _scale);

// Function definitions
void *jpegOpenFile(const char *szFilename, int32_t *pFileSize)
{
//  ESP_LOGI(TAG, "jpegOpenFile: %s", String(szFilename));
#ifdef USE_SPIFFS
  _f = SPIFFS.open(szFilename, FILE_READ);
#else
  _f = SD_MMC.open(szFilename, FILE_READ);
#endif

  *pFileSize = _f.size();
  return &_f;
}

void jpegCloseFile(void *pHandle)
{
  ESP_LOGD(TAG, "jpegCloseFile");
  File *f = static_cast<File *>(pHandle);
  f->close();
}

int32_t jpegReadFile(JPEGFILE *pFile, uint8_t *pBuf, int32_t iLen)
{
  ESP_LOGD(TAG, "jpegReadFile, iLen: %d", iLen);
  File *f = static_cast<File *>(pFile->fHandle);
  size_t r = f->read(pBuf, iLen);
  return r;
}

int32_t jpegSeekFile(JPEGFILE *pFile, int32_t iPosition)
{
  ESP_LOGD(TAG, "jpegSeekFile, pFile->iPos: %d, iPosition: %d\n", pFile->iPos, iPosition);
  File *f = static_cast<File *>(pFile->fHandle);
  f->seek(iPosition);
  return iPosition;
}

void jpegDraw(
    const char *filename, JPEG_DRAW_CALLBACK *jpegDrawCallback, bool useBigEndian,
    int x, int y, int widthLimit, int heightLimit, int _scale)
{
  ESP_LOGV(TAG, "Opening JPEG: %s", filename);
  ESP_LOGV(TAG, "Scale: %i", _scale);

  int ret = _jpeg.open(filename, jpegOpenFile, jpegCloseFile, jpegReadFile, jpegSeekFile, jpegDrawCallback);
  ESP_LOGD("JPEG", "JPEG open result: %d", ret);

  if (x == -1 && y == -1)
  {
    x = (widthLimit / 2) - (_jpeg.getWidth() / 2);
    y = (heightLimit / 2) - (_jpeg.getHeight() / 2);
  }
  _x = x;
  _y = y;
  _x_bound = _x + widthLimit - 1;
  _y_bound = _y + heightLimit - 1;

  // scale to fit height
  int iMaxMCUs;
  if (_scale == 0)
  {
    _scale = 0;
    iMaxMCUs = widthLimit / 16;
  }
  else if (_scale == 2)
  {
    _scale = JPEG_SCALE_HALF;
    iMaxMCUs = widthLimit / 8;
  }
  else if (_scale == 4)
  {
    _scale = JPEG_SCALE_QUARTER;
    iMaxMCUs = widthLimit / 4;
  }
  else if (_scale == 8)
  {
    _scale = JPEG_SCALE_EIGHTH;
    iMaxMCUs = widthLimit / 2;
  }

  _jpeg.setMaxOutputSize(iMaxMCUs);
  if (useBigEndian)
  {
    _jpeg.setPixelType(RGB565_BIG_ENDIAN);
  }
  _jpeg.decode(x, y, _scale);
  _jpeg.close();
}

#endif // _JPEGFUNC_H_