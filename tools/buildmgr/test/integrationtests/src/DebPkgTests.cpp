/*
 * Copyright (c) 2020-2021 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * IMPORTANT:
 * These tests are designed to run only in CI environment.
*/

#include "CBuildIntegTestEnv.h"
#include "CbuildUtils.h"

#include <algorithm>

using namespace std;

class DebPkgTests : public ::testing::Test {
public:
  void SetUp();
  void ValidateExtract(const string& path);
protected:
  string FindPackage  (const string& path);
  void ExtractPackage (const string& pkg, const string& extPath);
};

void DebPkgTests::SetUp() {
  if (CBuildIntegTestEnv::ci_installer_path.empty()) {
    GTEST_SKIP();
  }
}

string DebPkgTests::FindPackage(const string& path) {
  string cmd = "find " + path + " -name *.deb";
  string pkgfile = CbuildUtils::ExecCommand(cmd.c_str()).first;
  pkgfile.erase(pkgfile.find_last_not_of(" \f\n\r\t\v") + 1);
  return pkgfile;
}

void DebPkgTests::ExtractPackage(const string& pkg, const string& extPath) {
  string cmd = "dpkg-deb -xv " + pkg + " " + extPath;
  auto res = CbuildUtils::ExecCommand(cmd.c_str());

  ASSERT_FALSE(res.first.empty());
  ASSERT_EQ(res.second, 0);
}

void DebPkgTests::ValidateExtract(const string& extPath) {
  vector<string> pathVec = {
    "./etc/cmsis-build/AC5.5.6.7.cmake", "./etc/cmsis-build/AC6.6.16.0.cmake",
    "./etc/cmsis-build/CPRJ.xsd", "./etc/cmsis-build/GCC.10.2.1.cmake",
    "./etc/cmsis-build/setup",
    "./etc/profile.d/cmsis-build.sh", "./usr/bin/cbuild.sh",
    "./usr/bin/cp_init.sh",
    "./usr/bin/cp_install.sh", "./usr/doc/doc-base/cmsis-build",
    "./usr/lib/cmsis-build/bin/cbuild.sh",
    "./usr/lib/cmsis-build/bin/cbuildgen",
    "./usr/lib/cmsis-build/bin/cp_init.sh", "./usr/lib/cmsis-build/bin/cp_install.sh",
    "./usr/lib/cmsis-build/etc/AC5.5.6.7.cmake", "./usr/lib/cmsis-build/etc/AC6.6.16.0.cmake",
    "./usr/lib/cmsis-build/etc/CPRJ.xsd", "./usr/lib/cmsis-build/etc/GCC.10.2.1.cmake",
    "./usr/lib/cmsis-build/etc/setup",
    "./usr/share/doc/cmsis-build/copyright",
    "./usr/share/doc/cmsis-build/doc/index.html"
  };

  error_code ec;
  fs::current_path(extPath, ec);

  for_each(pathVec.begin(), pathVec.end(),
    [&](const string& str) { ASSERT_TRUE(fs::exists(str, ec)) << str << " not found !!!"; });
}

TEST_F(DebPkgTests, CheckMetadata) {
  string debpkgpath = CrossPlatformUtils::GetEnv("CI_CBUILD_DEB_PKG");
  string cmd, del = ", ", substr;
  set<string> deplist;
  set<string> expectDep = {
    "cmake", "ninja-build",
    "curl", "libxml2-utils",
    "dos2unix", "unzip" };

  cmd = "find " + debpkgpath + " -name *.deb";

  auto result = CbuildUtils::ExecCommand(cmd.c_str());
  ASSERT_FALSE(result.first.empty());
  ASSERT_EQ(result.second, 0);

  string pkgfile = result.first;
  cmd = "dpkg-deb --info " + pkgfile;
  auto res = CbuildUtils::ExecCommand(cmd.c_str());
  ASSERT_FALSE(res.first.empty());
  ASSERT_EQ(res.second, 0);

  istringstream f(res.first);
  string package, arch, depends, section, priority, homepage;
  string line;

  while (getline(f, line)) {
    if (line.compare(0, 10, " Package: ")           == 0)  package  = line.substr(10, line.length());
    else if (line.compare(0, 15, " Architecture: ") == 0)  arch     = line.substr(15, line.length());
    else if (line.compare(0, 10, " Depends: ")      == 0)  depends  = line.substr(10, line.length());
    else if (line.compare(0, 10, " Section: ")      == 0)  section  = line.substr(10, line.length());
    else if (line.compare(0, 11, " Priority: ")     == 0)  priority = line.substr(11, line.length());
    else if (line.compare(0, 11, " Homepage: ")     == 0)  homepage = line.substr(11, line.length());
    else {
      continue;
    }
  }

  ASSERT_EQ(package,  "cmsis-build");
  ASSERT_EQ(arch,     "amd64");
  ASSERT_EQ(section,  "devel");
  ASSERT_EQ(priority, "optional");
  ASSERT_EQ(homepage, "https://arm-software.github.io/CMSIS_5/Build/html/index.html");

  size_t start, end = 0;
  while ((start = depends.find_first_not_of(", ", end)) != std::string::npos) {
    end = depends.find(", ", start);
    deplist.insert(depends.substr(start, end - start));
  }

  for_each(expectDep.begin(), expectDep.end(), [&](const string& dep) {
    if (deplist.find(dep) == deplist.end()) {
      ASSERT_TRUE(false) << "error: dependency [" << dep << "] not found";
    }
  });
}

TEST_F(DebPkgTests, Extract_Completion) {
  error_code ec;
  string pkgPath, package, extPath;

  pkgPath = CrossPlatformUtils::GetEnv("CI_CBUILD_DEB_PKG");
  package = FindPackage(pkgPath);
  extPath = testout_folder + "/debextract";

  if (fs::exists(extPath, ec)) {
    RemoveDir(extPath);
  }

  ExtractPackage(package, extPath);
  ValidateExtract(extPath);
}
