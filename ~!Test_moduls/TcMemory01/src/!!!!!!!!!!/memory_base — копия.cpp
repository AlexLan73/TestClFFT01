// memory_base.cpp
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
