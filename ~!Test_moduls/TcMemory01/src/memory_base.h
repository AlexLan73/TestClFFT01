// memory_base.h
#pragma once

#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <thread>
#include <atomic>
#include <iostream>


enum class TypeBlockMemory {
  Read,
  Write
};
using MetadataMap = std::map<std::string, std::string>;

// Структура для передачи данных, как в C# RecDataMetaData
struct RecDataMetaData {
  std::vector<char> Bytes;
  MetadataMap MetaData;
};


class MemoryBase {
public:
  MemoryBase(const std::string& nameMemory,
    TypeBlockMemory typeBlockMemory,
    size_t dataSegmentSize,
    std::function<void(RecDataMetaData)> callBack);

  ~MemoryBase();

  // Запрет копирования
  MemoryBase(const MemoryBase&) = delete;
  MemoryBase& operator=(const MemoryBase&) = delete;

  // --- НОВЫЕ И ОБНОВЛЕННЫЕ МЕТОДЫ, АНАЛОГИЧНЫЕ C# ---
     // 1. Запись основного блока данных и метаданных (аналог WriteDataToMemory)
  void WriteData(const std::vector<unsigned char>& data, const MetadataMap& metadata);

  // 2. Чтение только контрольного блока (аналог GetCommandControl)
  MetadataMap GetCommandControl();

  // 3. Запись только в контрольный блок (аналог SetCommandControl)
  void SetCommandControl(const MetadataMap& metadata);

  // 4. Очистка контрольного блока (аналог ClearCommandControl)
  void ClearCommandControl();

private:
  void event_loop();
  MetadataMap parse_control_string(const char* str);
  std::string format_control_string(const MetadataMap& metadata);


  // --- Win32 API хэндлы ---
  HANDLE hControlMapFile = nullptr; // Хэндл для контрольной памяти
  HANDLE hDataMapFile = nullptr;    // Хэндл для основной памяти
  HANDLE hEvent = nullptr;          // Хэндл для события (аналог EventWaitHandle)

  // --- Имена ---
  std::string _nameMemoryData;
  std::string _nameMemoryDataControl;

  // --- Потоки и колбэки ---
  std::function<void(RecDataMetaData)> _callBack;
  std::thread _eventThread;
  std::atomic<bool> _running{ false };

  const size_t CONTROL_SIZE = 8 * 1024; // 8KB, как в C#
  int _dataSegmentSize;
};

