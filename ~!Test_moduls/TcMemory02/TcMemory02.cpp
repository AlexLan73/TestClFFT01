// main.cpp
#include "src/cuda_memory.h" // Ваш существующий класс
//#include "/SharedDataTypes.h"


#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <numeric>
#ifdef unpack
#undef unpack
#endif
#include "MemoryData/memory_nome.h"
//#include "MemoryData/SharedDataTypes.h"

#include "src/cuda_memory.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
  std::cout << "--- C++ КЛИЕНТ ЗАПУЩЕН (Архитектура с интерфейсом) ---\n";

  try {
    // 1. Создаем наш высокоуровневый обработчик. Вся настройка скрыта внутри.
    CudaMemory client("Cuda", ServerClient::Client);

    // 2. Создаем и отправляем данные через простой и понятный метод.
    std::vector<CudaVector> vectors_to_send = {
        { eCudaVector, {1.1, 2.2, 3.3} },
        { eCudaVector, {4.4, 5.5, 6.6} }
    };
    client.SendVectors(vectors_to_send);

    // 3. Ожидаем ответа.
    int timeout_seconds = 5;
    for (int i = 0; i < timeout_seconds * 10; ++i) {
      if (client.WasAckReceived()) break;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    if (client.WasAckReceived()) {
      std::cout << "\n[MAIN] Тест успешно завершен!\n";
    }
    else {
      std::cout << "\n[MAIN] ОШИБКА: Подтверждение от сервера не получено.\n";
    }

  }
  catch (const std::exception& e) {
    std::cerr << "Критическая ошибка в main: " << e.what() << std::endl;
  }

  std::cout << "Нажмите Enter для выхода.\n";
  std::cin.get();
  return 0;
}
