#include <QApplication>
#include <QDockWidget>
#include <ros/ros.h>
#include <rviz/visualization_frame.h>
#include <rviz/visualization_manager.h>
#include <rviz/display.h>
#include <annotate_standalone/bag_player.h>
#include <annotate_standalone/bag_player_widget.h>
#include <QString>

int main(int argc, char** argv)
{
  QApplication app(argc, argv);
  app.setApplicationName("Annotate Standalone");

  ros::init(argc, argv, "annotate_standalone");

  // Enable simulated time so TF and point cloud timestamps work correctly
  ros::NodeHandle nh;
  nh.setParam("/use_sim_time", true);

  // Create bag player
  annotate::BagPlayer* bag_player = new annotate::BagPlayer();

  // Create the full rviz visualization frame (provides displays panel, tools, views, etc.)
  rviz::VisualizationFrame* frame = new rviz::VisualizationFrame();
  frame->setApp(&app);
  frame->initialize();
  frame->setWindowTitle("Annotate Standalone");

  // Create a PointCloud2 display that will be driven by the bag player
  rviz::Display* pc_display = frame->getManager()->createDisplay(
      "rviz/PointCloud2", "Point Cloud", true);
  if (pc_display)
  {
    pc_display->subProp("Size (m)")->setValue(0.04f);
    pc_display->subProp("Style")->setValue("Spheres");
  }

  // When the bag has no TF, automatically sync RViz's Fixed Frame to the
  // point cloud's frame_id and update the display topic so no transform lookup is needed
  QObject::connect(bag_player, &annotate::BagPlayer::cloudFrameIdChanged,
                   [frame, bag_player, pc_display](const QString& frame_id) {
                     frame->getManager()->setFixedFrame(frame_id);
                     if (pc_display)
                     {
                       pc_display->subProp("Topic")->setValue(
                           QString::fromStdString(bag_player->topic()));
                     }
                   });

  // Add the bag player widget as a dock at the bottom
  annotate::BagPlayerWidget* player_widget = new annotate::BagPlayerWidget(bag_player);
  QDockWidget* player_dock = new QDockWidget("Bag Player", frame);
  player_dock->setWidget(player_widget);
  player_dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
  frame->addDockWidget(Qt::BottomDockWidgetArea, player_dock);

  frame->show();

  // Use async spinner for ROS callbacks (services, subscribers)
  ros::AsyncSpinner spinner(2);
  spinner.start();

  // If a bag file path was provided as argument, open it
  for (int i = 1; i < argc; ++i)
  {
    std::string arg = argv[i];
    if (arg.size() > 4 && arg.substr(arg.size() - 4) == ".bag")
    {
      bag_player->open(arg);
      break;
    }
  }

  int result = app.exec();

  spinner.stop();
  bag_player->close();
  delete bag_player;

  return result;
}
