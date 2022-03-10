#pragma once

#include <string>

/// \brief Main Lab program.
class CameraPoseSolution
{
public:
  /// \brief Constructs the lab.
  /// \param data_path Optional path to dataset.
  explicit CameraPoseSolution(const std::string& data_path = "../data/");

  /// \brief Runs the lab.
  void run();

private:
  std::string data_path_;
  std::string window_name_;
};
