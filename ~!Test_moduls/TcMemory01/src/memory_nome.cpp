// memory_nome.cpp

#include "memory_nome.h"
#include <iostream>
#include <cstdint> // Убедитесь, что этот заголовок подключен для uint8_t

// Размер сегмента памяти по умолчанию, аналогично C#
constexpr size_t default_data_size = 64 * 1024; // 64 KB

MemoryNome::MemoryNome(const std::string& nameMemory, ServerClient role, std::function<void(RecDataMetaData)> callback) {

  std::string readChannelName;
  std::string writeChannelName;

  if (role == ServerClient::Server) {
    // Сервер читает из канала "Read", пишет в канал "Write"
    readChannelName = nameMemory + "Read";
    writeChannelName = nameMemory + "Write";
    std::cout << "[Server] Канал чтения: " << readChannelName << ", Канал записи: " << writeChannelName << '\n';
  }
  else { // Client
    // Клиент делает наоборот: читает из "Write", пишет в "Read"
    readChannelName = nameMemory + "Write";
    writeChannelName = nameMemory + "Read";
    std::cout << "[Client] Канал чтения: " << readChannelName << ", Канал записи: " << writeChannelName << '\n';
  }

  // Канал чтения требует callback для обработки входящих сообщений
  _memoryRead = std::make_unique<MemoryBase>(readChannelName, TypeBlockMemory::Read, default_data_size, callback);

  // Канал записи не нуждается в callback
  _memoryWrite = std::make_unique<MemoryBase>(writeChannelName, TypeBlockMemory::Write, default_data_size, nullptr);
}

void MemoryNome::WriteDataToMemory(const std::vector<uint8_t>& bytes, const MetadataMap& map) {
  try {
    _memoryWrite->WriteData(bytes, map);
  }
  catch (const std::exception& e) {
    std::cerr << "Ошибка при записи данных в MemoryNome: " << e.what() << std::endl;
  }
}

