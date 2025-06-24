#pragma once
// cuda_memory.h
#pragma once
#include "memory_nome.h"
#include "CudaTemperature.h"
#include <vector>
#include <functional>
#include <memory>

// Определяем тип для функции обратного вызова, которая будет принимать обработанные данные
using TemperatureCallback = std::function<void(const std::vector<CudaTemperature>&)>;

class CudaMemory {
public:
  CudaMemory(const std::string& name, ServerClient role, TemperatureCallback tempCallback);

  // Отправка данных (используется Клиентом)
  void SendData(const std::vector<CudaTemperature>& data);

  // Отправка обработанных данных (используется Сервером для ответа)
  void SendProcessedData(std::vector<CudaTemperature> data);

private:
  // Внутренний callback, который получает сырые данные от MemoryNome
  void OnRawDataReceived(RecDataMetaData dMetaData);

  std::unique_ptr<MemoryNome> _memoryNome;
  TemperatureCallback _userCallback; // Внешний callback для передачи десериализованных данных
};
