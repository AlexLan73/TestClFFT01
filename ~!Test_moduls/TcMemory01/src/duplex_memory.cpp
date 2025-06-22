// duplex_memory.cpp
#include "duplex_memory.h"

DuplexMemory::DuplexMemory(const std::string& nameMemory, Role role,
  const std::function<void(const std::vector<char>&, const std::map<std::string, std::string>&)>& callBack,
  size_t dataSegmentSize)
{
  std::string readChannelName;
  std::string writeChannelName;

  if (role == Role::Server) {
    // Для Сервера: читаем из "Read", пишем в "Write"
    readChannelName = nameMemory + "Read";
    writeChannelName = nameMemory + "Write";
    std::cout << "[Сервер] Канал чтения: " << readChannelName << ", Канал записи: " << writeChannelName << std::endl;
  }
  else { // Role::Client
    // Для Клиента: все наоборот. Читаем из "Write", пишем в "Read"
    readChannelName = nameMemory + "Write";
    writeChannelName = nameMemory + "Read";
    std::cout << "[Клиент] Канал чтения: " << readChannelName << ", Канал записи: " << writeChannelName << std::endl;
  }

  // Создаем экземпляр MemoryBase для чтения, передаем ему наш колбэк
  _memoryRead = std::make_unique<MemoryBase>(readChannelName, TypeBlockMemory::Read, dataSegmentSize, callBack);

  // Создаем экземпляр MemoryBase для записи. Колбэк ему не нужен.
  _memoryWrite = std::make_unique<MemoryBase>(writeChannelName, TypeBlockMemory::Write, dataSegmentSize, nullptr);
}

void DuplexMemory::WriteData(const std::vector<char>& data, const std::map<std::string, std::string>& metadata) {
  if (_memoryWrite) {
    _memoryWrite->WriteData(data, metadata);
  }
}
