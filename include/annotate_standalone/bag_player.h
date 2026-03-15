#pragma once

#include <rosbag/bag.h>
#include <rosbag/view.h>
#include <ros/ros.h>
#include <sensor_msgs/PointCloud2.h>
#include <tf2_msgs/TFMessage.h>
#include <rosgraph_msgs/Clock.h>
#include <std_srvs/SetBool.h>
#include <geometry_msgs/TransformStamped.h>
#include <QObject>
#include <QTimer>
#include <memory>
#include <vector>

namespace annotate
{

class BagPlayer : public QObject
{
  Q_OBJECT
public:
  explicit BagPlayer(QObject* parent = nullptr);
  ~BagPlayer();

  bool open(const std::string& bag_file);
  void close();

  void play();
  void pause();
  void togglePlayPause();
  void stepForward();
  void stepBackward();
  void seekToFrame(int frame_index);

  bool isPlaying() const;
  bool isOpen() const;
  ros::Time currentTime() const;
  int currentFrame() const;
  int totalFrames() const;

  std::string topic() const;
  void setTopic(const std::string& topic);
  void setPlaybackSpeed(double speed);
  double playbackSpeed() const;

  std::vector<std::string> getPointCloudTopics() const;

Q_SIGNALS:
  void bagOpened(const QString& filename);
  void bagClosed();
  void frameChanged(int frame, int total);
  void playbackStateChanged(bool playing);
  void cloudFrameIdChanged(const QString& frame_id);

private Q_SLOTS:
  void publishNext();

private:
  void publishFrame(int index);
  void publishTf(ros::Time time);
  void publishStaticTf();
  void publishClock(ros::Time time);
  void loadFrameIndex();
  void cacheStaticTf();
  bool handlePauseService(std_srvs::SetBool::Request& req, std_srvs::SetBool::Response& res);

  std::unique_ptr<rosbag::Bag> bag_;
  std::string bag_file_;
  std::string topic_;
  double playback_speed_{ 1.0 };
  bool playing_{ false };

  struct FrameEntry
  {
    ros::Time time;
  };
  std::vector<FrameEntry> frame_index_;
  int current_frame_{ -1 };

  std::string cloud_frame_id_;
  std::vector<geometry_msgs::TransformStamped> static_transforms_;

  ros::NodeHandle nh_;
  ros::Publisher cloud_pub_;
  ros::Publisher clock_pub_;
  ros::Publisher tf_pub_;
  ros::Publisher tf_static_pub_;
  ros::ServiceServer pause_service_;

  QTimer* playback_timer_;
};

}  // namespace annotate
