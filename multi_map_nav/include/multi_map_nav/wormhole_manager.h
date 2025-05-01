
#pragma once

#include <string>
#include <utility>
#include <sqlite3.h>
#include <ros/ros.h>

class WormholeManager {
public:
    WormholeManager(const std::string& db_path);
    ~WormholeManager();
    std::pair<double, double> getWormholeToMap(const std::string& current_map, const std::string& target_map);
private:
    sqlite3* db_;
};
