#pragma once
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

