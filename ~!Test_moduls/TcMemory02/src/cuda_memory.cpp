#include "cuda_memory.h"


#include <iostream>

CudaMemory::CudaMemory(const std::string& name, ServerClient role) {
  // Создаем диспетчер из библиотеки MemoryData и передаем ему указатель на себя (this).
  // Это связывает низкоуровневый транспорт с нашим высокоуровневым обработчиком.
  // Теперь, когда MemoryProcessor получит данные, он вызовет наши методы (OnAckReceived и т.д.).
  _processor = std::make_unique<MemoryProcessor>(name, role, this);
}

// --- Реализация публичного API ---

void CudaMemory::SendVectors(const std::vector<CudaVector>& vectors) {
  if (!_processor) return;
  std::cout << "[CudaMemory] Отправка данных типа CudaVector...\n";
  // Используем шаблонный метод из диспетчера для отправки.
  _processor->SendData(eCudaVector, vectors);
}

void CudaMemory::SendValues(const std::vector<CudaValue>& values) {
  if (!_processor) return;
  std::cout << "[CudaMemory] Отправка данных типа CudaValue...\n";
  _processor->SendData(eCudaValue, values);
}

void CudaMemory::SendLog(const Logger& log) {
  if (!_processor) return;
  std::cout << "[CudaMemory] Отправка лога...\n";
  // Отправляем как вектор из одного элемента.
  _processor->SendData(eLogger, std::vector<Logger>{log});
}

bool CudaMemory::WasAckReceived() const {
  return _ackReceived.load();
}

// --- Реализация переопределенных методов интерфейса IMemoryDataHandler ---

void CudaMemory::OnAckReceived() {
  std::cout << "\n[CudaMemory] >>> ПОДТВЕРЖДЕНИЕ (ACK) ПОЛУЧЕНО! <<<\n";
  _ackReceived = true;
}

void CudaMemory::OnCudaVectorData(const std::vector<CudaVector>& data) {
  std::cout << "\n[CudaMemory] >>> Получены данные CudaVector. Количество записей: " << data.size() << "\n";
  // Здесь ваша логика обработки полученных векторов...
  if (!data.empty()) {
    std::cout << "  -> Первый элемент первого вектора: " << data[0].Values[0] << "\n";
  }
}

void CudaMemory::OnCudaValueData(const std::vector<CudaValue>& data) {
  std::cout << "\n[CudaMemory] >>> Получены данные CudaValue. Количество записей: " << data.size() << "\n";
  // Здесь ваша логика обработки полученных значений...
  if (!data.empty()) {
    std::cout << "  -> Первое значение: " << data[0].Value << "\n";
  }
}
