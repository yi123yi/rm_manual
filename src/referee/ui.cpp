//
// Created by peter on 2021/7/20.
//

#include "rm_manual/referee/ui.h"

namespace rm_manual {
UiBase::UiBase(ros::NodeHandle &nh, Data &data, const std::string &ui_type) : data_(data) {
  XmlRpc::XmlRpcValue config_param;
  if (!nh.getParam(ui_type, config_param)) {
    ROS_ERROR("%s no defined (namespace %s)", ui_type.c_str(), nh.getNamespace().c_str());
    return;
  }
  try {
    for (int i = 0; i < (int) config_param.size(); ++i)
      graph_vector_.insert(std::pair<std::string, Graph *>(config_param[i]["name"],
                                                           new Graph(config_param[i]["data"], data_.referee_)));
  } catch (XmlRpc::XmlRpcException &e) { ROS_ERROR("Wrong ui parameter: %s", e.getMessage().c_str()); }
  for (auto graph:graph_vector_) graph.second->setOperation(rm_common::GraphOperation::DELETE);
}

void UiBase::add() {
  for (auto graph:graph_vector_) {
    graph.second->setOperation(rm_common::GraphOperation::ADD);
    graph.second->display();
  }
}

void StateUi::update(const std::string &graph_name, uint8_t mode, bool burst_flag, bool color_flag) {
  auto graph = graph_vector_.find(graph_name);
  if (graph != graph_vector_.end()) {
    updateConfig(graph_name, graph->second, mode, burst_flag, color_flag);
    graph->second->setOperation(rm_common::GraphOperation::UPDATE);
    graph->second->display();
  }
}

void StateUi::updateConfig(const std::string &name, Graph *graph, uint8_t mode, bool burst_flag, bool color_flag) {
  if (name == "chassis") {
    graph->setContent(getChassisState(mode));
    if (burst_flag) graph->setColor(rm_common::GraphColor::ORANGE);
    else graph->setColor(rm_common::GraphColor::WHITE);
  } else if (name == "target") {
    graph->setContent(getTargetState(mode));
    if (burst_flag) graph->setColor(rm_common::GraphColor::ORANGE);
    else if (color_flag) graph->setColor(rm_common::GraphColor::PINK);
    else graph->setColor(rm_common::GraphColor::CYAN);
  }
}

const std::string StateUi::getChassisState(uint8_t mode) {
  if (mode == rm_msgs::ChassisCmd::RAW) return "raw";
  else if (mode == rm_msgs::ChassisCmd::FOLLOW) return "follow";
  else if (mode == rm_msgs::ChassisCmd::GYRO) return "gyro";
  else if (mode == rm_msgs::ChassisCmd::TWIST) return "twist";
  else return "error";
}

const std::string StateUi::getTargetState(uint8_t mode) {
  if (mode == rm_msgs::StatusChangeRequest::BUFF) return "buff";
  else if (mode == rm_msgs::StatusChangeRequest::ARMOR) return "armor";
  else return "error";
}

void AimUi::update() {
  if (data_.referee_.referee_data_.game_robot_status_.robot_level_ >= 4) return;
  for (auto graph:graph_vector_) {
    updateConfig(graph.second, data_.referee_.referee_data_.game_robot_status_.robot_level_);
    graph.second->setOperation(rm_common::GraphOperation::UPDATE);
    graph.second->display();
  }
}
void AimUi::updateConfig(Graph *graph, int level) {
  graph->updateX(level);
  graph->updateY(level);
}

void ArmorUi::update(const ros::Time &time) {
  for (auto graph:graph_vector_) {
    if (data_.referee_.referee_data_.robot_hurt_.hurt_type_ == 0x00
        && data_.referee_.referee_data_.robot_hurt_.armor_id_ == getArmorId(graph.first)) {
      updateConfig(graph.first, graph.second);
      graph.second->display(time, true, true);
      data_.referee_.referee_data_.robot_hurt_.hurt_type_ = 9;
    } else graph.second->display(time, false, true);
  }
}

void ArmorUi::updateConfig(const std::string &name, Graph *graph) {
  geometry_msgs::TransformStamped yaw2baselink;
  double roll, pitch, yaw;
  try { yaw2baselink = data_.tf_buffer_.lookupTransform("yaw", "base_link", ros::Time(0)); }
  catch (tf2::TransformException &ex) {}
  quatToRPY(yaw2baselink.transform.rotation, roll, pitch, yaw);
  if (getArmorId(name) == 0 || getArmorId(name) == 2) {
    graph->setStartX((int) (960 + 340 * sin(getArmorId(name) * M_PI_2 + yaw)));
    graph->setStartY((int) (540 + 340 * cos(getArmorId(name) * M_PI_2 + yaw)));
  } else {
    graph->setStartX((int) (960 + 340 * sin(-getArmorId(name) * M_PI_2 + yaw)));
    graph->setStartY((int) (540 + 340 * cos(-getArmorId(name) * M_PI_2 + yaw)));
  }
}

uint8_t ArmorUi::getArmorId(const std::string &name) {
  if (name == "armor0") return 0;
  else if (name == "armor1") return 1;
  else if (name == "armor2") return 2;
  else if (name == "armor3") return 3;
  return 9;
}

void WarningUi::update(const std::string &name, const ros::Time &time, bool state) {
  auto graph = graph_vector_.find(name);
  if (graph != graph_vector_.end()) graph->second->display(time, !state);
}

void CapacitorUi::add() {
  auto graph = graph_vector_.find("capacitor");
  if (graph != graph_vector_.end() && data_.referee_.referee_data_.capacity_data.cap_power_ != 0.) {
    graph->second->setOperation(rm_common::GraphOperation::ADD);
    graph->second->display();
  }
}

void CapacitorUi::update(const ros::Time &time) {
  auto graph = graph_vector_.find("capacitor");
  if (graph != graph_vector_.end() && data_.referee_.referee_data_.capacity_data.cap_power_ != 0.) {
    updateConfig(graph->second, data_.referee_.referee_data_.capacity_data.cap_power_ * 100);
    graph->second->setOperation(rm_common::GraphOperation::UPDATE);
    graph->second->display(time);
  }
}

void CapacitorUi::updateConfig(Graph *graph, double data) {
  char data_str[30] = {' '};
  sprintf(data_str, "cap:%1.0f%%", data);
  graph->setContent(data_str);
  if (data < 30.) graph->setColor(rm_common::GraphColor::ORANGE);
  else if (data > 70.) graph->setColor(rm_common::GraphColor::GREEN);
  else graph->setColor(rm_common::GraphColor::YELLOW);
}

}