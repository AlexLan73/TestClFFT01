//// cuda_node.h
//#pragma once
//
//#include "duplex_memory.h"
//#include "CudaTemperature.h"
//#include <iostream>
//#include <vector>
//#include <numeric> // Для std::accumulate
//#include <format>
//#include <chrono>
//
//class CudaNode {
//public:
//  CudaNode(const std::string& name, Role role);
//
//  // Метод, который будет вызван при получении данных
//  void callbackCommandDatAction(const std::vector<char>& data, const std::map<std::string, std::string>& metadata);
//
//  // Метод для отправки тестового пакета данных
//  void testDataMemory();
//
//private:
//  std::unique_ptr<DuplexMemory> _memory;
//  Role _role;
//
//  // Поля для хранения полученных данных
//  CudaTemperature _temperature;
//  std::vector<CudaTemperature> _temperatureArr;
//
//  // Вспомогательные функции
//  std::string get_current_time_str();
//};
