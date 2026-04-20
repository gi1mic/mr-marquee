#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <Arduino.h>
#include <WiFi.h>
#include <ESPFMfGK.h>
#include <FS.h>
#include <include.h>

#define FILE_MANAGER_PORT 80
extern ESPFMfGK filemgr;
extern const word filemanagerport;


void addFileSystems(void);
uint32_t checkFileFlags(fs::FS &fs, String filename, uint32_t flags);
void setupFilemanager(void);
int getFile(String url, String targetFilename);

#endif // FILESYSTEM_H
