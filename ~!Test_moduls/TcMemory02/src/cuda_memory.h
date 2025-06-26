#pragma once

// --- ВАЖНО: Подключаем инструменты из нашей библиотеки MemoryData ---
// MemoryProcessor будет делать всю "грязную" работу по обмену данными.
#include "MemoryData/MemoryProcessor.h"
// IMemoryDataHandler - это "контракт", который мы обязуемся выполнять.
#include "MemoryData/IMemoryDataHandler.h"

#include <memory>
#include <atomic>
#include <vector>

// CudaMemory - это конкретный обработчик данных, реализующий наш интерфейс.
// Он знает только о данных, связанных с CUDA.
class CudaMemory : public IMemoryDataHandler {
public:
  // Конструктор очень простой: принимает имя канала и роль (клиент/сервер).
  CudaMemory(const std::string& name, ServerClient role);

  // Виртуальный деструктор обязателен.
  virtual ~CudaMemory() = default;

  // --- Публичный API для конечного приложения ---
  // Эти методы вызываются из main() для отправки конкретных типов данных.
  void SendVectors(const std::vector<CudaVector>& vectors);
  void SendValues(const std::vector<CudaValue>& values);
  void SendLog(const Logger& log);

  // Метод для проверки, пришел ли ответ от сервера.
  bool WasAckReceived() const;

private:
  // --- Переопределяем виртуальные методы из интерфейса IMemoryDataHandler ---
  // Мы реализуем только те методы, которые относятся к логике CudaMemory.
  // Остальные (например, OnLoggerData) будут использовать реализацию по умолчанию из интерфейса.

  void OnAckReceived() override;
  void OnCudaVectorData(const std::vector<CudaVector>& data) override;
  void OnCudaValueData(const std::vector<CudaValue>& data) override;

  // Указатель на диспетчер из библиотеки MemoryData.
  // Он будет вызывать наши переопределенные методы OnAckReceived, OnCudaVectorData и т.д.
  std::unique_ptr<MemoryProcessor> _processor;

  // Состояние, специфичное для нашего класса.
  std::atomic<bool> _ackReceived{ false };
};
