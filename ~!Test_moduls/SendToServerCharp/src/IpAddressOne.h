#pragma once
#include <string>
#include <yaml-cpp/node/node.h>

struct ip_address_one {
    int id;
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
            node["Id"] = rhs.id;
            node["Name"] = rhs.name;
            node["IpAddress"] = rhs.ip_address;
            node["Port1"] = rhs.port1;
            node["Port2"] = rhs.port2;
            return node;
        }

        static bool decode(const Node& node, ip_address_one& rhs) {
            if (!node.IsMap()) return false;
            rhs.id = node["id"].as<int>();
            rhs.name = node["name"].as<std::string>();
            rhs.ip_address = node["ipAddress"].as<std::string>();
            rhs.port1 = node["port1"].as<int>();
            rhs.port2 = node["port2"].as<int>();
            return true;
        }
    };
}

