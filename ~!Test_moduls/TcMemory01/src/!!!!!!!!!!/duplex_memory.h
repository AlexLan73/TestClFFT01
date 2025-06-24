//#pragma once
//// duplex_memory.h
//#pragma once
//
//#include "memory_base.h"
//#include <string>
//#include <memory> // Для std::unique_ptr
//
//// Аналог вашего enum ServerClient
//enum class Role {
//  Server,
//  Client
//};
//
//class DuplexMemory {
//public:
//  // Конструктор, принимающий имя, роль и колбэк для входящих данных
//  DuplexMemory(const std::string& nameMemory, Role role,
//    const std::function<void(RecDataMetaData)>& callBack,
//    size_t dataSegmentSize = 65536);
//
//  // Запись данных (делегируется внутреннему _memoryWrite)
//  void WriteData(const std::vector<char>& data, const std::map<std::string, std::string>& metadata);
//
//  // Шаблонный хелпер для записи объектов
////  template <typename T>
////  void WriteObject(const T& obj, const std::map<std::string, std::string>& additional_metadata = {});
//
//private:
//  // Используем unique_ptr, т.к. MemoryBase не копируемый
//  std::unique_ptr<MemoryBase> _memoryRead;
//  std::unique_ptr<MemoryBase> _memoryWrite;
//};
//
////// Реализация шаблонного метода
////template <typename T>
////void DuplexMemory::WriteObject(const T& obj, const std::map<std::string, std::string>& additional_metadata) {
////  if (_memoryWrite) {
////    // Просто перенаправляем вызов в наш MemoryBase для записи
////    _memoryWrite->WriteObject(obj, additional_metadata);
////  }
////}
