/*

********************************************************
    This is a modified version of the library
    allowing it to support custom ports
********************************************************


FileFetcher - A library for fetching files & images from the web

Copyright (c) 2023  Brian Lough.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#include "FileFetcher.h"
static const char *TAG = "FETCH";

//------------------------------------------------
FileFetcher::FileFetcher(Client &client)
{
    this->client = &client;
}

//------------------------------------------------
int FileFetcher::makeGetRequest(int portNumber, const char *command, const char *authorization, const char *accept, const char *host)
{
    client->flush();
    client->setTimeout(FILE_FETCHER_TIMEOUT);
    if (!client->connect(host, portNumber))
    {
        ESP_LOGI(TAG, "Connection failed");
        return -1;
    }

    yield(); // give the esp a breather

    // Send HTTP request
    client->print(F("GET "));
    client->print(command);
    client->println(F(" HTTP/1.0"));

    // Headers
    client->print(F("Host: "));
    client->println(host);

    if (accept != NULL)
    {
        client->print(F("Accept: "));
        client->println(accept);
    }

    if (authorization != NULL)
    {
        client->print(F("Authorization: "));
        client->println(authorization);
    }

    client->println(F("User-Agent: Arduino/Arduino"));
    client->println(F("Cache-Control: no-cache"));

    if (client->println() == 0)
    {
        ESP_LOGI(TAG, "Failed to send request");
        return -2;
    }
    return getHttpStatusCode();
}

//------------------------------------------------
String getHost(char *fileUrl)
{
    if (fileUrl == NULL)
    {
        return "";
    }

    int protocolLength = 0;
    if (strncmp(fileUrl, "https://", 8) == 0)
    { // Is HTTPS
        protocolLength = 8;
    }
    else if (strncmp(fileUrl, "http://", 7) == 0)
    { // Is HTTP
        protocolLength = 7;
    }
    else
    {
        ESP_LOGI(TAG, "Url not in expected format: %s (expected it to start with \"https://\" or \"http://\")", fileUrl);
        return "";
    }

    char *authorityStart = fileUrl + protocolLength;
    char *pathStart = strchr(authorityStart, '/');
    char *authorityEnd = (pathStart != NULL) ? pathStart : (fileUrl + strlen(fileUrl));
    char *portSeparator = strchr(authorityStart, ':');
    if (portSeparator != NULL && portSeparator < authorityEnd)
    {
        authorityEnd = portSeparator;
    }

    int hostLength = authorityEnd - authorityStart;
    ESP_LOGD(TAG, "Host len: %i", hostLength);
    if (hostLength <= 0)
    {
        return "";
    }

    String host = String(authorityStart).substring(0, hostLength);
    ESP_LOGD(TAG, "Host name: %s", host.c_str());

    return host;
}

//------------------------------------------------
int getPort(char *fileUrl)
{
    if (fileUrl == NULL)
    {
        return 80;
    }

    int protocolLength = 0;
    int portNumber = 80;

    if (strncmp(fileUrl, "https://", 8) == 0)
    { // Is HTTPS
        protocolLength = 8;
        portNumber = 443;
    }
    else if (strncmp(fileUrl, "http://", 7) == 0)
    { // Is HTTP
        protocolLength = 7;
        portNumber = 80;
    }
    else
    {
        ESP_LOGI(TAG, "Url not in expected format: %s (expected it to start with \"https://\" or \"http://\")", fileUrl);
    }

    char *authorityStart = fileUrl + protocolLength;
    char *pathStart = strchr(authorityStart, '/');
    char *authorityEnd = (pathStart != NULL) ? pathStart : (fileUrl + strlen(fileUrl));
    char *portSeparator = strchr(authorityStart, ':');

    if (portSeparator != NULL && portSeparator < authorityEnd)
    {
        int portLength = authorityEnd - (portSeparator + 1);
        if (portLength > 0)
        {
            String customPort = String(portSeparator + 1).substring(0, portLength);
            int parsedPort = customPort.toInt();
            if (parsedPort > 0)
            {
                portNumber = parsedPort;
            }
            else
            {
                ESP_LOGI(TAG, "Invalid custom port in URL: %s", fileUrl);
            }
        }
    }
    ESP_LOGD(TAG, "Port: %i", portNumber);
    return portNumber;
}

//------------------------------------------------
String getPath(char *fileUrl)
{
    if (fileUrl == NULL)
    {
        return "/";
    }

    int protocolLength = 0;
    if (strncmp(fileUrl, "https://", 8) == 0)
    {
        protocolLength = 8;
    }
    else if (strncmp(fileUrl, "http://", 7) == 0)
    {
        protocolLength = 7;
    }

    char *pathStart = strchr(fileUrl + protocolLength, '/');
    if (pathStart == NULL)
    {
        ESP_LOGD(TAG, "Path: /");
        return "/";
    }

    int pathLength = strlen(pathStart);
    ESP_LOGD(TAG, "Path len:  %i", pathLength);

    String path = String(pathStart);
    ESP_LOGD(TAG, "Path:  %s", path.c_str());

    return path;
}

//------------------------------------------------
int FileFetcher::commonGetFile(char *fileUrl)
{
    ESP_LOGD(TAG, "Parsing file URL: %s", fileUrl);

    String hostNew = getHost(fileUrl);
    int portNew = getPort(fileUrl);
    String pathNew = getPath(fileUrl);

    int statusCode = makeGetRequest(portNew, pathNew.c_str(), NULL, "*/*", hostNew.c_str());
    ESP_LOGD(TAG, "Status Code:  %i", statusCode);
    if (statusCode == 200)
    {
        return getContentLength();
    }
    return -1; // Failed
}

//------------------------------------------------
bool FileFetcher::getFile(char *fileUrl, Stream *file)
{
    if (file == nullptr || fileUrl == nullptr)
    {
        ESP_LOGE(TAG, "Invalid argument(s)");
        return false;
    }
    int totalLength = commonGetFile(fileUrl);
    ESP_LOGD(TAG, "File length: %i", totalLength);
    if (totalLength <= 0)
    {
        closeClient();
        return false;
    }

    skipHeaders();
    int remaining = totalLength;
    int amountRead = 0;
    uint8_t buff[128] = {0};

    const uint32_t readTimeoutMs = 10000;
    uint32_t lastDataMs = millis();

    while ((client->connected() || client->available()) && remaining > 0)
    {
        size_t availableBytes = client->available();
        if (availableBytes > 0)
        {
            size_t toRead = availableBytes;
            if (toRead > sizeof(buff))
            {
                toRead = sizeof(buff);
            }
            if (toRead > static_cast<size_t>(remaining))
            {
                toRead = static_cast<size_t>(remaining);
            }

            int c = client->readBytes(buff, toRead);
            if (c <= 0)
            {
                ESP_LOGE(TAG, "Socket read failed");
                break;
            }

            size_t written = file->write(buff, static_cast<size_t>(c));
            if (written != static_cast<size_t>(c))
            {
                ESP_LOGE(TAG, "Failed to write fetched data to target stream");
                break;
            }
            amountRead += c;
            remaining -= c;
            lastDataMs = millis();
        }
        else
        {
            if (millis() - lastDataMs > readTimeoutMs)
            {
                ESP_LOGE(TAG, "Read timeout");
                break;
            }
            delay(1);
        }
        yield();
    }
    closeClient();

    if (amountRead != totalLength)
    {
        ESP_LOGE(TAG, "Incomplete read: got %d of %d bytes", amountRead, totalLength);
        return false;
    }
    ESP_LOGD(TAG, "Finished getting file (%d bytes)", amountRead);
    return true;
}

//------------------------------------------------
bool FileFetcher::getFile(char *fileUrl, uint8_t **file, int *fileLength)
{
    if (file == nullptr || fileLength == nullptr || fileUrl == nullptr)
    {
        ESP_LOGE(TAG, "Invalid argument(s)");
        return false;
    }

    *file = nullptr;
    *fileLength = 0;

    ESP_LOGD(TAG, "FileURL: %s", fileUrl);

    int totalLength = commonGetFile(fileUrl);
    ESP_LOGD(TAG, "File length: %d", totalLength);

    if (totalLength <= 0)
    {
        ESP_LOGE(TAG, "Invalid/unknown file length: %d", totalLength);
        closeClient();
        return false;
    }

    skipHeaders();

    uint8_t *imgPtr = static_cast<uint8_t *>(
        heap_caps_malloc(static_cast<size_t>(totalLength), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));

    if (imgPtr == nullptr)
    {
        ESP_LOGE(TAG, "Memory allocation failed (%d bytes)", totalLength);
        closeClient();
        return false;
    }

    int remaining = totalLength;
    int amountRead = 0;
    uint8_t buff[128];
    const uint32_t readTimeoutMs = 10000; // Optional: timeout to avoid hanging forever
    uint32_t lastDataMs = millis();

    ESP_LOGD(TAG, "Fetching file...");
    while ((client->connected() || client->available()) && remaining > 0)
    {
        size_t availableBytes = client->available();
        if (availableBytes > 0)
        {
            size_t toRead = availableBytes;
            if (toRead > sizeof(buff))
                toRead = sizeof(buff);
            if (toRead > static_cast<size_t>(remaining))
                toRead = static_cast<size_t>(remaining);

            int c = client->readBytes(buff, toRead);
            if (c <= 0)
            {
                ESP_LOGE(TAG, "Socket read failed");
                break;
            }
            memcpy(imgPtr + amountRead, buff, static_cast<size_t>(c));
            amountRead += c;
            remaining -= c;
            lastDataMs = millis();
        }
        else
        {
            // No data currently available; enforce timeout
            if (millis() - lastDataMs > readTimeoutMs)
            {
                ESP_LOGE(TAG, "Read timeout");
                break;
            }
            yield();
        }
    }

    closeClient();

    if (amountRead != totalLength)
    {
        ESP_LOGE(TAG, "Incomplete read: got %d of %d bytes", amountRead, totalLength);
        heap_caps_free(imgPtr);
        return false;
    }

    *file = imgPtr;
    *fileLength = totalLength;
    ESP_LOGD(TAG, "Finished getting file (%d bytes)", amountRead);
    return true;
}

//------------------------------------------------
int FileFetcher::getContentLength()
{
    if (client->find("Content-Length:"))
    {
        int contentLength = client->parseInt();
        ESP_LOGD(TAG, "Content-Length: %i", contentLength);
        return contentLength;
    }
    return -1;
}

//------------------------------------------------
void FileFetcher::skipHeaders()
{
    if (!client->find("\r\n\r\n"))
    {
        ESP_LOGI(TAG, "Invalid response");
        return;
    }
}

//------------------------------------------------
int FileFetcher::getHttpStatusCode()
{
    char status[32] = {0};
    client->readBytesUntil('\r', status, sizeof(status));
    ESP_LOGD(TAG, "Status: %s", status);
    char *token;
    token = strtok(status, " "); // https://www.tutorialspoint.com/c_standard_library/c_function_strtok.htm

    ESP_LOGD(TAG, "HTTP Version: %s", token);

    if (token != NULL && (strcmp(token, "HTTP/1.0") == 0 || strcmp(token, "HTTP/1.1") == 0))
    {
        token = strtok(NULL, " ");
        if (token != NULL)
        {
            ESP_LOGD(TAG, "Status Code: %s", token);
            return atoi(token);
        }
    }
    return -1;
}

//------------------------------------------------
void FileFetcher::closeClient()
{
    if (client->connected())
    {
        ESP_LOGD(TAG, "Closing client");
        client->stop();
    }
}