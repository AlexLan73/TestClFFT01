#pragma once
#include <string>

namespace YAML_soc
{
    struct IMSocket01
    {
        std::string Text;
        int Number;
    };
    struct MSocket01:public IMSocket01
    {

        // Метод сериализации для yaml-cpp
        template<typename Archive>
        void serialize(Archive& ar) {
            ar["Text"] = Text;
            ar["Number"] = Number;
        }

        // Сериализация объекта в YAML-строку
        std::string serializeToYaml(const IMSocket01& obj) {
            YAML::Node node;
            node["Text"] = obj.Text;
            node["Number"] = obj.Number;
            YAML::Emitter out;
            out << node;
            return std::string(out.c_str());
        }

        // Десериализация из YAML-строки в объект
        IMSocket01 deserializeFromYaml(const std::string& yamlStr) {
            YAML::Node node = YAML::Load(yamlStr);
            MSocket01 obj;
            if (node["Text"])
                obj.Text = node["Text"].as<std::string>();
            if (node["Number"])
                obj.Number = node["Number"].as<int>();
            return obj;
        }

    };

}

/*
// 
// #include <yaml-cpp/yaml.h>
//#include <iostream>
//#include <string>
//#include "src/MSocket01.h"
//
//
//int main()
//{
//    YAML_soc::IMSocket01 obj; 
//    obj.Text = "Hello YAML";
//    obj.Number = 42;
//
//    YAML_soc::MSocket01 m_socket01 = YAML_soc::MSocket01();
//    // Сериализация
//    std::string yamlStr = m_socket01.serializeToYaml(obj);
//    std::cout << "Serialized YAML:\n" << yamlStr << "\n\n";
//
//    // Десериализация
//    YAML_soc::IMSocket01 newObj = m_socket01.deserializeFromYaml(yamlStr);
//    std::cout << "Deserialized object:\n";
//    std::cout << "Text: " << newObj.Text << "\n";
//    std::cout << "Number: " << newObj.Number << "\n";
//
//    return 0;
//}

 
 */