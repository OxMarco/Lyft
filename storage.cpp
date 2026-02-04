#include "storage.h"
#include <LittleFS.h>

static bool g_fs_ready = false;

bool storageInit(bool formatOnFail = false) {
  if (g_fs_ready) return true;
  g_fs_ready = LittleFS.begin(formatOnFail);
  return g_fs_ready;
}

bool fileExists(const char* path) {
  if (!g_fs_ready) return false;
  return LittleFS.exists(path);
}

bool removeFile(const char* path) {
  if (!g_fs_ready) return false;
  return LittleFS.remove(path);
}

bool createFile(const char* path, const String& csv) {
  if (!g_fs_ready) return false;

  File f = LittleFS.open(path, "w"); // create new
  if (!f) return false;

  size_t n = f.print(csv);
  f.close();
  return (n == csv.length());
}

bool appendToFile(const char* path, const String& row) {
  if (!g_fs_ready) return false;

  File f = LittleFS.open(path, "a");   // append
  if (!f) return false;

  size_t n = f.print(row);
  f.close();
  return (n == row.length());
}

bool readFile(const char* path, String& out) {
  if (!g_fs_ready) return false;

  File f = LittleFS.open(path, "r");
  if (!f) return false;

  out = f.readString(); // reads until EOF
  f.close();
  return true;
}

// Stream line-by-line (better for large files / BLE chunking)
bool readFileByLine(const char* path, csv_line_cb_t stream) {
  if (!g_fs_ready || stream == nullptr) return false;

  File f = LittleFS.open(path, "r");
  if (!f) return false;

  while (f.available()) {
    String line = f.readStringUntil('\n');
    if (line.endsWith("\r")) line.remove(line.length() - 1); // handle CRLF
    stream(line);
  }

  f.close();
  return true;
}

// Read in fixed-size chunks (ideal for BLE notifications)
bool readFileByChunks(const char* path, size_t chunkSize, csv_chunk_cb_t stream) {
  if (!g_fs_ready || stream == nullptr || chunkSize == 0) return false;

  File f = LittleFS.open(path, "r");
  if (!f) return false;

  // long-lived buffer is NOT required; keep it local to avoid RAM pinning.
  // If you *want* static, you can make it and cap chunkSize.
  std::unique_ptr<uint8_t[]> buf(new uint8_t[chunkSize]);
  if (!buf) { f.close(); return false; }

  while (true) {
    int n = f.read(buf.get(), chunkSize);
    if (n <= 0) break;
    stream(buf.get(), (size_t)n);
  }

  f.close();
  return true;
}
