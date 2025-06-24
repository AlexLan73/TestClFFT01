// cuda_memory.cpp
#include "cuda_memory.h"
#include <iostream>
#include <numeric>   // для std::accumulate
#include <sstream>   // для std::stringstream

// Вспомогательная функция для расчета контрольной суммы
long calculate_sum(const std::vector<uint8_t>& bytes) {
  return std::accumulate(bytes.begin(), bytes.end(), 0L);
}

CudaMemory::CudaMemory(const std::string& name, ServerClient role, TemperatureCallback tempCallback)
  : _userCallback(tempCallback) {
  // Создаем MemoryNome и передаем ему наш внутренний метод-обработчик
  _memoryNome = std::make_unique<MemoryNome>(name, role,
    [this](RecDataMetaData meta) { this->OnRawDataReceived(meta); });
}

void CudaMemory::OnRawDataReceived(RecDataMetaData dMetaData) {
  if (dMetaData.Bytes.empty() || !dMetaData.MetaData.count("type") ||
    dMetaData.MetaData.at("type") != "cudatemperature[]") {
    std::cerr << "[CudaMemory] Ошибка: получены некорректные или пустые данные." << std::endl;
    return;
  }

  try {
    // Десериализация из массива байт
    msgpack::object_handle oh = msgpack::unpack(reinterpret_cast<const char*>(dMetaData.Bytes.data()), dMetaData.Bytes.size());

    // Преобразование в вектор структур CudaTemperature
    std::vector<CudaTemperature> temperatures = oh.get().as<std::vector<CudaTemperature>>();

    // Вызов внешнего callback с готовыми данными
    if (_userCallback) {
      _userCallback(temperatures);
    }

  }
  catch (const std::exception& e) {
    std::cerr << "[CudaMemory] Критическая ошибка десериализации: " << e.what() << '\n';
  }
}

// Метод для отправки данных (клиент -> сервер)
void CudaMemory::SendData(const std::vector<CudaTemperature>& data) {
  std::stringstream buffer;
  msgpack::pack(buffer, data);
  const std::string& packed_str = buffer.str();
  std::vector<uint8_t> bytes(packed_str.begin(), packed_str.end());

  MetadataMap metadata;
  metadata["type"] = "cudatemperature[]"; // Тип данных, как в C#
  metadata["size"] = std::to_string(bytes.size());
  metadata["control_sum"] = std::to_string(calculate_sum(bytes));

  std::cout << "[CudaMemory] Отправка данных (" << bytes.size() << " байт)..." << '\n';
  _memoryNome->WriteDataToMemory(bytes, metadata);
}

// Метод для отправки ответа (сервер -> клиент)
void CudaMemory::SendProcessedData(std::vector<CudaTemperature> data) {
  // Главная логика сервера: умножение температуры на 10
  for (auto& item : data) {
    item.Temp *= 10.0f;
  }

  // Сериализуем и отправляем измененные данные
  SendData(data);
}
