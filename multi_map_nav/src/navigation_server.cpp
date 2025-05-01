#include "multi_map_nav/navigation_server.h"

NavigationServer::NavigationServer(const std::string &db_path)
    : as_(nh_, "navigate_to_goal", boost::bind(&NavigationServer::execute, this, _1), false),
      wormhole_manager_(db_path)
{
    current_map_ = "map1";
    as_.start();
    ROS_INFO("Navigation Action Server started successfully. Ready to receive goals.");
}

bool NavigationServer::move_base_to(double x, double y, double yaw)
{
    actionlib::SimpleActionClient<move_base_msgs::MoveBaseAction> ac("move_base", true);
    if (!ac.waitForServer(ros::Duration(5.0)))
    {
        ROS_ERROR("Failed to connect to move_base server within timeout.");
        return false;
    }

    move_base_msgs::MoveBaseGoal mb_goal;
    mb_goal.target_pose.header.frame_id = "map";
    mb_goal.target_pose.header.stamp = ros::Time::now();
    mb_goal.target_pose.pose.position.x = x;
    mb_goal.target_pose.pose.position.y = y;

    tf2::Quaternion q;
    q.setRPY(0, 0, yaw);
    q.normalize();
    mb_goal.target_pose.pose.orientation = tf2::toMsg(q);

    ac.sendGoal(mb_goal);
    ac.waitForResult();

    return ac.getState() == actionlib::SimpleClientGoalState::SUCCEEDED;
}

void NavigationServer::execute(const multi_map_nav::NavigateToGoalGoalConstPtr &goal)
{
    ROS_INFO("Received goal: target_map: %s, target_x: %f, target_y: %f",
             goal->target_map.c_str(), goal->target_x, goal->target_y);

    double yaw = M_PI / 2;
    multi_map_nav::NavigateToGoalResult result;

    if (goal->target_map == current_map_)
    {
        ROS_INFO("Already in target map. Moving directly to goal...");
        if (!move_base_to(goal->target_x, goal->target_y, yaw))
        {
            result.success = false;
            result.message = "Failed to reach goal";
            as_.setAborted(result);
            return;
        }
        result.success = true;
        result.message = "Reached";
        ROS_INFO("Robot successfully reached the goal in map: %s", current_map_.c_str());
        as_.setSucceeded(result);
        return;
    }

    auto direct = wormhole_manager_.getWormholeToMap(current_map_, goal->target_map);
    if (direct.first != -9999 && direct.second != -9999)
    {
        ROS_INFO("Found direct wormhole from %s to %s", current_map_.c_str(), goal->target_map.c_str());

        if (!move_base_to(direct.first, direct.second, yaw))
        {
            result.success = false;
            result.message = "Failed to reach wormhole";
            as_.setAborted(result);
            return;
        }

        map_switcher_.switchToMap(goal->target_map);
        current_map_ = goal->target_map;

        if (!move_base_to(goal->target_x, goal->target_y, yaw))
        {
            result.success = false;
            result.message = "Failed to reach goal";
            as_.setAborted(result);
            return;
        }

        result.success = true;
        result.message = "Reached";
        ROS_INFO("Robot successfully reached the goal in map: %s", current_map_.c_str());
        as_.setSucceeded(result);
        return;
    }

    if (current_map_ != "map1")
    {
        auto to_intermediate = wormhole_manager_.getWormholeToMap(current_map_, "map1");
        if (to_intermediate.first == -9999 || to_intermediate.second == -9999)
        {
            ROS_ERROR("No wormhole from %s to map1", current_map_.c_str());
            result.success = false;
            result.message = "No wormhole to map1";
            as_.setAborted(result);
            return;
        }

        ROS_INFO("Moving from %s to map1 via (%f, %f)", current_map_.c_str(), to_intermediate.first, to_intermediate.second);

        if (!move_base_to(to_intermediate.first, to_intermediate.second, yaw))
        {
            result.success = false;
            result.message = "Failed to reach intermediate wormhole";
            as_.setAborted(result);
            return;
        }

        map_switcher_.switchToMap("map1");
        current_map_ = "map1";
    }

    auto to_final = wormhole_manager_.getWormholeToMap("map1", goal->target_map);
    if (to_final.first == -9999 || to_final.second == -9999)
    {
        ROS_ERROR("No wormhole from map1 to %s", goal->target_map.c_str());
        result.success = false;
        result.message = "No wormhole from map1 to target";
        as_.setAborted(result);
        return;
    }

    ROS_INFO("Moving from map1 to %s via (%f, %f)", goal->target_map.c_str(), to_final.first, to_final.second);

    if (!move_base_to(to_final.first, to_final.second, yaw))
    {
        result.success = false;
        result.message = "Failed to reach final wormhole";
        as_.setAborted(result);
        return;
    }

    map_switcher_.switchToMap(goal->target_map);
    current_map_ = goal->target_map;

    if (!move_base_to(goal->target_x, goal->target_y, yaw))
    {
        result.success = false;
        result.message = "Failed to reach goal";
        as_.setAborted(result);
        return;
    }

    result.success = true;
    result.message = "Reached";
    ROS_INFO("Robot successfully reached the goal in map: %s", current_map_.c_str());
    as_.setSucceeded(result);
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "navigation_server_node");
    ros::NodeHandle nh;

    std::string db_path;
    nh.param<std::string>("wormhole_db_path", db_path,
                          ros::package::getPath("multi_map_nav") + "/database/wormholes.db");

    ROS_INFO("Starting navigation server with wormhole database at: %s", db_path.c_str());
    NavigationServer navigation_server(db_path);

    ros::spin();

    return 0;
}
