#include <annotate_standalone/bag_player.h>
#include <QFileInfo>

namespace annotate
{

BagPlayer::BagPlayer(QObject* parent) : QObject(parent)
{
  clock_pub_ = nh_.advertise<rosgraph_msgs::Clock>("/clock", 10);
  tf_pub_ = nh_.advertise<tf2_msgs::TFMessage>("/tf", 100);
  tf_static_pub_ = nh_.advertise<tf2_msgs::TFMessage>("/tf_static", 100, true);

  pause_service_ = nh_.advertiseService("pause_playback", &BagPlayer::handlePauseService, this);

  playback_timer_ = new QTimer(this);
  connect(playback_timer_, SIGNAL(timeout()), this, SLOT(publishNext()));
}

BagPlayer::~BagPlayer()
{
  close();
}

bool BagPlayer::open(const std::string& bag_file)
{
  close();

  bag_ = std::make_unique<rosbag::Bag>();
  try
  {
    bag_->open(bag_file, rosbag::bagmode::Read);
  }
  catch (rosbag::BagException& e)
  {
    ROS_ERROR_STREAM("Failed to open bag: " << e.what());
    bag_.reset();
    return false;
  }

  bag_file_ = bag_file;

  // Auto-detect point cloud topic if not set
  if (topic_.empty())
  {
    auto topics = getPointCloudTopics();
    if (!topics.empty())
    {
      topic_ = topics.front();
    }
  }

  // Create publisher on the bag's original topic so AnnotateDisplay can subscribe
  if (!topic_.empty())
  {
    cloud_pub_ = nh_.advertise<sensor_msgs::PointCloud2>(topic_, 10);
  }

  loadFrameIndex();
  cacheStaticTf();

  Q_EMIT bagOpened(QString::fromStdString(bag_file));

  if (!frame_index_.empty())
  {
    seekToFrame(0);
  }

  return true;
}

void BagPlayer::close()
{
  pause();
  if (bag_)
  {
    bag_->close();
    bag_.reset();
  }
  frame_index_.clear();
  current_frame_ = -1;
  static_transforms_.clear();
  Q_EMIT bagClosed();
}

void BagPlayer::play()
{
  if (!isOpen() || frame_index_.empty())
    return;
  if (playing_)
    return;

  playing_ = true;

  // Compute playback interval from actual bag frame rate
  double interval_ms = 100.0 / playback_speed_;
  if (frame_index_.size() > 1)
  {
    double total_time = (frame_index_.back().time - frame_index_.front().time).toSec();
    double avg_interval = total_time / (frame_index_.size() - 1);
    interval_ms = avg_interval * 1000.0 / playback_speed_;
  }
  playback_timer_->start(static_cast<int>(std::max(1.0, interval_ms)));
  Q_EMIT playbackStateChanged(true);
}

void BagPlayer::pause()
{
  if (!playing_)
    return;
  playing_ = false;
  playback_timer_->stop();
  Q_EMIT playbackStateChanged(false);
}

void BagPlayer::togglePlayPause()
{
  if (playing_)
    pause();
  else
    play();
}

void BagPlayer::stepForward()
{
  if (!isOpen() || frame_index_.empty())
    return;
  pause();
  int next = current_frame_ + 1;
  if (next < static_cast<int>(frame_index_.size()))
  {
    publishFrame(next);
  }
}

void BagPlayer::stepBackward()
{
  if (!isOpen() || frame_index_.empty())
    return;
  pause();
  int prev = current_frame_ - 1;
  if (prev >= 0)
  {
    publishFrame(prev);
  }
}

void BagPlayer::seekToFrame(int frame_index)
{
  if (!isOpen() || frame_index < 0 || frame_index >= static_cast<int>(frame_index_.size()))
    return;
  publishFrame(frame_index);
}

bool BagPlayer::isPlaying() const
{
  return playing_;
}

bool BagPlayer::isOpen() const
{
  return bag_ != nullptr;
}

ros::Time BagPlayer::currentTime() const
{
  if (current_frame_ >= 0 && current_frame_ < static_cast<int>(frame_index_.size()))
    return frame_index_[current_frame_].time;
  return ros::Time();
}

int BagPlayer::currentFrame() const
{
  return current_frame_;
}

int BagPlayer::totalFrames() const
{
  return static_cast<int>(frame_index_.size());
}

std::string BagPlayer::topic() const
{
  return topic_;
}

void BagPlayer::setTopic(const std::string& topic)
{
  if (topic_ == topic)
    return;
  topic_ = topic;
  cloud_pub_ = nh_.advertise<sensor_msgs::PointCloud2>(topic_, 10);
  if (isOpen())
  {
    loadFrameIndex();
    if (!frame_index_.empty())
    {
      seekToFrame(0);
    }
  }
}

void BagPlayer::setPlaybackSpeed(double speed)
{
  playback_speed_ = std::max(0.1, speed);
  if (playing_)
  {
    // Restart timer with new interval
    pause();
    play();
  }
}

double BagPlayer::playbackSpeed() const
{
  return playback_speed_;
}

std::vector<std::string> BagPlayer::getPointCloudTopics() const
{
  std::vector<std::string> topics;
  if (!bag_)
    return topics;

  rosbag::View view(*bag_);
  for (const auto* conn : view.getConnections())
  {
    if (conn->datatype == "sensor_msgs/PointCloud2")
    {
      // Avoid duplicates
      if (std::find(topics.begin(), topics.end(), conn->topic) == topics.end())
      {
        topics.push_back(conn->topic);
      }
    }
  }
  return topics;
}

void BagPlayer::publishNext()
{
  if (!isOpen() || frame_index_.empty())
    return;
  int next = current_frame_ + 1;
  if (next >= static_cast<int>(frame_index_.size()))
  {
    pause();
    return;
  }
  publishFrame(next);
}

void BagPlayer::publishFrame(int index)
{
  if (!bag_ || index < 0 || index >= static_cast<int>(frame_index_.size()))
    return;

  current_frame_ = index;
  ros::Time frame_time = frame_index_[index].time;

  // Publish clock first so the rest of the system sees correct sim time
  publishClock(frame_time);

  // Publish TF data
  publishStaticTf();
  publishTf(frame_time);

  // Small delay to allow TF listener to process transforms
  ros::WallDuration(0.02).sleep();

  // Publish the point cloud message for this frame
  ros::Time start = frame_time - ros::Duration(0.001);
  ros::Time end = frame_time + ros::Duration(0.001);
  rosbag::View view(*bag_, rosbag::TopicQuery(topic_), start, end);
  for (auto it = view.begin(); it != view.end(); ++it)
  {
    auto cloud = it->instantiate<sensor_msgs::PointCloud2>();
    if (cloud)
    {
      cloud_pub_.publish(cloud);
      break;
    }
  }

  Q_EMIT frameChanged(current_frame_, totalFrames());
}

void BagPlayer::publishTf(ros::Time time)
{
  if (!bag_)
    return;

  // Publish dynamic TF for a time window around the current frame
  ros::Time bag_start = rosbag::View(*bag_).getBeginTime();
  ros::Time tf_start = (time - bag_start > ros::Duration(2.0)) ? time - ros::Duration(2.0) : bag_start;
  ros::Time tf_end = time + ros::Duration(0.1);

  std::vector<std::string> tf_topics = { "/tf" };
  try
  {
    rosbag::View tf_view(*bag_, rosbag::TopicQuery(tf_topics), tf_start, tf_end);
    for (auto it = tf_view.begin(); it != tf_view.end(); ++it)
    {
      auto tf_msg = it->instantiate<tf2_msgs::TFMessage>();
      if (tf_msg)
      {
        tf_pub_.publish(tf_msg);
      }
    }
  }
  catch (...)
  {
    // No /tf topic in bag
  }
}

void BagPlayer::publishStaticTf()
{
  if (static_transforms_.empty())
    return;

  tf2_msgs::TFMessage msg;
  msg.transforms = static_transforms_;
  tf_static_pub_.publish(msg);
}

void BagPlayer::publishClock(ros::Time time)
{
  rosgraph_msgs::Clock clock_msg;
  clock_msg.clock = time;
  clock_pub_.publish(clock_msg);
}

void BagPlayer::loadFrameIndex()
{
  frame_index_.clear();
  current_frame_ = -1;

  if (!bag_ || topic_.empty())
    return;

  rosbag::View view(*bag_, rosbag::TopicQuery(topic_));
  for (auto it = view.begin(); it != view.end(); ++it)
  {
    frame_index_.push_back({ it->getTime() });
  }

  ROS_INFO_STREAM("Loaded " << frame_index_.size() << " frames from topic " << topic_);
}

void BagPlayer::cacheStaticTf()
{
  static_transforms_.clear();
  if (!bag_)
    return;

  std::vector<std::string> topics = { "/tf_static" };
  try
  {
    rosbag::View view(*bag_, rosbag::TopicQuery(topics));
    for (auto it = view.begin(); it != view.end(); ++it)
    {
      auto tf_msg = it->instantiate<tf2_msgs::TFMessage>();
      if (tf_msg)
      {
        for (const auto& transform : tf_msg->transforms)
        {
          static_transforms_.push_back(transform);
        }
      }
    }
  }
  catch (...)
  {
    // No /tf_static topic in bag, that's OK
  }

  ROS_INFO_STREAM("Cached " << static_transforms_.size() << " static transforms");
}

bool BagPlayer::handlePauseService(std_srvs::SetBool::Request& req, std_srvs::SetBool::Response& res)
{
  if (req.data)
  {
    // Pause requested
    if (playing_)
    {
      pause();
      res.success = true;
      res.message = "Paused";
    }
    else
    {
      res.success = false;
      res.message = "Already paused";
    }
  }
  else
  {
    // Resume requested
    if (!playing_)
    {
      play();
      res.success = true;
      res.message = "Playing";
    }
    else
    {
      res.success = false;
      res.message = "Already playing";
    }
  }
  return true;
}

}  // namespace annotate
