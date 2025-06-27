// memory_base.h
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <iostream>
#include <map> // Для хранения распарсенных метаданных

// Boost для межпроцессного взаимодействия (IPC)
#include <boost/interprocess/managed_windows_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/named_condition.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/exceptions.hpp> // Для обработки исключений

// MessagePack для сериализации
#include <msgpack.hpp>

// Тип блока памяти, как в C#
enum class TypeBlockMemory {
  Read,
  Write
};

class MemoryBase {
public:
  // 1. Конструктор теперь принимает размер для основного блока данных (_nameMemoryData)
  MemoryBase(const std::string& nameMemory,
    TypeBlockMemory typeBlockMemory,
    size_t dataSegmentSize = 65536, // Размер для _shmData, по умолчанию 64КБ
    // Новый, более мощный колбэк, передающий и данные, и метаданные
    std::function<void(const std::vector<char>& data, const std::map<std::string, std::string>& metadata)> callBack = nullptr);
  void cleanup_ipc_objects(const std::string& baseName);

  ~MemoryBase();

  // Запрещаем копирование и присваивание
  MemoryBase(const MemoryBase&) = delete;
  MemoryBase& operator=(const MemoryBase&) = delete;

  // Главный метод для записи данных и метаданных (для "писателя")
  void WriteData(const std::vector<char>& data, const std::map<std::string, std::string>& metadata);

  // Шаблонный метод для удобной записи объектов, который вызывает WriteData
  template <typename T>
  void WriteObject(const T& obj, const std::map<std::string, std::string>& additional_metadata = {});

  // Новый метод для полного удаления всех связанных IPC объектов
  static void Destroy(const std::string& nameMemory);
  void destroy() const;

private:
  // Внутренний метод для цикла ожидания событий в отдельном потоке
  void run_control_event_loop();
  // Вспомогательные функции для работы с контрольной строкой
  std::map<std::string, std::string> parse_control_string(const char* control_str);
  std::string format_control_string(const std::map<std::string, std::string>& metadata);

  // Имена
  std::string _nameMemoryData;
  std::string _nameMemoryDataControl;
  std::string _eventNameMutex;
  std::string _eventNameCondition;

  // Ресурсы Boost.Interprocess
  boost::interprocess::managed_windows_shared_memory _shmControl;
  boost::interprocess::managed_windows_shared_memory _shmData;
  boost::interprocess::named_mutex _eventMutex;
  boost::interprocess::named_condition _eventCondition;

  // Колбэк-функция
  std::function<void(const std::vector<char>&, const std::map<std::string, std::string>&)> _callBack;

  // Для управления фоновым потоком
  std::thread _eventLoopThread;
  std::atomic<bool> _running;

  // 3. Фиксированный размер для контрольного блока
  static  const size_t _sizeDataControl = 8* 1024;
  // Новое имя для объекта-флага
  const char* _init_flag_name = "InitializationFlag";
};

// Реализация шаблонного метода должна оставаться в заголовочном файле
template <typename T>
void MemoryBase::WriteObject(const T& obj, const std::map<std::string, std::string>& additional_metadata) {
  // Сериализуем объект
  std::stringstream buffer;
  msgpack::pack(buffer, obj);
  const std::string& str = buffer.str();
  std::vector<char> bytes(str.begin(), str.end());

  // Формируем метаданные, добавляя информацию о размере и типе
  std::map<std::string, std::string> metadata = additional_metadata;
  metadata["size"] = std::to_string(bytes.size());
  metadata["type"] = "msgpack";
  // Можно добавить и другие авто-поля, например, checksum
  // metadata["checksum"] = calculate_checksum(bytes);

  // Вызываем основной метод записи
  WriteData(bytes, metadata);
}
