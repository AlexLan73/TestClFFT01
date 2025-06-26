// main.cpp
//#include "src/cuda_memory.h" // Ваш существующий класс

#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <numeric>
#ifdef unpack
#undef unpack
#endif

#include "src/cuda_memory.h"


int main() {
  std::cout << "--- C++ КЛИЕНТ ЗАПУЩЕН (Тест с квитированием) ---\n";
  const std::string channelName = "Cuda";

  try {
    CudaMemory client(channelName, ServerClient::Client);

    // --- ТЕСТ: Отправка двух пакетов данных с ожиданием ---

    // Пакет 1
    std::cout << "\n--- Отправка Пакета 1 ---\n";
    std::vector<CudaVector> vectors1 = { {eCudaVector, {1.1, 2.2}} };
    client.SendCudaVectors(vectors1);

    // Ждем, пока сервер прочитает и очистит контрольный блок
    std::cout << "[MAIN] Ожидание, пока сервер обработает Пакет 1...\n";
    for (int i = 0; i < 50; ++i) { // Ждем до 5 секунд
      if (client.IsWriteChannelReady()) {
        std::cout << "[MAIN] Сервер обработал Пакет 1. Канал готов.\n";
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (!client.IsWriteChannelReady()) {
      std::cout << "[MAIN] ОШИБКА: Сервер не обработал Пакет 1 за 5 секунд.\n";
      // Здесь можно включить "другой алгоритм", как ты и хотел.
    }
    else {
      // Пакет 2
      std::cout << "\n--- Отправка Пакета 2 ---\n";
      std::vector<CudaValue> values2 = { {eCudaValue, 99.9} };
      client.SendCudaValues(values2);
      std::cout << "[MAIN] Пакет 2 отправлен.\n";
    }

  }
  catch (const std::exception& e) {
    std::cerr << "Критическая ошибка в main: " << e.what() << std::endl;
  }

  std::cout << "\nНажмите Enter для выхода.\n";
  std::cin.get();
  return 0;
}
