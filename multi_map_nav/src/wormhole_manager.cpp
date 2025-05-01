#include "multi_map_nav/wormhole_manager.h"


WormholeManager::WormholeManager(const std::string &db_path)
{
    if (sqlite3_open(db_path.c_str(), &db_))
    {
        ROS_ERROR("Failed to open database: %s", sqlite3_errmsg(db_));
        db_ = nullptr;
    }
}

WormholeManager::~WormholeManager()
{
    if (db_)
        sqlite3_close(db_);
}

std::pair<double, double> WormholeManager::getWormholeToMap(const std::string &current_map, const std::string &target_map)
{
    double x =  -9999 , y = -9999;

    std::string sql = "SELECT from_x, from_y FROM wormholes WHERE from_map = ? AND to_map = ?;";
    sqlite3_stmt *stmt;

    ROS_INFO("Executing query: %s", sql.c_str());

    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
    { 
        sqlite3_bind_text(stmt, 1, current_map.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, target_map.c_str(), -1, SQLITE_TRANSIENT);

        int step_result = sqlite3_step(stmt);
        if (step_result == SQLITE_ROW)
        {
           
            x = sqlite3_column_double(stmt, 0);           
            y = sqlite3_column_double(stmt, 1);           
            ROS_INFO("Wormhole found at (%f, %f)", x, y); 
        }
        else
        {
            ROS_WARN("No wormhole found from %s to %s. Step result: %d", current_map.c_str(), target_map.c_str(), step_result);
        }
        sqlite3_finalize(stmt); 
    }
    else
    {
        ROS_ERROR("Query failed: %s", sqlite3_errmsg(db_));
    }

    return {x, y};
}