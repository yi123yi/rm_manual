//
// Created by peter on 2021/5/24.
//

#include "rm_manual/referee/referee_ui.h"

namespace rm_manual {
void RefereeUi::displayCapInfo(GraphicOperateType graph_operate_type) {
  if (ros::Time::now() - last_update_cap_time_ > ros::Duration(0.5)) return;
  float cap_power = referee_->super_capacitor_.parameters[3] * 100;
  char power_string[30];
  sprintf(power_string, "Cap: %1.0f%%", cap_power);
  if (cap_power >= 60)
    referee_->drawString(910, 100, 0, power_string, GREEN, graph_operate_type);
  else if (cap_power < 60 && cap_power >= 30)
    referee_->drawString(910, 100, 0, power_string, YELLOW, graph_operate_type);
  else if (cap_power < 30)
    referee_->drawString(910, 100, 0, power_string, ORANGE, graph_operate_type);
  last_update_cap_time_ = ros::Time::now();
}

void RefereeUi::displayChassisInfo(uint8_t chassis_mode, bool burst_flag, GraphicOperateType graph_operate_type) {
  GraphicColorType color = burst_flag ? ORANGE : YELLOW;
  if (chassis_mode == rm_msgs::ChassisCmd::PASSIVE)
    referee_->drawString(1470, 790, 1, "chassis:passive", color, graph_operate_type);
  else if (chassis_mode == rm_msgs::ChassisCmd::FOLLOW)
    referee_->drawString(1470, 790, 1, "chassis:follow", color, graph_operate_type);
  else if (chassis_mode == rm_msgs::ChassisCmd::GYRO)
    referee_->drawString(1470, 790, 1, "chassis:gyro", color, graph_operate_type);
  else if (chassis_mode == rm_msgs::ChassisCmd::TWIST)
    referee_->drawString(1470, 790, 1, "chassis:twist", color, graph_operate_type);
}

void RefereeUi::displayGimbalInfo(uint8_t gimbal_mode, GraphicOperateType graph_operate_type) {
  if (gimbal_mode == rm_msgs::GimbalCmd::PASSIVE)
    referee_->drawString(1470, 740, 2, "gimbal:passive", YELLOW, graph_operate_type);
  else if (gimbal_mode == rm_msgs::GimbalCmd::RATE)
    referee_->drawString(1470, 740, 2, "gimbal:rate", YELLOW, graph_operate_type);
  else if (gimbal_mode == rm_msgs::GimbalCmd::TRACK)
    referee_->drawString(1470, 740, 2, "gimbal:track", YELLOW, graph_operate_type);
}

void RefereeUi::displayShooterInfo(uint8_t shooter_mode, bool burst_flag, GraphicOperateType graph_operate_type) {
  GraphicColorType color = burst_flag ? ORANGE : YELLOW;
  if (shooter_mode == rm_msgs::ShootCmd::PASSIVE)
    referee_->drawString(1470, 690, 3, "shooter:passive", color, graph_operate_type);
  else if (shooter_mode == rm_msgs::ShootCmd::READY)
    referee_->drawString(1470, 690, 3, "shooter:ready", color, graph_operate_type);
  else if (shooter_mode == rm_msgs::ShootCmd::PUSH)
    referee_->drawString(1470, 690, 3, "shooter:push", color, graph_operate_type);
  else if (shooter_mode == rm_msgs::ShootCmd::STOP)
    referee_->drawString(1470, 690, 3, "shooter:stop", color, graph_operate_type);
}

void RefereeUi::displayAttackTargetInfo(bool attack_base_flag, GraphicOperateType graph_operate_type) {
  if (attack_base_flag) referee_->drawString(1470, 640, 4, "target:base", YELLOW, graph_operate_type);
  else referee_->drawString(1470, 640, 4, "target:all", YELLOW, graph_operate_type);
}

void RefereeUi::displayArmorInfo(const ros::Time &time) {
  if (referee_->referee_data_.robot_hurt_.hurt_type_ == 0x0) {
    double yaw = getArmorPosition();
    if (referee_->referee_data_.robot_hurt_.armor_id_ == 0) {
      referee_->drawCircle((int) (960 + 340 * sin(0 + yaw)), (int) (540 + 340 * cos(0 + yaw)),
                           50, 5, YELLOW, ADD);
      last_update_armor0_time_ = time;
    } else if (referee_->referee_data_.robot_hurt_.armor_id_ == 1) {
      referee_->drawCircle((int) (960 + 340 * sin(3 * M_PI_2 + yaw)), (int) (540 + 340 * cos(3 * M_PI_2 + yaw)),
                           50, 6, YELLOW, ADD);
      last_update_armor1_time_ = time;
    } else if (referee_->referee_data_.robot_hurt_.armor_id_ == 2) {
      referee_->drawCircle((int) (960 + 340 * sin(M_PI + yaw)), (int) (540 + 340 * cos(M_PI + yaw)),
                           50, 7, YELLOW, ADD);
      last_update_armor2_time_ = time;
    } else if (referee_->referee_data_.robot_hurt_.armor_id_ == 3) {
      referee_->drawCircle((int) (960 + 340 * sin(M_PI_2 + yaw)), (int) (540 + 340 * cos(M_PI_2 + yaw)),
                           50, 8, YELLOW, ADD);
      last_update_armor3_time_ = time;
    }
    referee_->referee_data_.robot_hurt_.hurt_type_ = 0x9;
    referee_->referee_data_.robot_hurt_.armor_id_ = 9;
  }

  if (time - last_update_armor0_time_ > ros::Duration(0.5))
    referee_->drawCircle(0, 0, 0, 5, YELLOW, DELETE);
  if (time - last_update_armor1_time_ > ros::Duration(0.5))
    referee_->drawCircle(0, 0, 0, 6, YELLOW, DELETE);
  if (time - last_update_armor2_time_ > ros::Duration(0.5))
    referee_->drawCircle(0, 0, 0, 7, YELLOW, DELETE);
  if (time - last_update_armor3_time_ > ros::Duration(0.5))
    referee_->drawCircle(0, 0, 0, 8, YELLOW, DELETE);
}

double RefereeUi::getArmorPosition() {
  geometry_msgs::TransformStamped gimbal_transformStamped;
  double roll, pitch, yaw;

  try { gimbal_transformStamped = this->tf_.lookupTransform("yaw", "base_link", ros::Time(0)); }
  catch (tf2::TransformException &ex) {}
  quatToRPY(gimbal_transformStamped.transform.rotation, roll, pitch, yaw);
  return yaw;
}
} // namespace rm_manual
