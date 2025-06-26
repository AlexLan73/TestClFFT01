// cuda_memory.cpp

#include "cuda_memory.h" // ВАЖНО: в C++ принято сначала включать заголовок самого класса
#include <iostream>    // Для std::cout
#include <stdexcept>   // Для std::runtime_error (если нужно бросать исключения)

// Конструктор класса CudaMemory
CudaMemory::CudaMemory(const std::string& channelName, ServerClient role) {
  // Создаем диспетчер MemoryProcessor из библиотеки MemoryData.
  // Передаем ему имя канала, роль и указатель на себя (this).
  // 'this' передается, потому что CudaMemory реализует интерфейс IMemoryDataHandler,
  // и MemoryProcessor будет вызывать наши переопределенные методы (OnAckReceived, OnCudaVectorData и т.д.).
  _processor = std::make_unique<MemoryProcessor>(channelName, role, this);
}

// --- Реализация публичных методов для отправки данных ---

void CudaMemory::SendLoggerData(const Logger& logEntry) {
  if (!_processor) throw std::runtime_error("MemoryProcessor is not initialized.");
  std::cout << "[CudaMemory] Отправка данных Logger...\n";
  // Используем шаблонный метод SendData диспетчера, передавая тип данных и их вектор.
  _processor->SendData(eLogger, std::vector<Logger>{logEntry}); // Отправляем один объект как вектор
}

void CudaMemory::SendCudaVectors(const std::vector<CudaVector>& vectors) {
  if (!_processor) throw std::runtime_error("MemoryProcessor is not initialized.");
  std::cout << "[CudaMemory] Отправка данных CudaVector...\n";
  _processor->SendData(eCudaVector, vectors);
}

void CudaMemory::SendCudaValues(const std::vector<CudaValue>& values) {
  if (!_processor) throw std::runtime_error("MemoryProcessor is not initialized.");
  std::cout << "[CudaMemory] Отправка данных CudaValue...\n";
  _processor->SendData(eCudaValue, values);
}

// --- Реализация метода для проверки ACK ---

bool CudaMemory::WasAckReceived() const {
  return _ackReceived.load(); // Атомарное чтение состояния
}

// --- Реализация переопределенных методов из интерфейса IMemoryDataHandler ---
// Эти методы будут вызываться MemoryProcessor, когда придут соответствующие данные.

void CudaMemory::OnAckReceived() {
  std::cout << "\n[CudaMemory::OnAckReceived] >>> ПОДТВЕРЖДЕНИЕ (ACK) ПОЛУЧЕНО! <<<\n";
  _ackReceived = true; // Устанавливаем флаг, что ACK получен
}

void CudaMemory::OnLoggerData(const std::vector<Logger>& data) {
  std::cout << "\n[CudaMemory::OnLoggerData] >>> Получены данные Logger. Количество записей: " << data.size() << "\n";
  // Здесь должна быть ваша специфическая логика обработки логов для CudaMemory
  if (!data.empty()) {
    const auto& log = data[0];
    std::cout << "  -> Log Entry: ID=" << log.Id << ", Module='" << log.Module
      << "', Log='" << log.Log << "', Code=" << static_cast<int>(log.Code) << "\n";
  }
}

void CudaMemory::OnCudaVectorData(const std::vector<CudaVector>& data) {
  std::cout << "\n[CudaMemory::OnCudaVectorData] >>> Получены данные CudaVector. Количество записей: " << data.size() << "\n";
  // Здесь должна быть ваша специфическая логика обработки CudaVector-данных
  if (!data.empty()) {
    const auto& vec = data[0];
    std::cout << "  -> CudaVector ID: " << vec.Id << ", Values: [";
    for (size_t i = 0; i < vec.Values.size(); ++i) {
      std::cout << vec.Values[i] << (i == vec.Values.size() - 1 ? "" : ", ");
    }
    std::cout << "]\n";
  }
}

void CudaMemory::OnCudaValueData(const std::vector<CudaValue>& data) {
  std::cout << "\n[CudaMemory::OnCudaValueData] >>> Получены данные CudaValue. Количество записей: " << data.size() << "\n";
  // Здесь должна быть ваша специфическая логика обработки CudaValue-данных
  if (!data.empty()) {
    const auto& val = data[0];
    std::cout << "  -> CudaValue ID: " << val.Id << ", Value: " << val.Value << "\n";
  }
}

bool CudaMemory::IsWriteChannelReady() {
  if (!_processor) return false;
  // Канал готов, если в его контрольном блоке нет метаданных.
  return _processor->CheckWriteChannel().empty();
}