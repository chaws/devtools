/*
 * Copyright (c) 2020-2021 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef CROSSPLATFORM_UTILS_H
#define CROSSPLATFORM_UTILS_H

#include <string>

 /**
 * @brief CrossPlatformUtils utility class provides proxy methods to call platform-specific functions
*/
class CrossPlatformUtils
{
private:
  // private constructor to prevent instantiating an utility class
  CrossPlatformUtils() {};

public:

  /**
   * @brief Returns environment variable value for given name
   * @param name environment variable name without decorations, e.g. PATH
   * @return value of the variable as std::string:
   * non-empty string if variable is supported, found and set to a non-empty value;
   * empty string in all other cases
   *
  */
  static std::string GetEnv(const std::string& name);

  /**
   * @brief Sets environment variable value
   * @param name environment variable name (may not be empty)
   * @param value string value to set (can be empty)
   * @return true if successful
  */
  static bool SetEnv(const std::string& name, const std::string& value);

  /**
   * @brief Returns CMSIS-Pack root directory path
   * @return CMSIS-Pack root directory path set by CMSIS_PACK_ROOT environment variable or default location
  */
  static std::string GetCMSISPackRootDir();

  /**
   * @brief Returns default CMSIS-Pack root directory path (platform specific)
   * @return default CMSIS-Pack root directory path
  */
  static std::string GetDefaultCMSISPackRootDir();

  /**
   * @brief calculate the wall-clock time used by the calling process
   * @return The elapsed time since the start of the process, measured in milliseconds
  */
  static unsigned long ClockInMsec();

};

#endif  /* #ifndef CROSSPLATFORM_UTILS_H */
