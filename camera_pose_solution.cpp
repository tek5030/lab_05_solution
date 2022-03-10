#include "camera_pose_solution.h"

#include "dataset.h"
#include "local_coordinate_system.h"
#include "viewer_3d.h"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

CameraPoseSolution::CameraPoseSolution(const std::string& data_path)
  : data_path_{data_path}
  , window_name_{"World point in camera"}
{}

void CameraPoseSolution::run()
{
  // Set up dataset.
  Dataset dataset{data_path_};

  // Define local coordinate system based on the position of a light pole.
  const GeodeticPosition light_pole_position{59.963516, 10.667307, 321.0};
  const LocalCoordinateSystem local_system(light_pole_position);

  // Construct viewers.
  cv::namedWindow(window_name_);
  Viewer3D viewer;

  // Process each image in the dataset.
  for (DataElement element{}; dataset >> element;)
  {
    // Compute the pose of the body in the local coordinate system.
    const Sophus::SE3d pose_local_body = local_system.toLocalPose(element.body_position_in_geo,
                                                                  element.body_attitude_in_geo.toSO3());

    // Add body coordinate axes to the 3D viewer.
    viewer.addBodyAxes(pose_local_body, element.img_num);

    // Compute the pose of the camera relative to the body.
    const Sophus::SE3d pose_body_camera(element.camera_attitude_in_body.toSO3(),
                                        element.camera_position_in_body.toVector());

    // Compute the pose of the camera relative to the local coordinate system.
    const Sophus::SE3d pose_local_camera(pose_local_body * pose_body_camera);

    // Add camera coordinate axes to the 3D viewer.
    viewer.addCameraAxes(pose_local_camera, element.img_num);

    // Construct a camera model based on the intrinsic calibration and camera pose.
    const PerspectiveCameraModel camera_model{element.intrinsics.toCalibrationMatrix(),
                                             pose_local_camera,
                                             element.intrinsics.toDistortionCoefficientVector()};

    // Undistort image.
    cv::Mat undistorted_img = camera_model.undistortImage(element.image);

    // Project world point (the origin) into the image.
    const Eigen::Vector2d pix_pos = camera_model.projectWorldPoint(Eigen::Vector3d::Zero());

    // Draw a marker in the image at the projected position.
    const Eigen::Vector2i pix_pos_int = (pix_pos.array().round()).cast<int>();
    cv::drawMarker(undistorted_img, {pix_pos_int.x(), pix_pos_int.y()}, {0.,0.,255.}, cv::MARKER_CROSS, 40, 3);

    // Add the camera frustum with the image to the 3D viewer.
    viewer.addCameraFrustum(camera_model, undistorted_img, element.img_num);

    // Show the image.
    cv::imshow(window_name_, undistorted_img);

    // Update the windows.
    viewer.spinOnce();
    cv::waitKey(100);
  }

  // Remove image viewer.
  cv::destroyWindow(window_name_);

  // Run 3D viewer until stopped.
  viewer.spin();
}
