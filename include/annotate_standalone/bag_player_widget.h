#pragma once

#include <QWidget>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QComboBox>
#include <QDoubleSpinBox>

namespace annotate
{

class BagPlayer;

class BagPlayerWidget : public QWidget
{
  Q_OBJECT
public:
  explicit BagPlayerWidget(BagPlayer* player, QWidget* parent = nullptr);

private Q_SLOTS:
  void onOpenBag();
  void onPlayPause();
  void onStepForward();
  void onStepBackward();
  void onSliderChanged(int value);
  void onSliderPressed();
  void onSliderReleased();
  void onFrameChanged(int frame, int total);
  void onPlaybackStateChanged(bool playing);
  void onBagOpened(const QString& filename);
  void onBagClosed();
  void onTopicChanged(const QString& topic);
  void onSpeedChanged(double speed);

private:
  BagPlayer* player_;
  QPushButton* open_btn_;
  QPushButton* play_pause_btn_;
  QPushButton* step_fwd_btn_;
  QPushButton* step_back_btn_;
  QPushButton* first_frame_btn_;
  QPushButton* last_frame_btn_;
  QSlider* timeline_slider_;
  QLabel* frame_label_;
  QLabel* time_label_;
  QLabel* bag_label_;
  QComboBox* topic_combo_;
  QDoubleSpinBox* speed_spin_;
  bool slider_pressed_{ false };
};

}  // namespace annotate
