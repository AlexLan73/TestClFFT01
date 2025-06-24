// memory_base.cpp


#include "memory_base.h"

#include <sstream>
#include <stdexcept>

// Вспомогательная функция для конвертации std::string (UTF-8) в std::wstring (UTF-16) для WinAPI
std::wstring to_wstring(const std::string& str) {
  if (str.empty()) return std::wstring();
  int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
  std::wstring wstr(size_needed, 0);
  MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], size_needed);
  return wstr;
}

// --- КОНСТРУКТОР ---
MemoryBase::MemoryBase(const std::string& nameMemory, TypeBlockMemory typeBlockMemory, size_t dataSegmentSize,
  std::function<void(RecDataMetaData)> callBack)
  : _nameMemoryData(nameMemory),
  _nameMemoryDataControl(nameMemory + "Control"),
  _dataSegmentSize(dataSegmentSize),
  _callBack(callBack) {

  std::wstring wNameControl = to_wstring(_nameMemoryDataControl);
  std::wstring wNameData = to_wstring(_nameMemoryData);
  std::wstring eventName = L"Global\\Event" + to_wstring(nameMemory);

  hControlMapFile = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, CONTROL_SIZE, wNameControl.c_str());
  if (hControlMapFile == NULL) throw std::runtime_error("Failed to create control file mapping.");

  hDataMapFile = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, dataSegmentSize, wNameData.c_str());
  if (hDataMapFile == NULL) {
    CloseHandle(hControlMapFile);
    throw std::runtime_error("Failed to create data file mapping.");
  }

  hEvent = CreateEventW(NULL, FALSE, FALSE, eventName.c_str());
  if (hEvent == NULL) {
    CloseHandle(hControlMapFile);
    CloseHandle(hDataMapFile);
    throw std::runtime_error("Failed to create event.");
  }

  if (typeBlockMemory == TypeBlockMemory::Read && _callBack) {
    _running = true;
    _eventThread = std::thread(&MemoryBase::event_loop, this);
  }
}

// --- ДЕСТРУКТОР ---
MemoryBase::~MemoryBase() {
  if (_running.load()) {
    _running = false;
    SetEvent(hEvent);
    if (_eventThread.joinable()) {
      _eventThread.join();
    }
  }
  if (hEvent) CloseHandle(hEvent);
  if (hControlMapFile) CloseHandle(hControlMapFile);
  if (hDataMapFile) CloseHandle(hDataMapFile);
}

// --- ПУБЛИЧНЫЕ МЕТОДЫ ---
void MemoryBase::WriteData(const std::vector<char>& data, const MetadataMap& metadata) {
  if (data.size() > _dataSegmentSize) {
    throw std::runtime_error("Размер данных превышает размер выделенного сегмента памяти.");
  }

  LPVOID pDataBuf = MapViewOfFile(hDataMapFile, FILE_MAP_ALL_ACCESS, 0, 0, _dataSegmentSize);
  if (pDataBuf == nullptr) {
    throw std::runtime_error("Не удалось получить View Of File для записи данных.");
  }

  ZeroMemory(pDataBuf, _dataSegmentSize);
  CopyMemory(pDataBuf, data.data(), data.size());
  UnmapViewOfFile(pDataBuf);

  SetCommandControl(metadata);
}

MetadataMap MemoryBase::GetCommandControl() {
  LPVOID pBuf = MapViewOfFile(hControlMapFile, FILE_MAP_READ, 0, 0, CONTROL_SIZE);
  if (pBuf == nullptr) return {};

  std::string controlStr(static_cast<char*>(pBuf));
  UnmapViewOfFile(pBuf);

  return parse_control_string(controlStr.c_str());
}

void MemoryBase::SetCommandControl(const MetadataMap& metadata) {
  std::string controlStr = format_control_string(metadata);

  LPVOID pBuf = MapViewOfFile(hControlMapFile, FILE_MAP_ALL_ACCESS, 0, 0, CONTROL_SIZE);
  if (pBuf == nullptr) {
    throw std::runtime_error("Не удалось получить View Of File для записи метаданных.");
  }

  ZeroMemory(pBuf, CONTROL_SIZE);
  CopyMemory(pBuf, controlStr.c_str(), controlStr.length());
  UnmapViewOfFile(pBuf);

  ::SetEvent(hEvent);
}

void MemoryBase::ClearCommandControl() {
  std::cout << "[Команда] Очистка контрольной памяти..." << std::endl;
  LPVOID pBuf = MapViewOfFile(hControlMapFile, FILE_MAP_ALL_ACCESS, 0, 0, CONTROL_SIZE);
  if (pBuf == nullptr) {
    throw std::runtime_error("Не удалось получить View Of File для очистки.");
  }
  ZeroMemory(pBuf, CONTROL_SIZE);
  UnmapViewOfFile(pBuf);
  std::cout << "[Команда] Память очищена. Подаю сигнал." << std::endl;
  ::SetEvent(hEvent);
}

// --- ПРИВАТНЫЕ МЕТОДЫ ---
void MemoryBase::event_loop() {
  while (_running.load()) {
    if (WaitForSingleObject(hEvent, 1000) == WAIT_OBJECT_0) {
      if (!_running.load()) break;

      MetadataMap metadata = GetCommandControl();
      if (metadata.empty()) {
        if (_callBack) _callBack({ {}, metadata });
        continue;
      }

      auto it = metadata.find("size");
      size_t dataSize = (it != metadata.end()) ? std::stoul(it->second) : 0;
      std::vector<char> data;

      if (dataSize > 0) {
        LPVOID pDataBuf = MapViewOfFile(hDataMapFile, FILE_MAP_READ, 0, 0, dataSize);
        if (pDataBuf != nullptr) {
          data.assign(static_cast<char*>(pDataBuf), static_cast<char*>(pDataBuf) + dataSize);
          UnmapViewOfFile(pDataBuf);
        }
      }
      if (_callBack) {
        _callBack({ data, metadata });
      }
    }
  }
}

// =========================================================================
// === Вспомогательные функции (теперь с полной реализацией) ===
// =========================================================================

MetadataMap MemoryBase::parse_control_string(const char* control_str) {
  MetadataMap metadata;
  if (control_str == nullptr) return metadata;

  std::string str(control_str);
  std::stringstream ss(str);
  std::string segment;

  while (std::getline(ss, segment, ';')) {
    if (segment.empty()) continue;
    std::string::size_type pos = segment.find('=');
    if (pos != std::string::npos) {
      metadata[segment.substr(0, pos)] = segment.substr(pos + 1);
    }
  }
  return metadata;
}

std::string MemoryBase::format_control_string(const MetadataMap& metadata) {
  std::stringstream ss;
  for (const auto& [key, val] : metadata) {
    ss << key << "=" << val << ";";
  }
  return ss.str();
}
























/*
#include <sstream>
#include <system_error>

// Конвертация std::string (UTF-8) в std::wstring (UTF-16) для WinAPI
std::wstring to_wstring(const std::string& str) {
  if (str.empty()) return std::wstring();
  int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
  std::wstring wstr(size_needed, 0);
  MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstr[0], size_needed);
  return wstr;
}

MemoryBase::MemoryBase(const std::string& nameMemory, TypeBlockMemory typeBlockMemory, 
            std::function<void(const std::vector<char>&, const std::map<std::string, std::string>&)> callBack)
  : _nameMemoryData(nameMemory),
  _nameMemoryDataControl(nameMemory + "Control"),
  _callBack(callBack) {
  // !!!!!! убрать
  const size_t size_data_ = 64 * 1024;
  // --- 1. Создание имен, полностью совместимых с C# ---
  std::wstring wNameControl = to_wstring(_nameMemoryDataControl);
  std::wstring wNameData = to_wstring(_nameMemoryData);

  // C# создает событие с префиксом "Event", добавим его
  std::wstring eventName = L"Global\\Event" + to_wstring(nameMemory);

  // --- 2. Создание объектов Win32 API ---
  hControlMapFile = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, CONTROL_SIZE, wNameControl.c_str());
  if (hControlMapFile == NULL) {
    throw std::runtime_error("Failed to create control file mapping.");
  }

  // Основной блок данных создается по требованию, но можно и сразу
  hDataMapFile = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 
    0, size_data_, wNameData.c_str());
  if (hDataMapFile == NULL) {
    CloseHandle(hControlMapFile);
    throw std::runtime_error("Failed to create data file mapping.");
  }

  hEvent = CreateEventW(NULL, FALSE, FALSE, eventName.c_str()); // FALSE = AutoReset, как в C#
  if (hEvent == NULL) {
    CloseHandle(hControlMapFile);
    CloseHandle(hDataMapFile);
    throw std::runtime_error("Failed to create event.");
  }

  std::cout << "[C++] Успешно создан или открыт Event: " << nameMemory << std::endl;

  if (typeBlockMemory == TypeBlockMemory::Read && _callBack) {
    _running = true;
    _eventThread = std::thread(&MemoryBase::event_loop, this);
  }
}

MemoryBase::~MemoryBase() {
  if (_running.load()) {
    _running = false;
    SetEvent(hEvent); // "Разбудить" поток, чтобы он завершился
    if (_eventThread.joinable()) {
      _eventThread.join();
    }
  }
  if (hEvent) CloseHandle(hEvent);
  if (hControlMapFile) CloseHandle(hControlMapFile);
  if (hDataMapFile) CloseHandle(hDataMapFile);
}

void MemoryBase::SetCommandControl(const std::map<std::string, std::string>& metadata) {
  std::string controlStr = format_control_string(metadata);

  LPVOID pBuf = MapViewOfFile(hControlMapFile, FILE_MAP_ALL_ACCESS, 0, 0, CONTROL_SIZE);
  if (pBuf == nullptr) return;

  ZeroMemory(pBuf, CONTROL_SIZE);
  CopyMemory(pBuf, controlStr.c_str(), controlStr.length());

  UnmapViewOfFile(pBuf);

  SetEvent(hEvent); // <--- Подача сигнала, который C# сможет "услышать"
}

void MemoryBase::event_loop() {
  while (_running.load()) {
    // Ждем сигнала от C# (или другого C++ процесса)
    DWORD waitResult = WaitForSingleObject(hEvent, 1000); // 1 сек таймаут для проверки _running

    if (waitResult == WAIT_OBJECT_0) {
      if (!_running.load()) break;

      // --- Чтение контрольного блока ---
      LPVOID pControlBuf = MapViewOfFile(hControlMapFile, FILE_MAP_READ, 0, 0, CONTROL_SIZE);
      if (pControlBuf == nullptr) continue;

      std::string controlStr(static_cast<char*>(pControlBuf));
      UnmapViewOfFile(pControlBuf);

      auto metadata = parse_control_string(controlStr.c_str());
      if (metadata.find("size") == metadata.end()) continue;

      size_t dataSize = std::stoul(metadata["size"]);
      if (dataSize == 0) {
        if (_callBack) _callBack({}, metadata); // Вызываем колбэк с пустыми данными
        continue;
      }

      // --- Чтение основного блока данных ---
      LPVOID pDataBuf = MapViewOfFile(hDataMapFile, FILE_MAP_READ, 0, 0, dataSize);
      if (pDataBuf == nullptr) continue;

      std::vector<char> data(dataSize);
      CopyMemory(data.data(), pDataBuf, dataSize);
      UnmapViewOfFile(pDataBuf);

      if (_callBack) {
        _callBack(data, metadata);
      }
    }
  }
}

// Ваши вспомогательные функции парсинга/форматирования (остаются без изменений)
std::map<std::string, std::string> MemoryBase::parse_control_string(const char* control_str) {
  std::map<std::string, std::string> metadata;
  std::string str(control_str);
  std::stringstream ss(str);
  std::string segment;
  while (std::getline(ss, segment, ';')) {
    if (segment.empty()) continue;
    std::string::size_type pos = segment.find('=');
    if (pos != std::string::npos) {
      metadata[segment.substr(0, pos)] = segment.substr(pos + 1);
    }
  }
  return metadata;
}

std::string MemoryBase::format_control_string(const std::map<std::string, std::string>& metadata) {
  std::stringstream ss;
  for (auto const& [key, val] : metadata) {
    ss << key << "=" << val << ";";
  }
  return ss.str();
}
*/


/*

#include "memory_base.h"
#include <sstream> // Для std::stringstream
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/named_condition.hpp>



namespace ipc = boost::interprocess;


// Псевдоним можно оставить для удобства в теле методов
namespace ipc = boost::interprocess;

MemoryBase::MemoryBase(const std::string& nameMemory, TypeBlockMemory typeBlockMemory,
  size_t dataSegmentSize,
  std::function<void(const std::vector<char>&, const std::map<std::string, std::string>&)> callBack)
  // Инициализация имен
  : _nameMemoryData(nameMemory),
  _nameMemoryDataControl(_nameMemoryData + "Control"),
  _eventNameMutex(_nameMemoryData + "Mutex"),
  _eventNameCondition(_nameMemoryData + "Condition"),

  // ИСПОЛЬЗУЕМ ПОЛНЫЕ ИМЕНА ЗДЕСЬ
//  _shmControl(boost::interprocess::open_or_create, _nameMemoryDataControl.c_str(), _sizeDataControl),
  // ВЫДЕЛЯЕМ ПАМЯТЬ С ЗАПАСОМ ДЛЯ СЛУЖЕБНЫХ НУЖД BOOST
  _shmControl(boost::interprocess::open_or_create,_nameMemoryDataControl.c_str(), _sizeDataControl *2),

  _eventMutex(boost::interprocess::open_or_create, _eventNameMutex.c_str()),
  _eventCondition(boost::interprocess::open_or_create, _eventNameCondition.c_str()),

  // Колбэк и флаги
  _callBack(callBack),
  _running(false)
{

  // Логика основного блока данных остается прежней
  try {
    _shmData = boost::interprocess::managed_windows_shared_memory(boost::interprocess::open_only, _nameMemoryData.c_str());
    std::cout << "Подключились к существующему сегменту данных: " << _nameMemoryData << std::endl;
  }
  catch (const boost::interprocess::interprocess_exception&) {
    std::cout << "Создаем новый сегмент данных: " << _nameMemoryData << " размером " << dataSegmentSize << " байт." << std::endl;
    _shmData = boost::interprocess::managed_windows_shared_memory(boost::interprocess::create_only, _nameMemoryData.c_str(), dataSegmentSize);
  }

    // Ищем или создаем флаг. find_or_construct вернет true, если флаг был создан только что
  auto* flag = _shmControl.find_or_construct<bool>(_init_flag_name)();

  if (typeBlockMemory == TypeBlockMemory::Write) {
    // Мы "писатель", мы отвечаем за инициализацию
    *flag = true; // Сообщаем всем, что память готова к использованию
    std::cout << "Память инициализирована. Флаг готовности взведен.\n";
  }
  else {
    // Мы "читатель", мы должны проверить флаг
    if (!(*flag)) {
      // Это можно улучшить, добавив цикл ожидания, но для начала просто выведем предупреждение
      std::cerr << "ВНИМАНИЕ: Подключение к неинициализированной памяти! Ожидайте проблем.\n";
    }
    else {
      std::cout << "Память уже инициализирована. Успешное подключение.\n";
    }
  }

  // Запуск потока-слушателя
  if (_callBack && typeBlockMemory == TypeBlockMemory::Read) {
    _running = true;
    _eventLoopThread = std::thread(&MemoryBase::run_control_event_loop, this);
    std::cout << "Режим чтения активен. Ожидание команд..." << std::endl;
  }
}



MemoryBase::~MemoryBase() {
  if (_running.load()) {
    _running = false;
    _eventCondition.notify_all(); // Разбудить поток, чтобы он мог проверить _running и завершиться
    if (_eventLoopThread.joinable()) {
      _eventLoopThread.join();
    }
  }
  std::cout << "Ресурсы MemoryBase освобождены." << std::endl;
}



// 4. Реализация вашего алгоритма для "читателя"
void MemoryBase::run_control_event_loop() {
  while (_running.load()) {
    { // Начало блока для scoped_lock
      ipc::scoped_lock<ipc::named_mutex> lock(_eventMutex);
      _eventCondition.wait(lock); // Ждем сигнала от "писателя"

      if (!_running.load()) break; // Выходим, если пришел сигнал о завершении

      // 4.1: Считываем память _nameMemoryDataControl
      auto res_ctrl = _shmControl.find<char>(_nameMemoryDataControl.c_str());
      if (!res_ctrl.first) {
        std::cerr << "Не удалось найти контрольный блок!" << std::endl;
        continue;
      }

      // 4.2: Парсим строку с метаданными
      std::map<std::string, std::string> metadata = parse_control_string(res_ctrl.first);
      if (metadata.find("size") == metadata.end()) {
        std::cerr << "В метаданных отсутствует ключ 'size'!" << std::endl;
        continue;
      }

      // 4.3: Получаем размер в байтах и читаем из памяти _nameMemoryData
      size_t dataSize = 0;
      try {
        dataSize = std::stoul(metadata["size"]);
      }
      catch (const std::exception& e) {
        std::cerr << "Ошибка конвертации размера: " << e.what() << std::endl;
        continue;
      }

      auto res_data = _shmData.find<std::vector<char>>(_nameMemoryData.c_str());

      if (res_data.first && res_data.first->size() >= dataSize) {
        // 4.4: Конвертируем/передаем данные через колбэк
        std::vector<char> received_data = *res_data.first;
        received_data.resize(dataSize); // Убираем лишние байты, если вектор был больше
        _callBack(received_data, metadata);
      }
      else {
        std::cerr << "Блок данных не найден или его размер недостаточен!" << std::endl;
      }
    } // Конец блока для scoped_lock, мьютекс освобождается
  }
}

// Метод для "писателя"
void MemoryBase::WriteData(const std::vector<char>& data, const std::map<std::string, std::string>& metadata) {
  // ВСТАВИТЬ ЭТОТ КОД
// Пытаемся захватить мьютекс в течение 5 секунд
  //boost::posix_time::ptime timeout = boost::posix_time::microsec_clock::universal_time() + boost::posix_time::seconds(5);

  //ipc::scoped_lock<ipc::named_mutex> lock(_eventMutex, timeout);

  //if (!lock.owns()) {
  //  // Если мьютекс не был захвачен за 5 секунд
  //  std::cerr << "КРИТИЧЕСКАЯ ОШИБКА: Не удалось захватить мьютекс. Возможно, другой процесс завис." << std::endl;
  //  return; // Не продолжаем, чтобы не повредить данные
  //}
  //////////////////////////////////////////////////////////////////

  ipc::scoped_lock<ipc::named_mutex> lock(_eventMutex);

  // Записываем основные данные в _shmData
  auto* vec = _shmData.find_or_construct<std::vector<char>>(_nameMemoryData.c_str())();
  *vec = data;

  // Формируем и записываем служебную информацию в _shmControl
  std::string control_str = format_control_string(metadata);
  char* mem = _shmControl.find_or_construct<char>(_nameMemoryDataControl.c_str())[_sizeDataControl]();
  strncpy_s(mem, _sizeDataControl, control_str.c_str(), _sizeDataControl - 1);
  mem[_sizeDataControl - 1] = '\0'; // Гарантируем нуль-терминатор на всякий случай

  // Уведомляем всех "читателей"
  _eventCondition.notify_all();
}

// Вспомогательная функция для парсинга: "key1=value1;key2=value2;"
std::map<std::string, std::string> MemoryBase::parse_control_string(const char* control_str) {
  std::map<std::string, std::string> metadata;
  std::string str(control_str);
  std::stringstream ss(str);
  std::string segment;

  while (std::getline(ss, segment, ';')) {
    if (segment.empty()) continue;
    std::string::size_type pos = segment.find('=');
    if (pos != std::string::npos) {
      metadata[segment.substr(0, pos)] = segment.substr(pos + 1);
    }
  }
  return metadata;
}

// Вспомогательная функция для форматирования
std::string MemoryBase::format_control_string(const std::map<std::string, std::string>& metadata) {
  std::stringstream ss;
  for (auto const& [key, val] : metadata) {
    ss << key << "=" << val << ";";
  }
  return ss.str();
}

void MemoryBase::Destroy(const std::string& nameMemory) {
  std::cout << "Принудительное удаление всех IPC объектов для '" << nameMemory << "'...\n";
  boost::interprocess::named_mutex::remove((nameMemory + "Mutex").c_str());
  boost::interprocess::named_condition::remove((nameMemory + "Condition").c_str());
  boost::interprocess::shared_memory_object::remove((nameMemory + "Control").c_str());
  boost::interprocess::shared_memory_object::remove(nameMemory.c_str());
  std::cout << "Удаление завершено.\n";
}

void MemoryBase::destroy() const
{
  Destroy(_nameMemoryData);
}
*/