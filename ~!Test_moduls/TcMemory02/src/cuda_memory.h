#pragma once
// cuda_memory.h

#pragma once

// --- Подключаем публичные заголовки библиотеки MemoryData ---
// MemoryProcessor будет делать всю "грязную" работу по обмену данными.
#include "MemoryData/MemoryProcessor.h"
// IMemoryDataHandler - это "контракт", который мы обязуемся выполнять.
#include "MemoryData/IMemoryDataHandler.h"

// --- Подключаем другие необходимые заголовки ---
#include <memory>  // Для std::unique_ptr
#include <atomic>  // Для std::atomic
#include <string>  // Для std::string
#include <vector>  // Для std::vector

// CudaMemory - это конкретный прикладной обработчик данных, реализующий интерфейс IMemoryDataHandler.
// Он будет знать только о данных, связанных с CUDA и логировании в этом контексте.
class CudaMemory : public IMemoryDataHandler {
public:
  // Конструктор: принимает имя канала (например, "CudaChannel") и роль (Клиент/Сервер).
  CudaMemory(const std::string& channelName, ServerClient role);

  // Виртуальный деструктор обязателен для классов, наследующих интерфейсы.
  virtual ~CudaMemory() = default;

  // --- Публичные методы для отправки данных (прикладной API) ---
  // Эти методы будут вызываться из твоего основного приложения (main()).
  void SendLoggerData(const Logger& logEntry);
  void SendCudaVectors(const std::vector<CudaVector>& vectors);
  void SendCudaValues(const std::vector<CudaValue>& values);
  // Добавь другие методы SendXxx для других типов данных по мере необходимости.

  // Метод для проверки, пришел ли ACK от сервера.
  bool WasAckReceived() const;
  // Возвращает true, если получатель прочитал и очистил предыдущее сообщение.
  bool IsWriteChannelReady();

private:
  // --- Переопределяем виртуальные методы из интерфейса IMemoryDataHandler ---
  // Мы реализуем только те методы, которые относятся к логике CudaMemory.
  // Остальные (например, OnCudaMatrixData) будут использовать реализацию по умолчанию из IMemoryDataHandler.

  void OnAckReceived() override;
  void OnLoggerData(const std::vector<Logger>& data) override;
  void OnCudaVectorData(const std::vector<CudaVector>& data) override;
  void OnCudaValueData(const std::vector<CudaValue>& data) override;

  // Указатель на диспетчер MemoryProcessor из библиотеки MemoryData.
  // Он будет вызывать наши переопределенные методы OnAckReceived, OnCudaVectorData и т.д.
  std::unique_ptr<MemoryProcessor> _processor;

  // Состояние, специфичное для нашего класса CudaMemory.
  std::atomic<bool> _ackReceived{ false };
};
