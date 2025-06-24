// TcMemory01.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

// main.cpp
#include "src/memory_base.h"
#include "src/CudaTemperature.h"
#include <iostream>
#include <vector>
#include <map>
#include <numeric>   // Для std::accumulate (расчет контрольной суммы)
#include <chrono>    // Для получения текущего времени
#include <format>    // Для форматирования времени (требует C++20)
#include "src/cuda_node.h"
#include <thread>

#include "src/memory_base.h"

// --- Вспомогательная функция для получения времени в формате HH:mm:ss.fff ---
std::string get_current_time_str() {
  const auto now = std::chrono::system_clock::now();
  // В C++20 и выше можно использовать std::format
  return std::format("{:%T}", now);
}

//RecDataMetaData{
//  std::vector<char> Bytes;
//  MetadataMap MetaData;
//};

// --- Логика ЧИТАТЕЛЯ ---
// Эта функция будет вызвана, когда придут новые данные
//void handle_data_received(const std::vector<char>& data, const std::map<std::string, std::string>& metadata) {
void handle_data_received(const RecDataMetaData& dm) {
    std::cout << "\n--- [ЧИТАТЕЛЬ] Получены новые данные! ---\n";
    auto metadata = dm.MetaData;
    auto data = dm.Bytes;
  // --- НАДЕЖНАЯ ПРОВЕРКА МЕТАДАННЫХ ---
  auto type_it = metadata.find("type");
  if (type_it == metadata.end() || type_it->second != "cudatemperature[]") {
    std::cerr << "[ЧИТАТЕЛЬ] Ошибка: тип не указан или не соответствует 'cudatemperature[]'\n";
    return;
  }

  auto size_it = metadata.find("size");
  if (size_it == metadata.end()) {
    std::cerr << "[ЧИТАТЕЛЬ] Ошибка: в метаданных отсутствует ключ 'size'\n";
    return;
  }

  size_t expected_size;
  try {
    expected_size = std::stoul(size_it->second);
  }
  catch (const std::exception& e) {
    std::cerr << "[ЧИТАТЕЛЬ] Ошибка конвертации размера: " << e.what() << "\n";
    return;
  }

  if (data.size() < expected_size) {
    std::cerr << "[ЧИТАТЕЛЬ] Ошибка: фактический размер данных (" << data.size()
      << ") меньше ожидаемого (" << expected_size << ")\n";
    return;
  }

  // --- НАДЕЖНАЯ ПРОВЕРКА КОНТРОЛЬНОЙ СУММЫ ---
  auto sum_it = metadata.find("control_sum");
  if (sum_it != metadata.end()) {
    long received_sum = std::accumulate(data.begin(), data.begin() + expected_size, 0L);
    if (std::to_string(received_sum) == sum_it->second) {
      std::cout << "Контрольная сумма совпадает!\n";
    }
    else {
      std::cout << "ВНИМАНИЕ: Контрольная сумма не совпадает!\n";
    }
  }

  // --- НАДЕЖНАЯ ДЕСЕРИАЛИЗАЦИЯ ---
  try {
    msgpack::object_handle oh = msgpack::unpack(data.data(), expected_size);
    msgpack::object deserialized = oh.get();

    std::vector<CudaTemperature> temperatures = deserialized.as<std::vector<CudaTemperature>>();

    std::cout << "\nУспешно десериализовано " << temperatures.size() << " записей:\n";
    for (const auto& temp : temperatures) {
      std::cout << "  Время: " << temp.time << ", Температура: " << temp.temperature << " C\n";
    }
  }
  catch (const std::exception& e) {
    std::cerr << "[ЧИТАТЕЛЬ] КРИТИЧЕСКАЯ ОШИБКА ДЕСЕРИАЛИЗАЦИИ: " << e.what() << std::endl;
  }
  std::cout << "--- [ЧИТАТЕЛЬ] Обработка завершена ---\n";
}



void SendTestData(MemoryBase& writer) {
  std::cout << "\n--- Отправка тестовых данных... ---\n";

  // 1. Создаем данные
  std::vector<CudaTemperature> ls = {
      {get_current_time_str(), 43.0f},
      {get_current_time_str(), 41.0f},
      {get_current_time_str(), 42.0f},
      {get_current_time_str(), 44.0f},
      {get_current_time_str(), 33.0f}
  };

  // 2. Сериализуем данные в байты (MessagePack)
  std::stringstream buffer;
  msgpack::pack(buffer, ls);
  const std::string& packed_str = buffer.str();
  std::vector<char> bytesTemp(packed_str.begin(), packed_str.end());

  // 3. Формируем метаданные
  const std::string nameTypeRecord = "cudatemperature[]"; // Согласованное имя типа
  //long sumByte = std::accumulate(bytesTemp.begin(), bytesTemp.end(), 0L);

  // Мы используем лямбда-функцию, чтобы каждый char трактовался как unsigned char
  long sumByte = std::accumulate(bytesTemp.begin(), bytesTemp.end(), 0L,
    [](long sum, char val) {
      return sum + static_cast<unsigned char>(val);
    });

  MetadataMap metadata;
  metadata["type"] = nameTypeRecord;
  metadata["size"] = std::to_string(bytesTemp.size());
  metadata["control_sum"] = std::to_string(sumByte);

  // 4. ОДНА КОМАНДА ДЛЯ ЗАПИСИ ДАННЫХ И МЕТАДАННЫХ
  try {
    writer.WriteData(bytesTemp, metadata);
    std::cout << "Данные и метаданные успешно записаны.\n";
  }
  catch (const std::exception& e) {
    std::cerr << "Ошибка записи: " << e.what() << std::endl;
  }
}



int main(int argc, char* argv[]) {
  const int size_buff_ = 64 * 1024;
  const std::string memoryName = "CudaWrite";
  MemoryBase writer(memoryName, TypeBlockMemory::Write, size_buff_, handle_data_received);
  SendTestData(writer);
  std::this_thread::sleep_for(std::chrono::seconds(2));
//  writer.ClearCommandControl(); // Демонстрация очистки
  std::cout << "\nОбмен данными завершен. Нажмите Enter для выхода...\n";
  std::cin.get();


    return 0;
}


////if (argc < 2) {
////  std::cerr << "Использование: " << argv[0] << " <reader|writer>" << std::endl;
////  return 1;
////}
//
//const std::string memoryName = "CudaWrite";
////  cleanup_ipc_objects(memoryName);
//
////  std::string mode = argv[1];
//
////  if (mode == "writer") {
//    // --- Логика ПИСАТЕЛЯ ---
//std::cout << "--- РЕЖИМ ПИСАТЕЛЯ ---\n";
//MemoryBase writer(memoryName, TypeBlockMemory::Write, handle_data_received); // 1МБ для данных
//
////// --- Логика ЧИТАТЕЛЯ ---
////std::cout << "--- РЕЖИМ ЧИТАТЕЛЯ ---\n";
////MemoryBase reader(memoryName, TypeBlockMemory::Read, 64 * 1024, handle_data_received);
//
////std::cout << "Читатель запущен. Ожидание данных...\n";
//
//
//// 1. Создаем данные, как в C#
//std::vector<CudaTemperature> ls = {
//    {get_current_time_str(), 43.0f},
//    {get_current_time_str(), 41.0f},
//    {get_current_time_str(), 42.0f},
//    {get_current_time_str(), 44.0f},
//    {get_current_time_str(), 33.0f}
//};
//
//std::string _x_name1 = typeid(ls).name();
////auto xxx = typeid(ls).;
//
//// 2. Сериализуем данные в байты
//std::stringstream buffer;
//msgpack::pack(buffer, ls);
//const std::string& packed_str = buffer.str();
//std::vector<char> bytesTemp(packed_str.begin(), packed_str.end());
//
//std::cout << "Данные сериализованы в " << bytesTemp.size() << " байт.\n";
//
//// 3. Формируем метаданные
//// В C++ нет простого способа получить имя типа `CudaTemperature[]`
//// поэтому мы используем согласованную строку.
//const std::string nameTypeRecord = "cudatemperature[]";
//long sumByte = std::accumulate(bytesTemp.begin(), bytesTemp.end(), 0L);
//
//std::map<std::string, std::string> metadata;
//metadata["type"] = nameTypeRecord;
//metadata["size"] = std::to_string(bytesTemp.size());
//metadata["control_sum"] = std::to_string(sumByte);
//
//std::cout << "Метаданные сформированы.\n";
//
//writer.SetCommandControl(metadata);
//std::cout << "\nОбмен данными завершен. Нажмите Enter для выхода...\n";
//std::cin.get();





//    // 4. ПИШЕМ В ПАМЯТЬ ДАННЫЕ И СЛУЖЕБНУЮ ИНФОРМАЦИЮ (атомарно)
//    writer.WriteData(bytesTemp, metadata);
//
//    std::cout << "Данные и метаданные успешно записаны в разделяемую память.\n";
//    std::cout << "Писатель завершает работу.\n";
//
//  std::cout << "(Нажмите Enter для выхода)\n";
//    std::cin.get();
//    std::cout << "Читатель завершает работу.\n";
//
//    writer.destroy();
////    reader.destroy();
//
//
//    const std::string memoryNameNew = "CUDA_001";
//
//    // Полная очистка перед стартом для надежности
//    cleanup_ipc_objects111(memoryNameNew);
//
//    std::cout << "--- Создание Сервера и Клиента ---\n";
//    CudaNode server(memoryNameNew, Role::Server);
//    CudaNode client(memoryNameNew, Role::Client);
//    std::cout << "--- Объекты созданы, каналы настроены ---\n";
//
//    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Дадим потокам запуститься
//
//    // Клиент отправляет данные Серверу
//    client.testDataMemory();
//
//    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Пауза для наглядности
//
//    // Сервер отправляет данные Клиенту
//    server.testDataMemory();

//    std::cout << "\nОбмен данными завершен. Нажмите Enter для выхода...\n";
//    std::cin.get();
//
//  return 0;
//}



//void cleanup_ipc_objects(const std::string& baseName) {
//  std::cout << "Очистка IPC объектов для '" << baseName << "'..." << std::endl;
//  boost::interprocess::named_mutex::remove((baseName + "Mutex").c_str());
//  boost::interprocess::named_condition::remove((baseName + "Condition").c_str());
//  boost::interprocess::shared_memory_object::remove((baseName + "Control").c_str());
//  boost::interprocess::shared_memory_object::remove(baseName.c_str());
//  std::cout << "Очистка завершена." << std::endl;
//}
//
//// Функция очистки из предыдущих шагов
//void cleanup_ipc_objects111(const std::string& baseName) {
//  MemoryBase::Destroy(baseName + "Read");
//  MemoryBase::Destroy(baseName + "Write");
//}
//


//#include <iostream>
//#include <thread>
//#include <map>
//#include <string>
//
//#include <yaml-cpp/yaml.h>
//#include <fstream>
//
////#include "MemoryBase.h"
//#include "src/TestYaml.h"
//
//typedef std::map<std::string, std::string> StringMap;
//
//// Специализация YAML::convert для сериализации/десериализации IpAddressOne
//
//
//void serialize_string_map(const StringMap& map, const std::string& filename) {
//  YAML::Node node;
//  for (const auto& pair : map) {
//    node[pair.first] = pair.second;
//  }
//  std::ofstream fout(filename);
//  fout << node;
//}
//
//StringMap deserialize_string_map(const std::string& filename) {
//  StringMap map;
//  YAML::Node node = YAML::LoadFile(filename);
//  for (YAML::const_iterator it = node.begin(); it != node.end(); ++it) {
//    map[it->first.as<std::string>()] = it->second.as<std::string>();
//  }
//  return map;
//}
//
//
//int main() {
//
//
//
//
//
//
//
//
////  start_test();
//  StringMap myMap;
//  myMap["apple"] = "red";
//  myMap["banana"] = "yellow";
//  myMap["grape"] = "purple";
//  myMap["size1"] = std::to_string(sizeof(int));
//  myMap["size2"] = std::to_string(sizeof(myMap));
//  myMap["size3"] = std::to_string(sizeof("red"));
//
//
//  serialize_string_map(myMap, "example.yaml");
//
//  StringMap loadedMap = deserialize_string_map("example.yaml");
//
//  for (const auto& pair : loadedMap) {
//    std::cout << pair.first << ": " << pair.second << std::endl;
//  }
//
//
//
//  return 0;
//}
//
//
//



//int main() {
/*
    // Запуск двух потоков
    std::thread thread1([]() {
        // Код для первого потока
        std::cout << "Thread 1 started\n";
        try {
            std::cout << "=== ПРОЦЕСС 1 (Инициатор) ===" << std::endl;

            MemoryBase memory(L"MyCUDA", MemoryBase::TypeBlockMemory::Write);
            memory.SetCommandControl("Memory test 01");
            std::cout << "Отправлено: Memory test 01" << std::endl;

//            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
        catch (const std::exception& ex) {
            std::cerr << "Ошибка: " << ex.what() << std::endl;
        }
        std::cout << "Thread 1 finished\n";
        });

    std::thread thread2([]() {
        // Код для второго потока
        std::cout << "Thread 2 started\n";
        try {
            std::cout << "=== ПРОЦЕСС 2 (Обработчик) ===" << std::endl;

            MemoryBase memory(L"MyCUDA", MemoryBase::TypeBlockMemory::Read, ProcessMessage);
            std::cout << "Ожидаем сообщения..." << std::endl;

            //while (true) {
            //    std::this_thread::sleep_for(std::chrono::seconds(1));
            //}
        }
        catch (const std::exception& ex) {
            std::cerr << "Ошибка: " << ex.what() << std::endl;
        }
        std::cout << "Thread 2 finished\n";
        });

    // Ожидание завершения потоков
    thread1.join();  // Аналог _task1.Wait()
    thread2.join();  // Аналог _task0.Wait()

    std::cout << "All threads completed!\n";
*/
/*
  MemoryBase writer(L"TEST", MemoryBase::TypeBlockMemory::Write);
  writer.WriteObject(42);

  MemoryBase readd(L"TEST", MemoryBase::TypeBlockMemory::Read);
  auto x = readd.ReadObject<int>();

*/

//    return 0;
//}


/*

 MemoryBase writer(L"MyCUDA", MemoryBase::TypeBlockMemory::Write);

// Простой тип
writer.WriteObject<int>(42);

// Сложная структура
std::map<std::string, std::vector<float>> data = {
    {"test", {1.1f, 2.2f}},
    {"sample", {3.3f, 4.4f}}
};
writer.WriteObject(data);

MemoryBase reader(L"MyCUDA", MemoryBase::TypeBlockMemory::Read,
    [](const std::string& serializedData) {
        try {
            auto oh = msgpack::unpack(serializedData.data(), serializedData.size());
            std::cout << "Received data: " << oh.get() << std::endl;
        }
        catch (...) {
            std::cerr << "Deserialization error" << std::endl;
        }
    });
 
 */