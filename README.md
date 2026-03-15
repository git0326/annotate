# annotate_standalone

Standalone 3D annotation tool with built-in rosbag playback control, based on RViz.

This project is based on the [annotate](https://github.com/Earthwings/annotate) RViz plugin, but provides a self-contained application that **loads and reads rosbag files directly** instead of relying on external `rosbag play`.

## Key Differences from Original `annotate`

| Feature | Original `annotate` | `annotate_standalone` |
|---|---|---|
| Bag playback | External `rosbag play` | Built-in BagPlayer |
| Playback control | Via `pause_playback` service (external) | UI buttons + keyboard shortcuts |
| Frame navigation | Forward only (space to advance) | Forward, backward, seek to any frame |
| Timeline | No visual timeline | Slider with frame counter |
| Application type | RViz plugin (loaded in standard rviz) | Standalone app (embeds rviz) |
| Annotation features | Full | Full (identical) |

## Requirements

- ROS Melodic or Noetic
- The `annotate` package (for icon resources, in the same workspace)

## Building

```bash
cd /path/to/your/workspace/src
# Ensure annotate_standalone is in the workspace
catkin build annotate_standalone
source /path/to/your/workspace/devel/setup.bash
```

## Usage

### Launch with a bag file

```bash
roslaunch annotate_standalone annotate_standalone.launch bag:="/path/to/your/bag_file.bag"
```

### Launch without a bag (open one later via the UI)

```bash
roslaunch annotate_standalone annotate_standalone.launch
```

### Direct executable

```bash
rosrun annotate_standalone annotate_standalone_node /path/to/your/bag_file.bag
```

## Workflow

1. **Open a bag file**: Click "Open Bag" in the Bag Player panel at the bottom, or provide it as a launch argument.
2. **Select the topic**: The point cloud topic is auto-detected. Switch topics using the dropdown if needed.
3. **Set the display topic**: In the Annotated PointCloud2 display, set the Topic field to match the bag's point cloud topic.
4. **Navigate frames**: Use the playback controls:
   - **Play/Pause**: Continuous playback or pause
   - **Step Forward/Back**: Frame-by-frame navigation
   - **First/Last**: Jump to beginning or end
   - **Timeline slider**: Seek to any frame
   - **Speed control**: Adjust playback speed (0.1x to 10x)
5. **Annotate**: Use the same annotation workflow as the original project:
   - Click "New Annotation" tool, then click on the point cloud
   - Right-click annotation box to set label, adjust mode, commit
   - Use keyboard shortcuts (Ctrl+F auto-fit, Ctrl+C commit, etc.)
6. **Save**: Annotations are saved to the configured YAML file on each commit.

## Keyboard Shortcuts

| Action | Default Shortcut |
|---|---|
| Undo | Ctrl+Z |
| Toggle pause | Space |
| Rotate clockwise | Right arrow |
| Rotate anti-clockwise | Left arrow |
| Shrink to points | Ctrl+B |
| Auto-fit points | Ctrl+F |
| Commit annotation | Ctrl+C |

## Icons

This project references icons from the `annotate` package (`package://annotate/icons/`).
If you remove the original `annotate` package, copy its `icons/` directory into this package.

## Architecture

- **`annotate_standalone_node`**: Main executable embedding RViz with the Bag Player dock widget
- **`libannotate_standalone.so`**: RViz plugin library providing the Annotated PointCloud2 display and New Annotation tool
- **`BagPlayer`**: Reads rosbag files, publishes point cloud / TF / clock messages, provides `pause_playback` service
- **`BagPlayerWidget`**: Qt widget with playback controls (buttons, slider, topic selector)
- **Annotation code**: Adapted from the original `annotate` project with identical functionality
