// MyYaml.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <yaml-cpp/yaml.h>
#include <string>
#include <iostream>
#include <sstream>

// Пример произвольного класса
struct ip_address_one {
    int Id;
    std::string name;
    std::string ip_address;
    int port1;
    int port2;
};

// Специализация YAML::convert для сериализации/десериализации IpAddressOne
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






int main() {
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

    return 0;
}






///---****************************************************************** ****
//#include <yaml-cpp/yaml.h>
//#include <fstream>
//#include <iostream>
//#include <map>
//#include <string>
//
//struct IpAddressOne {
//    int Id;
//    std::string Name;
//    std::string IpAddress;
//    int Port1;
//    int Port2;
//};
//
//// Специализация YAML::convert для IpAddressOne
//namespace YAML {
//    template<>
//    struct convert<IpAddressOne> {
//        static Node encode(const IpAddressOne& rhs) {
//            Node node;
//            node["id"] = rhs.Id;
//            node["name"] = rhs.Name;
//            node["ipAddress"] = rhs.IpAddress;
//            node["port1"] = rhs.Port1;
//            node["port2"] = rhs.Port2;
//            return node;
//        }
//
//        static bool decode(const Node& node, IpAddressOne& rhs) {
//            if (!node.IsMap()) return false;
//            rhs.Id = node["id"].as<int>();
//            rhs.Name = node["name"].as<std::string>();
//            rhs.IpAddress = node["ipAddress"].as<std::string>();
//            rhs.Port1 = node["port1"].as<int>();
//            rhs.Port2 = node["port2"].as<int>();
//            return true;
//        }
//    };
//}
//
//class ReadWriteYaml {
//private:
//    std::string _pathFileName;
//
//public:
//    ReadWriteYaml(const std::string& pathFileName) : _pathFileName(pathFileName) {}
//
//    std::map<int, IpAddressOne> ReadYaml(const std::string& path = "") {
//        std::string fileToRead = path.empty() ? _pathFileName : path;
//
//        std::ifstream fin(fileToRead);
//        if (!fin.is_open()) {
//            throw std::runtime_error("Error: file not found " + fileToRead);
//        }
//
//        YAML::Node root = YAML::Load(fin);
//        std::map<int, IpAddressOne> loadedMap;
//
//        // Ожидаем, что YAML — это мапа, где ключ — int, значение — IpAddressOne
//        for (auto it = root.begin(); it != root.end(); ++it) {
//            int key = it->first.as<int>();
//            IpAddressOne value = it->second.as<IpAddressOne>();
//            loadedMap[key] = value;
//        }
//
//        return loadedMap;
//    }
//
//    void WriteYaml(const std::map<int, IpAddressOne>& data, const std::string& path = "") {
//        std::string fileToWrite = path.empty() ? _pathFileName : path;
//
//        std::ofstream fout(fileToWrite);
//        if (!fout.is_open()) {
//            throw std::runtime_error("Error: cannot open file for writing " + fileToWrite);
//        }
//
//        YAML::Node root;
//        for (const auto& [key, value] : data) {
//            root[key] = value;
//        }
//
//        fout << root;
//    }
//};
//
//int main() {
//    std::string _pathYaml = R"(E:\C#\OpenCLDeskTop\Core\DeskTop\ipAddresses.yaml)";
//
//    try {
//        ReadWriteYaml rw(_pathYaml);
//
//        // Чтение
//        auto data = rw.ReadYaml();
//
//        // Вывод прочитанных данных
//        for (const auto& [key, ip] : data) {
//            std::cout << "Key: " << key << ", Name: " << ip.Name << ", IP: " << ip.IpAddress
//                << ", Port1: " << ip.Port1 << ", Port2: " << ip.Port2 << "\n";
//        }
//
//        // Добавим новый элемент
//        IpAddressOne newIp{ 3, "NewDevice", "192.168.1.100", 8080, 8081 };
//        data[3] = newIp;
//
//        // Запись обратно в YAML
//        rw.WriteYaml(data, "new_ipAddresses.yaml");
//
//    }
//    catch (const std::exception& ex) {
//        std::cerr << "Exception: " << ex.what() << std::endl;
//    }
//
//    return 0;
//}
//
///---****************************************************************** ****



