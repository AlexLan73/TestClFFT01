#pragma once
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <msgpack.hpp>
#include <yaml-cpp/yaml.h>







//
//class MemoryBase {
//
//public:
//  enum class TypeBlockMemory { Read, Write };
//
//  MemoryBase(const std::wstring& nameMemory, TypeBlockMemory typeBlockMemory,
//        std::function<void(const std::string&)> callback = nullptr)
//        : _nameMemoryData(nameMemory),
//        _nameMemoryDataControl(nameMemory + L"Control"),
//        _eventNameMemoryDataControl(L"Global\\Event" + nameMemory),
//        _callback(callback),
//        _shouldStop(false)
//    {
//        // Создаем или открываем Memory-Mapped File для контроля
//        _hMapFileControl = CreateFileMapping(
//            INVALID_HANDLE_VALUE,    // Используем файл подкачки
//            NULL,                   // Дескриптор безопасности по умолчанию
//            PAGE_READWRITE,         // Доступ на чтение/запись
//            0,                      // Максимальный размер (старшее слово)
//            _sizeDataControl,        // Максимальный размер (младшее слово)
//            _nameMemoryDataControl.c_str());
//
//        if (_hMapFileControl == NULL) {
//            throw std::runtime_error("Не удалось создать FileMapping");
//        }
//
//        // Создаем или открываем событие
//        //_hEvent = CreateEvent(
//        //    NULL,               // Атрибуты безопасности
//        //    FALSE,              // Auto-reset event
//        //    FALSE,              // Начальное состояние несигнальное
//        //    _eventNameMemoryDataControl.c_str());
//        _hEvent = CreateEvent(
//            NULL,               // Атрибуты безопасности
//            FALSE,              // Manual-reset = FALSE (Auto-reset)
//            FALSE,              // Начальное состояние
//            _eventNameMemoryDataControl.c_str());
//
//        if (_hEvent == NULL) {
//            CloseHandle(_hMapFileControl);
//            throw std::runtime_error("Не удалось создать событие");
//        }
//
//        if (typeBlockMemory == TypeBlockMemory::Read && callback) {
//            _eventThread = std::thread(&MemoryBase::WaitDataControlEvent, this);
//        }
//    }
//
//  ~MemoryBase() {
//        _shouldStop = true;
//        SetEvent(_hEvent); // Прерываем ожидание
//        if (_eventThread.joinable()) {
//            _eventThread.join();
//        }
//        CloseHandle(_hEvent);
//        CloseHandle(_hMapFileControl);
//  }
//
//  void SetCommandControl(const std::string& sData) {
//        std::lock_guard<std::mutex> lock(_mutex);
//
//        // Получаем доступ к памяти
//        LPVOID pBuf = MapViewOfFile(
//            _hMapFileControl,   // Дескриптор объекта проекции
//            FILE_MAP_ALL_ACCESS, // Доступ на чтение/запись
//            0,
//            0,
//            _sizeDataControl);
//
//        if (pBuf == NULL) {
//            throw std::runtime_error("Не удалось отобразить вид файла");
//        }
//
//        // Копируем данные
//        ZeroMemory(pBuf, _sizeDataControl);
//        size_t dataSize = std::min<size_t>(sData.size(), static_cast<size_t>(_sizeDataControl));
//        memcpy(pBuf, sData.c_str(), dataSize);
//
//        UnmapViewOfFile(pBuf);
//
//        // Сигнализируем о новых данных
//        SetEvent(_hEvent);
//    }
//
//        // Дополнительный метод для записи объектов (с сериализацией)
//  void WriteControlHeader(size_t dataSize, const std::string& typeName) {
//    std::string header = "";// CreateControlHeader(dataSize, typeName);
//    header.resize(_sizeDataControl, '\0');
//
//    LPVOID pBuf = MapViewOfFile(_hMapFileControl, FILE_MAP_WRITE, 0, 0, _sizeDataControl);
//    if (pBuf) {
//      memcpy(pBuf, header.c_str(), _sizeDataControl);
//      UnmapViewOfFile(pBuf);
//    }
//  }
//
//    template<typename T>
//  void WriteObject(const T& obj) {
//    // 1. Сериализация
//    msgpack::sbuffer sbuf;
//    msgpack::pack(sbuf, obj);
//
//    // 2. Подготовка заголовка
//    std::string header = CreateHeader(sbuf.size(), typeid(T).name());
//    header.resize(256, '\0'); // Фиксированный размер заголовка
//
//    // 3. Запись заголовка
//    {
//      std::lock_guard<std::mutex> lock(_mutex);
//      LPVOID pBuf = MapViewOfFile(_hMapFileControl, FILE_MAP_WRITE, 0, 0, 256);
//      memcpy(pBuf, header.c_str(), 256);
//      UnmapViewOfFile(pBuf);
//    }
//
//    // 4. Запись данных
//    this->WriteByteData(std::vector<BYTE>(sbuf.data(), sbuf.data() + sbuf.size()));
//
//    // 5. Уведомление (после записи всего!)
//    this->SetCommandControl("data_ready");
//  }
//
//  // Дополнительный метод для чтения объектов (с десериализацией)
//
//  template<typename T>
//  T ReadObject(size_t bufferSize = 1024) {
//    try {
//      // Чтение с проверкой размера
//      auto bytes = this->ReadMemoryData(bufferSize);
//
//      // Определяем реальный размер данных (без нулевых байтов)
//      size_t actualSize = 0;
//      while (actualSize < bytes.size() && bytes[actualSize] != 0) {
//        actualSize++;
//      }
//
//      if (actualSize == 0) {
//        throw std::runtime_error("No data available in shared memory");
//      }
//
//      // Десериализация с проверкой
//      msgpack::object_handle oh;
//      try {
//        oh = msgpack::unpack(reinterpret_cast<const char*>(bytes.data()), actualSize);
//      }
//      catch (const msgpack::insufficient_bytes&) {
//        throw std::runtime_error("Insufficient data for deserialization");
//      }
//
//      try {
//        return oh.get().as<T>();
//      }
//      catch (const msgpack::type_error&) {
//        throw std::runtime_error("Type mismatch during deserialization");
//      }
//    }
//    catch (const std::exception& e) {
//      throw std::runtime_error(std::string("Deserialization failed: ") + e.what());
//    }
//  }
//
//protected:
//  std::vector<BYTE> ReadMemoryData(size_t sizeData) {
//        // Создаем/открываем Memory-Mapped File для данных
//        HANDLE hMapFile = CreateFileMapping(
//            INVALID_HANDLE_VALUE,
//            NULL,
//            PAGE_READWRITE,
//            0,
//            static_cast<DWORD>(sizeData),
//            _nameMemoryData.c_str());
//
//        if (hMapFile == NULL) {
//            throw std::runtime_error("Не удалось создать FileMapping для данных");
//        }
//
//        LPVOID pBuf = MapViewOfFile(
//            hMapFile,
//            FILE_MAP_READ,
//            0,
//            0,
//            sizeData);
//
//        if (pBuf == NULL) {
//            CloseHandle(hMapFile);
//            throw std::runtime_error("Не удалось отобразить вид файла данных");
//        }
//
//        // Копируем данные в вектор
//        std::vector<BYTE> buffer(sizeData);
//        memcpy(buffer.data(), pBuf, sizeData);
//
//        UnmapViewOfFile(pBuf);
//        CloseHandle(hMapFile);
//
//        return buffer;
//    }
//
//  void WriteByteData(const std::vector<BYTE>& bytes) {
//        HANDLE hMapFile = CreateFileMapping(
//            INVALID_HANDLE_VALUE,
//            NULL,
//            PAGE_READWRITE,
//            0,
//            static_cast<DWORD>(bytes.size()),
//            _nameMemoryData.c_str());
//
//        if (hMapFile == NULL) {
//            throw std::runtime_error("Не удалось создать FileMapping для записи");
//        }
//
//        LPVOID pBuf = MapViewOfFile(
//            hMapFile,
//            FILE_MAP_ALL_ACCESS,
//            0,
//            0,
//            bytes.size());
//
//        if (pBuf == NULL) {
//            CloseHandle(hMapFile);
//            throw std::runtime_error("Не удалось отобразить вид файла для записи");
//        }
//
//        memcpy(pBuf, bytes.data(), bytes.size());
//
//        UnmapViewOfFile(pBuf);
//        CloseHandle(hMapFile);
//    }
//
//private:
//  void WaitDataControlEvent() {
//    while (!_shouldStop) {
//      if (WaitForSingleObject(_hEvent, 1000) == WAIT_OBJECT_0) {
//        // 1. Чтение заголовка
//        std::string header(256, '\0');
//        {
//          LPVOID pBuf = MapViewOfFile(_hMapFileControl, FILE_MAP_READ, 0, 0, 256);
//          memcpy(header.data(), pBuf, 256);
//          UnmapViewOfFile(pBuf);
//        }
//
//        auto [size, type, checksum] = ParseHeader(header);
//
//        // 2. Проверка контрольной суммы
//        uint32_t expected = std::hash<std::string>{}(type + std::to_string(size));
//        if (checksum != expected) {
//          std::cerr << "Checksum mismatch!" << std::endl;
//          continue;
//        }
//
//        // 3. Чтение данных
//        auto data = this->ReadMemoryData(size);
//
//        // 4. Десериализация
//        try {
//          msgpack::object_handle oh = msgpack::unpack(
//            reinterpret_cast<const char*>(data.data()),
//            data.size());
//
//          if (_callback) {
//            _callback(oh.get().as<std::string>());
//          }
//        }
//        catch (...) {
//          std::cerr << "Deserialization failed!" << std::endl;
//        }
//      }
//    }
//  }
//
//  std::string CreateHeader(size_t size, const std::string& type) {
//    std::stringstream ss;
//    ss << "size=" << size << ";\n"
//      << "type=" << type << ";\n"
//      << "checksum=" << std::hash<std::string>{}(type + std::to_string(size)) << ";\n";
//    return ss.str();
//  }
//
//  std::tuple<size_t, std::string, uint32_t> ParseHeader(const std::string& header) {
//    std::istringstream iss(header);
//    std::string line;
//    size_t size = 0;
//    std::string type;
//    uint32_t checksum = 0;
//
//    while (std::getline(iss, line)) {
//      if (line.find("size=") != std::string::npos) {
//        size = std::stoul(line.substr(5));
//      }
//      else if (line.find("type=") != std::string::npos) {
//        type = line.substr(5, line.size() - 6);
//      }
//      else if (line.find("checksum=") != std::string::npos) {
//        checksum = std::stoul(line.substr(9));
//      }
//    }
//    return { size, type, checksum };
//  }
//
//  YAML::Node ReadControlHeader() {
//      std::vector<BYTE> headerData(_sizeDataControl);
//
//      LPVOID pBuf = MapViewOfFile(_hMapFileControl, FILE_MAP_READ, 0, 0, _sizeDataControl);
//      if (pBuf) {
//        memcpy(headerData.data(), pBuf, _sizeDataControl);
//        UnmapViewOfFile(pBuf);
//      }
//
//      std::string headerStr(headerData.begin(), headerData.end());
//      return YAML::Load(headerStr);
//    }
//
//
//    const std::wstring _nameMemoryData;
//    const std::wstring _nameMemoryDataControl;
//    const std::wstring _eventNameMemoryDataControl;
//    HANDLE _hEvent = NULL;
//    HANDLE _hMapFileControl = NULL;
//    const DWORD _sizeDataControl = 1024;
//    std::function<void(const std::string&)> _callback;
//    std::thread _eventThread;
//    std::atomic<bool> _shouldStop;
//    std::mutex _mutex;
//};

