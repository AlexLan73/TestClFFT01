//#include "cuda_node.h"
//// cuda_node.cpp
//
//CudaNode::CudaNode(const std::string& name, Role role) : _role(role) {
//  // Создаем DuplexMemory, передавая ему наш метод `callbackCommandDatAction` в качестве колбэка.
//  // Используем лямбда-функцию для привязки 'this'
//  auto callback = [this](const std::vector<char>& data, const std::map<std::string, std::string>& metadata) {
//    this->callbackCommandDatAction(data, metadata);
//    };
//  _memory = std::make_unique<DuplexMemory>(name, role, callback, 1024 * 64);
//}
//
//std::string CudaNode::get_current_time_str() {
//  return std::format("{:%T}", std::chrono::system_clock::now());
//}
//
//void CudaNode::callbackCommandDatAction(const std::vector<char>& data, const std::map<std::string, std::string>& metadata) {
//  const char* role_str = (_role == Role::Server) ? "[СЕРВЕР]" : "[КЛИЕНТ]";
//  std::cout << "\n--- " << role_str << " Получены данные! ---\n";
//
//  // Здесь вся логика валидации и десериализации, как в вашем C# коде
//  auto type_it = metadata.find("type");
//  if (type_it == metadata.end()) { /* обработка ошибки */ return; }
//
//  auto size_it = metadata.find("size");
//  if (size_it == metadata.end()) { /* обработка ошибки */ return; }
//
//  auto sum_it = metadata.find("control_sum");
//  if (sum_it == metadata.end()) { /* обработка ошибки */ return; }
//
//  // Проверка контрольной суммы
//  long received_sum = std::accumulate(data.begin(), data.end(), 0L);
//  if (std::to_string(received_sum) != sum_it->second) {
//    std::cerr << role_str << " ОШИБКА: Контрольная сумма не совпадает!\n";
//    return;
//  }
//  std::cout << role_str << " Контрольная сумма OK.\n";
//
//  // Десериализация в зависимости от типа
//  const std::string& typeName = type_it->second;
//  try {
//    if (typeName == "cudatemperature[]") {
//      msgpack::object_handle oh = msgpack::unpack(data.data(), data.size());
//      _temperatureArr = oh.get().as<std::vector<CudaTemperature>>();
//      std::cout << role_str << " Успешно десериализован массив из " << _temperatureArr.size() << " элементов.\n";
//    }
//    else if (typeName == "cudatemperature") {
//      msgpack::object_handle oh = msgpack::unpack(data.data(), data.size());
//      _temperature = oh.get().as<CudaTemperature>();
//      std::cout << role_str << " Успешно десериализован один элемент.\n";
//    }
//    else {
//      std::cerr << role_str << " Неизвестный тип данных: " << typeName << "\n";
//    }
//  }
//  catch (const std::exception& e) {
//    std::cerr << role_str << " Ошибка десериализации: " << e.what() << "\n";
//  }
//}
//
//void CudaNode::testDataMemory() {
//  const char* role_str = (_role == Role::Server) ? "[СЕРВЕР]" : "[КЛИЕНТ]";
//  std::cout << "\n--- " << role_str << " Отправка тестовых данных... ---\n";
//
//  std::vector<CudaTemperature> ls = {
//      {get_current_time_str(), 43.0f}, {get_current_time_str(), 41.0f},
//      {get_current_time_str(), 42.0f}, {get_current_time_str(), 44.0f},
//      {get_current_time_str(), 33.0f}
//  };
//
////  _memory->WriteObject(ls, { {"author", (_role == Role::Server ? "server" : "client")} });
////  std::cout << role_str << " Данные отправлены.\n";
//}
