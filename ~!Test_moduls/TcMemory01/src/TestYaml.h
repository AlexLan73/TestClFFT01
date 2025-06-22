#pragma once
#include <iostream>
#include <ostream>
#include <string>

void ProcessMessage(const std::string& message) {
  std::cout << "Получено сообщение: " << message << std::endl;

  MemoryBase memoryResponse(L"MyCUDA", MemoryBase::TypeBlockMemory::Write);
  std::string response = message + " == Return ALL OK!!! == ";
  memoryResponse.SetCommandControl(response);
  std::cout << "Отправлен ответ: " << response << std::endl;
}



// Пример произвольного класса
struct ip_address_one {
  int Id;
  std::string name;
  std::string ip_address;
  int port1;
  int port2;
};

namespace YAML {
  template<>
  struct convert<ip_address_one>
  {
    static Node encode(const ip_address_one& rhs) {
      Node node;
      node["id"] = rhs.Id;
      node["name"] = rhs.name;
      node["ipAddress"] = rhs.ip_address;
      node["port1"] = rhs.port1;
      node["port2"] = rhs.port2;
      return node;
    }

    static bool decode(const Node& node, ip_address_one& rhs) {
      if (!node.IsMap()) return false;
      rhs.Id = node["id"].as<int>();
      rhs.name = node["name"].as<std::string>();
      rhs.ip_address = node["ipAddress"].as<std::string>();
      rhs.port1 = node["port1"].as<int>();
      rhs.port2 = node["port2"].as<int>();
      return true;
    }
  };
}


template<typename T>
class YamlConverter
{
public:
  // Сериализация объекта в YAML-строку
  std::string serialize(const T& obj) {
    YAML::Node node = YAML::convert<T>::encode(obj);
    YAML::Emitter out;
    out << node;
    return std::string(out.c_str());
  }

  // Десериализация объекта из YAML-строки
  T deserialize(const std::string& yamlStr) {
    YAML::Node node = YAML::Load(yamlStr);
    return node.as<T>();
  }
};


void start_test()
{
  YamlConverter<ip_address_one> converter;

  ip_address_one obj{ 1, "Device1", "192.168.0.1", 8080, 8081 };

  // Сериализация в строку
  std::string yamlStr = converter.serialize(obj);
  std::cout << "Serialized YAML:\n" << yamlStr << "\n\n";

  // Десериализация из строки
  ip_address_one newObj = converter.deserialize(yamlStr);
  std::cout << "Deserialized object:\n";
  std::cout << "Id: " << newObj.Id << "\n";
  std::cout << "Name: " << newObj.name << "\n";
  std::cout << "IpAddress: " << newObj.ip_address << "\n";
  std::cout << "Port1: " << newObj.port1 << "\n";
  std::cout << "Port2: " << newObj.port2 << "\n";

}