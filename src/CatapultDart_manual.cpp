//
// Created by luotinkai on 2022/7/15.
//

#include "rm_manual/CatapultDart_manual.h"

namespace rm_manual {
CatapultDartManual::CatapultDartManual(ros::NodeHandle &nh, ros::NodeHandle &nh_referee) : ManualBase(nh, nh_referee) {
  //  ros::NodeHandle dart_list_nh(nh, "dart_list");
  XmlRpc::XmlRpcValue dart_list, targets, launch_id;
  nh.getParam("launch_id", launch_id);
  nh.getParam("dart_list", dart_list);
  nh.getParam("targets", targets);
  getList(dart_list, targets, launch_id);


  ros::NodeHandle nh_chassis_yaw = ros::NodeHandle(nh, "chassis_yaw");
  ros::NodeHandle nh_trigger_left = ros::NodeHandle(nh, "trigger_left");

  chassis_yaw_sender_ = new rm_common::JointPointCommandSender(nh_chassis_yaw, joint_state_);
  trigger_sender_ = new rm_common::JointPointCommandSender(nh_trigger_left, joint_state_);

  ros::NodeHandle nh_dart_position = ros::NodeHandle(nh, "dart_position");
  ros::NodeHandle nh_trigger_switch = ros::NodeHandle(nh, "trigger_switch");


  trigger_position_sender_ = new rm_common::JointPointCommandSender(nh_dart_position, joint_state_);

  trigger_switch_sender_ = new rm_common::JointPointCommandSender(nh_trigger_switch, joint_state_);


  XmlRpc::XmlRpcValue trigger_rpc_value, gimbal_rpc_value;
  nh.getParam("trigger_calibration", trigger_rpc_value);
  trigger_calibration_ = new rm_common::CalibrationQueue(trigger_rpc_value, nh, controller_manager_);
  nh.getParam("gimbal_calibration", gimbal_rpc_value);
  gimbal_calibration_ = new rm_common::CalibrationQueue(gimbal_rpc_value, nh, controller_manager_);

  left_switch_up_event_.setActiveHigh(boost::bind(&CatapultDartManual::leftSwitchUpOn, this));
  left_switch_mid_event_.setActiveHigh(boost::bind(&CatapultDartManual::leftSwitchMidOn, this));
  left_switch_down_event_.setActiveHigh(boost::bind(&CatapultDartManual::leftSwitchDownOn, this));
  right_switch_down_event_.setActiveHigh(boost::bind(&CatapultDartManual::rightSwitchDownOn, this));
  right_switch_mid_event_.setRising(boost::bind(&CatapultDartManual::rightSwitchMidRise, this));
  right_switch_up_event_.setRising(boost::bind(&CatapultDartManual::rightSwitchUpRise, this));
  wheel_clockwise_event_.setRising(boost::bind(&CatapultDartManual::wheelClockwise, this));
  wheel_anticlockwise_event_.setRising(boost::bind(&CatapultDartManual::wheelAntiClockwise, this));
  dbus_sub_ = nh.subscribe<rm_msgs::DbusData>("/dbus_data", 10, &CatapultDartManual::dbusDataCallback, this);
  dart_client_cmd_sub_ = nh_referee.subscribe<rm_msgs::DartClientCmd>("dart_client_cmd_data", 10,
                                                                      &CatapultDartManual::dartClientCmdCallback, this);
  game_robot_hp_sub_ =
      nh_referee.subscribe<rm_msgs::GameRobotHp>("game_robot_hp", 10, &CatapultDartManual::gameRobotHpCallback, this);
  game_status_sub_ =
      nh_referee.subscribe<rm_msgs::GameStatus>("game_status", 10, &CatapultDartManual::gameStatusCallback, this);
}


void CatapultDartManual::getList(const XmlRpc::XmlRpcValue &darts, const XmlRpc::XmlRpcValue &targets,
                         const XmlRpc::XmlRpcValue &launch_id ) {
  for (const auto &dart: darts) {
    ROS_ASSERT(dart.second.hasMember("param") and dart.second.hasMember("id"));
    ROS_ASSERT(dart.second["param"].getType() == XmlRpc::XmlRpcValue::TypeArray and
               dart.second["id"].getType() == XmlRpc::XmlRpcValue::TypeInt);

    for (int i = 0; i < 4; ++i) {
      if (dart.second["id"] == launch_id[i]) {
        Dart dart_info;
        dart_info.outpost_dart_position_ = static_cast<double>(dart.second["param"][1]);
        dart_info.outpost_shoot_position_ = static_cast<double>(dart.second["param"][2]);
        dart_info.base_dart_position_ = static_cast<double>(dart.second["param"][3]);
        dart_info.base_shoot_position_ = static_cast<double>(dart.second["param"][4]);

        dart_list_.insert(std::make_pair(i, dart_info));
      }
    }
  }
  for (const auto &target: targets) {
    ROS_ASSERT(target.second.hasMember("position"));
    ROS_ASSERT(target.second["position"].getType() == XmlRpc::XmlRpcValue::TypeArray);
    std::vector<double> position(2);
    position[0] = static_cast<double>(target.second["position"][0]);
    target_position_.insert(std::make_pair(target.first, position));
  }
}

void CatapultDartManual::run() {
  ManualBase::run();
  trigger_calibration_->update(ros::Time::now());
  gimbal_calibration_->update(ros::Time::now());
}

void CatapultDartManual::gameRobotStatusCallback(const rm_msgs::GameRobotStatus::ConstPtr &data) {
  ManualBase::gameRobotStatusCallback(data);
  robot_id_ = data->robot_id;
}

void CatapultDartManual::gameStatusCallback(const rm_msgs::GameStatus::ConstPtr &data) {
  ManualBase::gameStatusCallback(data);
  game_progress_ = data->game_progress;
}

void CatapultDartManual::sendCommand(const ros::Time &time) {
  trigger_sender_->sendCommand(time);
  trigger_position_sender_->sendCommand(time);
  trigger_switch_sender_->sendCommand(time);
  chassis_yaw_sender_->sendCommand(time);

}

void CatapultDartManual::updateRc(const rm_msgs::DbusData::ConstPtr &dbus_data) {
  ManualBase::updateRc(dbus_data);
  move(chassis_yaw_sender_, dbus_data->ch_l_x);
}

void CatapultDartManual::updatePc(const rm_msgs::DbusData::ConstPtr &dbus_data) {
  ManualBase::updatePc(dbus_data);
  double velocity_threshold = 0.001;
  if ( yaw_velocity_ < velocity_threshold)
    move_state_ = STOP;
  else
    move_state_ = MOVING;
  getDartFiredNum();
  if (game_progress_ == rm_msgs::GameStatus::IN_BATTLE) {
    switch (auto_state_) {
      case OUTPOST:

        break;
      case BASE:

        break;
    }
    moveDart();
    if (last_dart_door_status_ - dart_launch_opening_status_ ==
        rm_msgs::DartClientCmd::OPENING_OR_CLOSING - rm_msgs::DartClientCmd::OPENED) {
      dart_door_open_times_++;
      initial_dart_fired_num_ = dart_fired_num_;
      has_stopped = false;
    }
    if (move_state_ == STOP)
      launch_state_ = AIMED;
    else
      launch_state_ = NONE;
    if (dart_launch_opening_status_ == rm_msgs::DartClientCmd::OPENED) {
      switch (launch_state_) {
        case NONE:

          break;
        case AIMED:
          LaunchDart();
          break;
      }
    } else{

    }


    last_dart_door_status_ = dart_launch_opening_status_;
  } else {
    AllToZero();
  }
}

void CatapultDartManual::checkReferee() {
  ManualBase::checkReferee();
}

void CatapultDartManual::remoteControlTurnOn() {
  ManualBase::remoteControlTurnOn();
  gimbal_calibration_->stopController();
  trigger_calibration_->stopController();

  gimbal_calibration_->reset();
  trigger_calibration_->reset();

}

void CatapultDartManual::leftSwitchDownOn() {
 AllToZero();
}

void CatapultDartManual::leftSwitchMidOn() {
  getDartFiredNum();
  initial_dart_fired_num_ = dart_fired_num_;
  has_stopped = false;

      moveDart();

}

void CatapultDartManual::leftSwitchUpOn() {
  getDartFiredNum();

  LaunchDart();

  switch (manual_state_) {
    case OUTPOST:

      break;
    case BASE:

      break;
  }

}


void CatapultDartManual::LaunchDart(){

  switch (Launch_state_) {
    case ToArrive:
      moveDart();
      break;
    case ToReady:
      waitUntilReady();
      break;
    case ToShoot:
      launchTwoDart();
      break;
  }

}

void CatapultDartManual::rightSwitchDownOn() {
  recordPosition(dbus_data_);
  if (dbus_data_.ch_l_y == 1.) {
    chassis_yaw_sender_->setPoint(chassis_yaw_outpost_);
  }
  if (dbus_data_.ch_l_y == -1.) {
    chassis_yaw_sender_->setPoint(chassis_yaw_base_);
  }
  if (dbus_data_.ch_l_x == 1.) {
    chassis_yaw_sender_->setPoint(target_position_["outpost"][0]);
  }
  if (dbus_data_.ch_l_x == -1.) {
    chassis_yaw_sender_->setPoint(target_position_["base"][0]);
  }
}

void CatapultDartManual::rightSwitchMidRise() {
  ManualBase::rightSwitchMidRise();
  dart_door_open_times_ = 0;
  initial_dart_fired_num_ = 0;
  move_state_ = NORMAL;
}

void CatapultDartManual::rightSwitchUpRise() {
  ManualBase::rightSwitchUpRise();
  chassis_yaw_sender_->setPoint(0);
}

void CatapultDartManual::move(rm_common::JointPointCommandSender *joint, double ch) {
  if (!joint_state_.position.empty()) {
    double position = joint_state_.position[joint->getIndex()];
    if (ch != 0.) {
      joint->setPoint(position + ch * scale_);
      if_stop_ = true;
    }
    if (ch == 0. && if_stop_) {
      joint->setPoint(joint_state_.position[joint->getIndex()]);
      if_stop_ = false;
    }
  }
}

void CatapultDartManual::triggerComeBackProtect() {
  if (trigger_position_ < 0.0003)
    trigger_sender_->setPoint(0.);
}

void CatapultDartManual::waitAfterLaunch(double time) {
  if (!has_stopped)
    stop_time_ = ros::Time::now();
  if (ros::Time::now() - stop_time_ < ros::Duration(time))
  {

    has_stopped = true;
  } else{
    trigger_sender_->setPoint(upward_vel_);
  }


    ReadyToShoot();
}

void CatapultDartManual::AllToZero()
{

}

// 检查是否符合发射条件
void CatapultDartManual::ReadyToShoot()
{
  if( !have_launch )
  {
    Launch_state_ = ToArrive ;
  }else
  {

    have_launch = false;
  }

}


void CatapultDartManual::waitUntilReady(){
  if( !have_launch )
  {
    switch (trigger_state_)
    {
      case Dropdown:
        have_launch = false ;

        break;
      case Reset:

        break;
    }
  }

}


void CatapultDartManual::moveDart()
{
  Launch_state_ = ToShoot;
}


void CatapultDartManual::launchTwoDart() {
  if (dart_fired_num_ < 4) {


  } else{

  }

}


void CatapultDartManual::getDartFiredNum() {

}

void CatapultDartManual::recordPosition(const rm_msgs::DbusData dbus_data) {
  if (dbus_data.ch_r_y == 1.) {
    chassis_yaw_outpost_ = joint_state_.position[chassis_yaw_sender_->getIndex()];
    ROS_INFO("Recorded outpost position.");
  }
  if (dbus_data.ch_r_y == -1.) {
    chassis_yaw_base_ = joint_state_.position[chassis_yaw_sender_->getIndex()];
    ROS_INFO("Recorded base position.");
  }
}

void CatapultDartManual::dbusDataCallback(const rm_msgs::DbusData::ConstPtr& data) {
  ManualBase::dbusDataCallback(data);
  if (!joint_state_.name.empty()) {
    yaw_velocity_ = std::abs(joint_state_.velocity[chassis_yaw_sender_->getIndex()]);
  }
  wheel_clockwise_event_.update(data->wheel == 1.0);
  wheel_anticlockwise_event_.update(data->wheel == -1.0);
  dbus_data_ = *data;
}

void CatapultDartManual::dartClientCmdCallback(const rm_msgs::DartClientCmd::ConstPtr& data) {
  dart_launch_opening_status_ = data->dart_launch_opening_status;
}

void CatapultDartManual::gameRobotHpCallback(const rm_msgs::GameRobotHp::ConstPtr& data) {
  switch (robot_id_) {
    case rm_msgs::GameRobotStatus::RED_DART:
      outpost_hp_ = data->blue_outpost_hp;
      break;
    case rm_msgs::GameRobotStatus::BLUE_DART:
      outpost_hp_ = data->red_outpost_hp;
      break;
  }
  if (outpost_hp_ != 0)
    auto_state_ = OUTPOST;
  else
    auto_state_ = BASE;
}


void CatapultDartManual::wheelClockwise() {
  switch (move_state_) {
    case NORMAL:
      scale_ = scale_micro_;
      move_state_ = MICRO;
      ROS_INFO("Pitch and yaw : MICRO_MOVE_MODE");
      break;
    case MICRO:
      scale_ = 0.04;
      move_state_ = NORMAL;
      ROS_INFO("Pitch and yaw : NORMAL_MOVE_MODE");
      break;
  }
}

void CatapultDartManual::wheelAntiClockwise() {
  switch (manual_state_) {
    case OUTPOST:
      manual_state_ = BASE;
      ROS_INFO("Launch target : BASE_MODE");
      chassis_yaw_sender_->setPoint(chassis_yaw_base_);
      break;
    case BASE:
      manual_state_ = OUTPOST;
      ROS_INFO("Launch target : OUTPOST_MODE");
      chassis_yaw_sender_->setPoint(chassis_yaw_outpost_);
      break;
  }
}

}