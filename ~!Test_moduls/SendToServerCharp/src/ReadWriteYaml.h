#pragma once
#include <fstream>
#include <map>
#include <yaml-cpp/yaml.h>
#include "IpAddressOne.h"

class ReadWriteYaml {
private:
    std::string _pathFileName;

public:
    explicit ReadWriteYaml(std::string pathFileName) : _pathFileName(std::move(pathFileName)) {}

    std::map<int, ip_address_one> ReadYaml(const std::string& path = "") {
        std::string fileToRead = path.empty() ? _pathFileName : path;

        std::ifstream fin(fileToRead);
        if (!fin.is_open()) {
            throw std::runtime_error("Error: file not found " + fileToRead);
        }

        YAML::Node root = YAML::Load(fin);
        std::map<int, ip_address_one> loadedMap;

        // Ожидаем, что YAML — это мапа, где ключ — int, значение — IpAddressOne
        for (auto it = root.begin(); it != root.end(); ++it) {
            int key = it->first.as<int>();
            ip_address_one value = it->second.as<ip_address_one>();
            loadedMap[key] = value;
        }

        return loadedMap;
    }

    void WriteYaml(const std::map<int, ip_address_one>& data, const std::string& path = "") {
        std::string fileToWrite = path.empty() ? _pathFileName : path;

        std::ofstream fout(fileToWrite);
        if (!fout.is_open()) {
            throw std::runtime_error("Error: cannot open file for writing " + fileToWrite);
        }

        YAML::Node root;
        for (const auto& [key, value] : data) {
            root[key] = value;
        }

        fout << root;
    }
};
