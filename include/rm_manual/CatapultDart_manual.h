//
// Created by luotinkai on 2022/7/15.
//

#pragma once

#include "rm_manual/manual_base.h"
#include <rm_common/decision/calibration_queue.h>
#include <rm_msgs/DartClientCmd.h>
#include <rm_msgs/GameRobotStatus.h>
#include <rm_msgs/GameStatus.h>
#include <unordered_map>

namespace rm_manual
{
class CatapultDartManual : public ManualBase
{
public:
  CatapultDartManual(ros::NodeHandle& nh, ros::NodeHandle& nh_referee);
  enum AimMode
  {
    OUTPOST,
    BASE
  };
  enum TriggerMode
  {
    ToArrive,
    ToReady,
    ToShoot,
  };
  enum DartMode
  {
    Dropdown,
    Reset,
  };
  enum MoveMode
  {
    NORMAL,
    MICRO,
    MOVING,
    STOP
  };
  enum LaunchMode
  {
    NONE,
    AIMED
  };
  struct Dart
  {
    double outpost_dart_position_, base_dart_position_;
    double outpost_shoot_position_, base_shoot_position_;
    double dart_push_position_, dart_trans_position_;
  };

protected:
  void sendCommand(const ros::Time& time) override;
  void getList(const XmlRpc::XmlRpcValue& darts, const XmlRpc::XmlRpcValue& targets,
               const XmlRpc::XmlRpcValue& launch_id);
  void run() override;
  void checkReferee() override;
  void remoteControlTurnOn() override;
  void leftSwitchMidOn();
  void leftSwitchDownOn();
  void leftSwitchUpOn();
  void rightSwitchDownOn() override;
  void rightSwitchMidRise() override;
  void rightSwitchUpRise() override;
  void updateRc(const rm_msgs::DbusData::ConstPtr& dbus_data) override;
  void updatePc(const rm_msgs::DbusData::ConstPtr& dbus_data) override;
  void move(rm_common::JointPointCommandSender* joint, double ch);
  void recordPosition(const rm_msgs::DbusData dbus_data);
  void waitAfterLaunch(const double time);
  void launchTwoDart();
  void getDartFiredNum();
  void triggerComeBackProtect();
  void LaunchDart();
  void AllToZero();
  void ReadyToShoot();
  void waitUntilReady();
  void moveDart();
  void gameRobotStatusCallback(const rm_msgs::GameRobotStatus::ConstPtr& data) override;
  void dbusDataCallback(const rm_msgs::DbusData::ConstPtr& data) override;
  void dartClientCmdCallback(const rm_msgs::DartClientCmd::ConstPtr& data);
  void gameRobotHpCallback(const rm_msgs::GameRobotHp::ConstPtr& data) override;
  void gameStatusCallback(const rm_msgs::GameStatus::ConstPtr& data) override;
  void wheelClockwise();
  void wheelAntiClockwise();
  rm_common::JointPointCommandSender *trigger_sender_, *trigger_position_sender_, *trigger_switch_sender_;
  rm_common::JointPointCommandSender *chassis_yaw_sender_;
  rm_common::CalibrationQueue *trigger_calibration_, *gimbal_calibration_;
  double pitch_outpost_{}, pitch_base_{}, chassis_yaw_outpost_{}, chassis_yaw_base_{};
  double qd_, upward_vel_;
  std::unordered_map<int, Dart> dart_list_{};
  std::unordered_map<std::string, std::vector<double>> target_position_{};
  double scale_{ 0.04 }, scale_micro_{ 0.01 };
  bool if_stop_{ true }, has_stopped{ false },have_launch{true};

  rm_msgs::DbusData dbus_data_;
  uint8_t robot_id_, game_progress_, dart_launch_opening_status_;

  int dart_fired_num_ = 0, initial_dart_fired_num_ = 0;
  double trigger_position_ = 0., pitch_velocity_ = 0., yaw_velocity_ = 0.;
  InputEvent wheel_clockwise_event_, wheel_anticlockwise_event_;
  ros::Time stop_time_;
  ros::Subscriber dart_client_cmd_sub_;
  InputEvent dart_client_cmd_event_;
  int outpost_hp_;
  int dart_door_open_times_ = 0, last_dart_door_status_ = 1;
  int auto_state_ = OUTPOST, manual_state_ = OUTPOST, move_state_ = NORMAL, launch_state_ = NONE;
  int trigger_state_=Dropdown;
  int Launch_state_ = ToArrive;
};
}  // namespace rm_manual
