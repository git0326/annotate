#include <annotate_standalone/bag_player_widget.h>
#include <annotate_standalone/bag_player.h>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QStyle>
#include <QFileInfo>

namespace annotate
{

BagPlayerWidget::BagPlayerWidget(BagPlayer* player, QWidget* parent)
  : QWidget(parent), player_(player)
{
  auto* main_layout = new QVBoxLayout(this);
  main_layout->setContentsMargins(4, 4, 4, 4);
  main_layout->setSpacing(4);

  // Top row: bag file and topic selection
  auto* top_layout = new QHBoxLayout();
  open_btn_ = new QPushButton("Open Bag");
  open_btn_->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
  top_layout->addWidget(open_btn_);

  bag_label_ = new QLabel("No bag loaded");
  bag_label_->setMinimumWidth(150);
  top_layout->addWidget(bag_label_);

  top_layout->addWidget(new QLabel("Topic:"));
  topic_combo_ = new QComboBox();
  topic_combo_->setMinimumWidth(200);
  topic_combo_->setEnabled(false);
  top_layout->addWidget(topic_combo_);

  top_layout->addStretch();
  main_layout->addLayout(top_layout);

  // Bottom row: playback controls and timeline
  auto* controls_layout = new QHBoxLayout();

  first_frame_btn_ = new QPushButton();
  first_frame_btn_->setIcon(style()->standardIcon(QStyle::SP_MediaSkipBackward));
  first_frame_btn_->setToolTip("First frame");
  first_frame_btn_->setEnabled(false);
  first_frame_btn_->setFixedWidth(32);
  controls_layout->addWidget(first_frame_btn_);

  step_back_btn_ = new QPushButton();
  step_back_btn_->setIcon(style()->standardIcon(QStyle::SP_MediaSeekBackward));
  step_back_btn_->setToolTip("Previous frame");
  step_back_btn_->setEnabled(false);
  step_back_btn_->setFixedWidth(32);
  controls_layout->addWidget(step_back_btn_);

  play_pause_btn_ = new QPushButton();
  play_pause_btn_->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
  play_pause_btn_->setToolTip("Play / Pause");
  play_pause_btn_->setEnabled(false);
  play_pause_btn_->setFixedWidth(40);
  controls_layout->addWidget(play_pause_btn_);

  step_fwd_btn_ = new QPushButton();
  step_fwd_btn_->setIcon(style()->standardIcon(QStyle::SP_MediaSeekForward));
  step_fwd_btn_->setToolTip("Next frame");
  step_fwd_btn_->setEnabled(false);
  step_fwd_btn_->setFixedWidth(32);
  controls_layout->addWidget(step_fwd_btn_);

  last_frame_btn_ = new QPushButton();
  last_frame_btn_->setIcon(style()->standardIcon(QStyle::SP_MediaSkipForward));
  last_frame_btn_->setToolTip("Last frame");
  last_frame_btn_->setEnabled(false);
  last_frame_btn_->setFixedWidth(32);
  controls_layout->addWidget(last_frame_btn_);

  timeline_slider_ = new QSlider(Qt::Horizontal);
  timeline_slider_->setEnabled(false);
  timeline_slider_->setMinimum(0);
  timeline_slider_->setMaximum(0);
  controls_layout->addWidget(timeline_slider_, 1);

  frame_label_ = new QLabel("0 / 0");
  frame_label_->setMinimumWidth(80);
  frame_label_->setAlignment(Qt::AlignCenter);
  controls_layout->addWidget(frame_label_);

  time_label_ = new QLabel("0.000s");
  time_label_->setMinimumWidth(100);
  time_label_->setAlignment(Qt::AlignCenter);
  controls_layout->addWidget(time_label_);

  controls_layout->addWidget(new QLabel("Speed:"));
  speed_spin_ = new QDoubleSpinBox();
  speed_spin_->setRange(0.1, 10.0);
  speed_spin_->setSingleStep(0.1);
  speed_spin_->setValue(1.0);
  speed_spin_->setSuffix("x");
  speed_spin_->setFixedWidth(80);
  controls_layout->addWidget(speed_spin_);

  main_layout->addLayout(controls_layout);

  // Connect UI signals to local slots
  connect(open_btn_, SIGNAL(clicked()), this, SLOT(onOpenBag()));
  connect(play_pause_btn_, SIGNAL(clicked()), this, SLOT(onPlayPause()));
  connect(step_fwd_btn_, SIGNAL(clicked()), this, SLOT(onStepForward()));
  connect(step_back_btn_, SIGNAL(clicked()), this, SLOT(onStepBackward()));
  connect(first_frame_btn_, &QPushButton::clicked, [this]() {
    player_->seekToFrame(0);
  });
  connect(last_frame_btn_, &QPushButton::clicked, [this]() {
    player_->seekToFrame(player_->totalFrames() - 1);
  });
  connect(timeline_slider_, SIGNAL(valueChanged(int)), this, SLOT(onSliderChanged(int)));
  connect(timeline_slider_, SIGNAL(sliderPressed()), this, SLOT(onSliderPressed()));
  connect(timeline_slider_, SIGNAL(sliderReleased()), this, SLOT(onSliderReleased()));
  connect(topic_combo_, SIGNAL(currentTextChanged(const QString&)), this, SLOT(onTopicChanged(const QString&)));
  connect(speed_spin_, SIGNAL(valueChanged(double)), this, SLOT(onSpeedChanged(double)));

  // Connect BagPlayer signals
  connect(player_, SIGNAL(bagOpened(const QString&)), this, SLOT(onBagOpened(const QString&)));
  connect(player_, SIGNAL(bagClosed()), this, SLOT(onBagClosed()));
  connect(player_, SIGNAL(frameChanged(int, int)), this, SLOT(onFrameChanged(int, int)));
  connect(player_, SIGNAL(playbackStateChanged(bool)), this, SLOT(onPlaybackStateChanged(bool)));
}

void BagPlayerWidget::onOpenBag()
{
  QString file = QFileDialog::getOpenFileName(this, "Open Bag File", QString(),
                                              "ROS Bag Files (*.bag);;All Files (*)");
  if (!file.isEmpty())
  {
    player_->open(file.toStdString());
  }
}

void BagPlayerWidget::onPlayPause()
{
  player_->togglePlayPause();
}

void BagPlayerWidget::onStepForward()
{
  player_->stepForward();
}

void BagPlayerWidget::onStepBackward()
{
  player_->stepBackward();
}

void BagPlayerWidget::onSliderChanged(int value)
{
  if (slider_pressed_)
  {
    player_->seekToFrame(value);
  }
}

void BagPlayerWidget::onSliderPressed()
{
  slider_pressed_ = true;
}

void BagPlayerWidget::onSliderReleased()
{
  slider_pressed_ = false;
  player_->seekToFrame(timeline_slider_->value());
}

void BagPlayerWidget::onFrameChanged(int frame, int total)
{
  if (!slider_pressed_)
  {
    timeline_slider_->blockSignals(true);
    timeline_slider_->setValue(frame);
    timeline_slider_->blockSignals(false);
  }
  frame_label_->setText(QString("%1 / %2").arg(frame + 1).arg(total));

  ros::Time current = player_->currentTime();
  time_label_->setText(QString("%1s").arg(current.toSec(), 0, 'f', 3));
}

void BagPlayerWidget::onPlaybackStateChanged(bool playing)
{
  if (playing)
  {
    play_pause_btn_->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    play_pause_btn_->setToolTip("Pause");
  }
  else
  {
    play_pause_btn_->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    play_pause_btn_->setToolTip("Play");
  }
}

void BagPlayerWidget::onBagOpened(const QString& filename)
{
  QFileInfo info(filename);
  bag_label_->setText(info.fileName());

  // Populate topic combo box
  topic_combo_->blockSignals(true);
  topic_combo_->clear();
  auto topics = player_->getPointCloudTopics();
  for (const auto& topic : topics)
  {
    topic_combo_->addItem(QString::fromStdString(topic));
  }
  if (!topics.empty())
  {
    topic_combo_->setCurrentText(QString::fromStdString(player_->topic()));
  }
  topic_combo_->blockSignals(false);
  topic_combo_->setEnabled(true);

  // Enable controls
  int total = player_->totalFrames();
  timeline_slider_->setMaximum(std::max(0, total - 1));
  timeline_slider_->setValue(0);
  timeline_slider_->setEnabled(total > 0);
  play_pause_btn_->setEnabled(total > 0);
  step_fwd_btn_->setEnabled(total > 1);
  step_back_btn_->setEnabled(total > 1);
  first_frame_btn_->setEnabled(total > 1);
  last_frame_btn_->setEnabled(total > 1);
}

void BagPlayerWidget::onBagClosed()
{
  bag_label_->setText("No bag loaded");
  topic_combo_->clear();
  topic_combo_->setEnabled(false);
  timeline_slider_->setMaximum(0);
  timeline_slider_->setEnabled(false);
  play_pause_btn_->setEnabled(false);
  step_fwd_btn_->setEnabled(false);
  step_back_btn_->setEnabled(false);
  first_frame_btn_->setEnabled(false);
  last_frame_btn_->setEnabled(false);
  frame_label_->setText("0 / 0");
  time_label_->setText("0.000s");
}

void BagPlayerWidget::onTopicChanged(const QString& topic)
{
  if (!topic.isEmpty())
  {
    player_->setTopic(topic.toStdString());
  }
}

void BagPlayerWidget::onSpeedChanged(double speed)
{
  player_->setPlaybackSpeed(speed);
}

}  // namespace annotate
