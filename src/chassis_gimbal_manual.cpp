//
// Created by peter on 2021/7/22.
//

#include "rm_manual/chassis_gimbal_manual.h"

namespace rm_manual {
ChassisGimbalManual::ChassisGimbalManual(ros::NodeHandle &nh) : ManualBase(nh) {
  ros::NodeHandle chassis_nh(nh, "chassis");
  chassis_cmd_sender_ =
      new rm_common::ChassisCommandSender(chassis_nh, data_.referee_data_);
  ros::NodeHandle vel_nh(nh, "vel");
  vel_cmd_sender_ = new rm_common::Vel2DCommandSender(vel_nh);
  if (!vel_nh.getParam("gyro_move_reduction", gyro_move_reduction_))
    ROS_ERROR("Gyro move reduction no defined (namespace: %s)",
              nh.getNamespace().c_str());
  ros::NodeHandle gimbal_nh(nh, "gimbal");
  gimbal_cmd_sender_ =
      new rm_common::GimbalCommandSender(gimbal_nh, data_.referee_data_);
  ros::NodeHandle ui_nh(nh, "ui");

  chassis_power_on_event_.setRising(
      boost::bind(&ChassisGimbalManual::chassisOutputOn, this));
  gimbal_power_on_event_.setRising(
      boost::bind(&ChassisGimbalManual::gimbalOutputOn, this));
  w_event_.setEdge(boost::bind(&ChassisGimbalManual::wPress, this),
                   boost::bind(&ChassisGimbalManual::wRelease, this));
  w_event_.setActiveHigh(boost::bind(&ChassisGimbalManual::wPressing, this));
  s_event_.setEdge(boost::bind(&ChassisGimbalManual::sPress, this),
                   boost::bind(&ChassisGimbalManual::sRelease, this));
  s_event_.setActiveHigh(boost::bind(&ChassisGimbalManual::sPressing, this));
  a_event_.setEdge(boost::bind(&ChassisGimbalManual::aPress, this),
                   boost::bind(&ChassisGimbalManual::aRelease, this));
  a_event_.setActiveHigh(boost::bind(&ChassisGimbalManual::aPressing, this));
  d_event_.setEdge(boost::bind(&ChassisGimbalManual::dPress, this),
                   boost::bind(&ChassisGimbalManual::dRelease, this));
  d_event_.setActiveHigh(boost::bind(&ChassisGimbalManual::dPressing, this));
  mouse_mid_event_.setRising(
      boost::bind(&ChassisGimbalManual::mouseMidRise, this));
}

void ChassisGimbalManual::sendCommand(const ros::Time &time) {
  chassis_cmd_sender_->sendCommand(time);
  vel_cmd_sender_->sendCommand(time);
  gimbal_cmd_sender_->sendCommand(time);
}

void ChassisGimbalManual::updateRc() {
  ManualBase::updateRc();
  if (std::abs(data_.dbus_data_.wheel) > 0.01) {
    chassis_cmd_sender_->setMode(rm_msgs::ChassisCmd::GYRO);
  } else
    chassis_cmd_sender_->setMode(rm_msgs::ChassisCmd::FOLLOW);
  vel_cmd_sender_->setAngularZVel(data_.dbus_data_.wheel);
  vel_cmd_sender_->setLinearXVel(data_.dbus_data_.ch_r_y);
  vel_cmd_sender_->setLinearYVel(-data_.dbus_data_.ch_r_x);
  gimbal_cmd_sender_->setRate(-data_.dbus_data_.ch_l_x,
                              -data_.dbus_data_.ch_l_y);
}

void ChassisGimbalManual::updatePc() {
  ManualBase::updatePc();

  if (data_.dbus_data_.m_x != 0. || data_.dbus_data_.m_y != 0.)
    gimbal_cmd_sender_->setRate(-data_.dbus_data_.m_x * gimbal_scale_,
                                data_.dbus_data_.m_y * gimbal_scale_);
}

void ChassisGimbalManual::checkReferee() {
  ManualBase::checkReferee();
  chassis_power_on_event_.update(
      data_.referee_data_.game_robot_status_.mains_power_chassis_output_);
  gimbal_power_on_event_.update(
      data_.referee_data_.game_robot_status_.mains_power_gimbal_output_);
}

void ChassisGimbalManual::checkKeyboard() {
  ManualBase::checkKeyboard();
  if (data_.referee_data_.robot_id_ == rm_common::RobotId::RED_ENGINEER ||
      data_.referee_data_.robot_id_ == rm_common::RobotId::BLUE_ENGINEER) {
    w_event_.update((!data_.dbus_data_.key_ctrl) &
                    (!data_.dbus_data_.key_shift) & data_.dbus_data_.key_w);
    s_event_.update((!data_.dbus_data_.key_ctrl) &
                    (!data_.dbus_data_.key_shift) & data_.dbus_data_.key_s);
    a_event_.update((!data_.dbus_data_.key_ctrl) &
                    (!data_.dbus_data_.key_shift) & data_.dbus_data_.key_a);
    d_event_.update((!data_.dbus_data_.key_ctrl) &
                    (!data_.dbus_data_.key_shift) & data_.dbus_data_.key_d);
  } else {
    w_event_.update(data_.dbus_data_.key_w);
    s_event_.update(data_.dbus_data_.key_s);
    a_event_.update(data_.dbus_data_.key_a);
    d_event_.update(data_.dbus_data_.key_d);
  }
  mouse_mid_event_.update(data_.dbus_data_.m_z != 0.);
}

void ChassisGimbalManual::remoteControlTurnOff() {
  ManualBase::remoteControlTurnOff();
  vel_cmd_sender_->setZero();
  chassis_cmd_sender_->setZero();
  gimbal_cmd_sender_->setZero();
}

void ChassisGimbalManual::rightSwitchDownRise() {
  ManualBase::rightSwitchDownRise();
  chassis_cmd_sender_->setMode(rm_msgs::ChassisCmd::FOLLOW);
  vel_cmd_sender_->setZero();
  gimbal_cmd_sender_->setMode(rm_msgs::GimbalCmd::RATE);
  gimbal_cmd_sender_->setZero();
}

void ChassisGimbalManual::rightSwitchMidRise() {
  ManualBase::rightSwitchMidRise();
  chassis_cmd_sender_->setMode(rm_msgs::ChassisCmd::FOLLOW);
  gimbal_cmd_sender_->setMode(rm_msgs::GimbalCmd::RATE);
}

void ChassisGimbalManual::rightSwitchUpRise() {
  ManualBase::rightSwitchUpRise();
  chassis_cmd_sender_->setMode(rm_msgs::ChassisCmd::FOLLOW);
  vel_cmd_sender_->setZero();
  gimbal_cmd_sender_->setMode(rm_msgs::GimbalCmd::RATE);
}

void ChassisGimbalManual::leftSwitchMidFall() {
  chassis_cmd_sender_->power_limit_->updateState(rm_common::PowerLimit::CHARGE);
}

void ChassisGimbalManual::leftSwitchDownRise() {
  ManualBase::leftSwitchDownRise();
  gimbal_cmd_sender_->setMode(rm_msgs::GimbalCmd::RATE);
}

void ChassisGimbalManual::wPressing() {
  vel_cmd_sender_->setLinearXVel(chassis_cmd_sender_->getMsg()->mode ==
                                         rm_msgs::ChassisCmd::GYRO
                                     ? x_scale_ * gyro_move_reduction_
                                     : x_scale_);
}

void ChassisGimbalManual::wRelease() {
  x_scale_ = x_scale_ <= -1.0 ? -1.0 : x_scale_ - 1.0;
  vel_cmd_sender_->setLinearXVel(x_scale_);
}

void ChassisGimbalManual::sPressing() {
  vel_cmd_sender_->setLinearXVel(chassis_cmd_sender_->getMsg()->mode ==
                                         rm_msgs::ChassisCmd::GYRO
                                     ? x_scale_ * gyro_move_reduction_
                                     : x_scale_);
}

void ChassisGimbalManual::sRelease() {
  x_scale_ = x_scale_ >= 1.0 ? 1.0 : x_scale_ + 1.0;
  vel_cmd_sender_->setLinearXVel(x_scale_);
}

void ChassisGimbalManual::aPressing() {
  vel_cmd_sender_->setLinearYVel(chassis_cmd_sender_->getMsg()->mode ==
                                         rm_msgs::ChassisCmd::GYRO
                                     ? y_scale_ * gyro_move_reduction_
                                     : y_scale_);
}

void ChassisGimbalManual::aRelease() {
  y_scale_ = y_scale_ <= -1.0 ? -1.0 : y_scale_ - 1.0;
  vel_cmd_sender_->setLinearYVel(y_scale_);
}

void ChassisGimbalManual::dPressing() {
  vel_cmd_sender_->setLinearYVel(chassis_cmd_sender_->getMsg()->mode ==
                                         rm_msgs::ChassisCmd::GYRO
                                     ? y_scale_ * gyro_move_reduction_
                                     : y_scale_);
}

void ChassisGimbalManual::dRelease() {
  y_scale_ = y_scale_ >= 1.0 ? 1.0 : y_scale_ + 1.0;
  vel_cmd_sender_->setLinearYVel(y_scale_);
}

void ChassisGimbalManual::mouseMidRise() {
  if (gimbal_scale_ >= 0. && gimbal_scale_ <= 3.) {
    if (gimbal_scale_ + 0.2 <= 3. && data_.dbus_data_.m_z > 0.)
      gimbal_scale_ += 0.2;
    else if (gimbal_scale_ - 0.2 >= 0. && data_.dbus_data_.m_z < 0.)
      gimbal_scale_ -= 0.2;
  }
}

} // namespace rm_manual
