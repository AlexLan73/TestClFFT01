#pragma once
// memory_nome.h
#pragma once
#include "memory_base.h"
#include <string>
#include <memory>
#include <functional>

// Определяем роли, как в C#
enum class ServerClient {
  Server,
  Client
};

class MemoryNome {
public:
  MemoryNome(const std::string& nameMemory, ServerClient role, std::function<void(RecDataMetaData)> callback);
  ~MemoryNome() = default;

  // Запрещаем копирование для безопасности
  MemoryNome(const MemoryNome&) = delete;
  MemoryNome& operator=(const MemoryNome&) = delete;

  // Основной метод для отправки данных
  void WriteDataToMemory(const std::vector<uint8_t>& bytes, const MetadataMap& map);

private:
  std::unique_ptr<MemoryBase> _memoryRead;
  std::unique_ptr<MemoryBase> _memoryWrite;
};
