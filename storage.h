#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>

// Callback signature: void on_line(const String& line)
typedef void (*csv_line_cb_t)(const String& line);
typedef void (*csv_chunk_cb_t)(const uint8_t* data, size_t len);

bool storageInit(bool formatOnFail);
bool fileExists(const char* path);
bool removeFile(const char* path);
bool createFile(const char* path, const String& csv);
bool appendToFile(const char* path, const String& row);
bool readFile(const char* path, String& out);
bool readFileByLine(const char* path, csv_line_cb_t stream);
bool readFileByChunks(const char* path, size_t chunkSize, csv_chunk_cb_t stream);

#endif // STORAGE_H
