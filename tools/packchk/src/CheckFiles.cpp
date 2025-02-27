/*
 * Copyright (c) 2020-2021 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "CheckFiles.h"

#include "CrossPlatform.h"
#include "ErrLog.h"
#include "RteUtils.h"
#include "RteFsUtils.h"

#include <algorithm>

using namespace std;

/**
 * @brief visitor class constructor for files found in PDSC description
 * @param packagePath string path to package
 * @param packageName string name of package
*/
CheckFilesVisitor::CheckFilesVisitor(const string& packagePath, const string& packageName)
{
  m_checkFiles.SetPackagePath(packagePath);
  m_checkFiles.SetPackageName(packageName);
}

/**
 * @brief visitor class destructor for files found in PDSC description
*/
CheckFilesVisitor::~CheckFilesVisitor()
{
}

/**
 * @brief visitor callback for files found in PDSC description
 * @param item RteItem file item
 * @return VISIT_RESULT
*/
VISIT_RESULT CheckFilesVisitor::Visit(RteItem* item)
{
  m_checkFiles.CheckFile(item);

  return CONTINUE_VISIT;
}

/**
 * @brief class constructor, checks all files found in PDSC
*/
CheckFiles::CheckFiles()
{
}

/**
 * @brief class destructor
*/
CheckFiles::~CheckFiles()
{
}

/**
 * @brief sets internal representation of package path, calculates path slashes
 * @param packagePath string path to package
*/
void CheckFiles::SetPackagePath(const string& packagePath)
{
  m_packagePath = RteUtils::BackSlashesToSlashes(RteUtils::RemoveTrailingBackslash(packagePath));
}

/**
 * @brief returns internal representation of package path
 * @return string path to package
*/
const string& CheckFiles::GetPackagePath() const
{
  return m_packagePath;
}

/**
 * @brief returns data for attribute "folder"
 * @param item RteItem
 * @return string result or empty
*/
const string& CheckFiles::GetFolder(RteItem* item) const
{
  if(!item) {
    return RteUtils::EMPTY_STRING;
  }

  const string& tag = item->GetTag();
  if(tag == "example") {
    return item->GetAttribute("folder");
  }
  else if(tag == "environment") {
    RteItem* parent = item->GetParent();
    return parent->GetAttribute("folder");
  }

  return RteUtils::EMPTY_STRING;
}

/**
 * @brief returns the file name of a file item considering different categories
 * @param item RteItem item to fetch filename from
 * @param filename return string
 * @param fileType return type
 * @return passed / failed
*/
bool CheckFiles::GetFileName(RteItem* item, std::string& filename, FileType& fileType) const
{
  if(!item) {
    return false;
  }

  fileType = FileType::Generic;

  const string& tag = item->GetTag();
  if(tag == "book" || tag == "algorithm" || tag == "file") {
    filename = item->GetName();
    return true;
  }
  else if(tag == "compile") {
    filename = item->GetAttribute("header");
    return true;
  }
  else if(tag == "debugvars") {
    filename = item->GetAttribute("configfile");
    return true;
  }
  else if(tag == "environment") {
    string str = GetFolder(item);
    str = RteUtils::RemoveTrailingBackslash(str);
    str += "/";
    string ownFolder = item->GetAttribute("folder");
    if(!ownFolder.empty()) {
      str += ownFolder;
      str = RteUtils::RemoveTrailingBackslash(str);
      str += "/";
    }
    str += item->GetAttribute("load");
    str = RteUtils::BackSlashesToSlashes(str);
    filename = str;
    return true;
  }
  else if(tag == "doc") {
    filename = item->GetText();
    return true;
  }

  return false;
}

/**
 * @brief converts text to UPPER chars
 * @param text input/output buffer to convert
 * @return passed / failed
*/
bool CheckFiles::ToUpper(string& text)
{
  std::transform(text.begin(), text.end(), text.begin(), ::toupper);

  return true;
}

/**
 * @brief check aspects of an RTE file item
 * @param item RteItem item to check
 * @return passed / failed
*/
bool CheckFiles::CheckFile(RteItem* item)
{
  if(!item) {
    return true;
  }

  const string& tag = item->GetTag();
  string envName;
  if(tag == "environment") {
    envName = item->GetName();
    ToUpper(envName);
    if(envName != "UV" && envName != "DS5") {
      return true;
    }
  }

  const string& folder = GetFolder(item);
  const string& category = item->GetAttribute("category");
  const string& attr = item->GetAttribute("attr");
  FileType fileType = FileType::Generic;
  int lineNo = item->GetLineNumber();

  string fileName, fileName2;
  if(tag == "example") {
    const string& abstr = item->GetAttribute("doc");
    fileName = RteUtils::RemoveTrailingBackslash(folder);
    fileName += '/';
    fileName += abstr;
    fileName = RteUtils::BackSlashesToSlashes(fileName);
  }
  else if(tag == "image") {
    fileName = item->GetAttribute("large");
    fileName2 = item->GetAttribute("small");
  }
  else {
    GetFileName(item, fileName, fileType);
  }

  if(fileName.empty() && fileName2.empty()) {
    return true;
  }

  if(fileName == "\\" || fileName == ".\\" || fileName == "/" || fileName == "./") {
    return true;
  }

  if(fileName == item->GetTag()) {
    return true;
  }

  // check if fileName is a URL
  if(fileName.find(":", 2) != (size_t)-1 || fileName.find("www.") == 0) {
    return CONTINUE_VISIT;
  }

  // Trim #directJump from htm(l) strings
  if(fileName.find(".htm#") != string::npos || fileName.find(".html#") != string::npos) {
    size_t pos = fileName.find("#");
    if(pos != string::npos) {
      fileName.erase(pos);
    }
  }

  // Filename
  if(!fileName.empty()) {
    if(CheckFileExists(fileName, lineNo)) {
      CheckCaseSense(fileName, lineNo);
    }

    if(tag == "environment" && envName == "DS5") {
      bool pdscFound = false;

      string::size_type pos = fileName.find(".project");
      if(pos != string::npos) {
        string::size_type pos1 = fileName.find_last_of(".");
        if(pos == pos1) {
          pdscFound = true;
          string fn = fileName;
          fn.insert(pos + 1, "c");
          CheckFileExists(fn, lineNo, true);      // Check for associated file
        }
      }
      if(!pdscFound) {
        LogMsg("M321", lineNo);
      }
    }
  }

  // Filename2
  if(!fileName2.empty()) {
    if(CheckFileExists(fileName2, lineNo)) {
      CheckCaseSense(fileName2, lineNo);   // File must exist for this check!
    }
  }

  if(attr == "config") {
    CheckFileHasVersion(item);
  }

  if(attr == "template") {
    CheckTemplate(item);
  }

  if(category == "library" || category == "sourceAsm") {
    CheckCompilerDependency(item);
  }

  CheckFileExtension(item);

  string ext = RteUtils::ExtractFileExtension(fileName);
  if((category == "source" && (!_stricmp(ext.c_str(), "s") || !_stricmp(ext.c_str(), "asm"))) || category == "sourceAsm") {
    CheckAsmGccCompilerDependency(item);
    CheckCompilerDependency(item);
  }

  return true;
}

/**
 * @brief returns full path to a file, appends package path
 * @param fileName pack relative path to a file
 * @return full path to a file
*/
string CheckFiles::GetFullFilename(const string& fileName)
{
  string checkPath = RteUtils::BackSlashesToSlashes(GetPackagePath());
  checkPath += "/";
  checkPath += RteUtils::BackSlashesToSlashes(RteUtils::RemoveTrailingBackslash(fileName));

  return checkPath;
}

/**
 * @brief test if the file can be found physically on the location specified
 * @param fileName full path to the file object
 * @param lineNo line number in PDSC for error reporting
 * @param associated
 * @return passed / failed
*/
bool CheckFiles::CheckFileExists(const string& fileName, int lineNo, bool associated /* = false */)
{
  LogMsg("M074", PATH(fileName));

  string checkPath = GetFullFilename(fileName);

  bool ok = true;
  if(!RteFsUtils::Exists(checkPath)) {
    if(associated) {
      LogMsg("M322", PATH(checkPath), lineNo);
    }
    else {
      LogMsg("M323", PATH(checkPath), lineNo);
    }
    ok = false;
  }

  if(ok) {
    LogMsg("M010");
  }

  return ok;
}

/**
 * @brief searches the filesystem for the exact name of a file object (case sensitive name)
 * @param path the path to search in
 * @param fileNameIn filename as written in PDSC
 * @param fileNameOut filename as written on filesystem
 * @return found / not found
*/
bool CheckFiles::FindGetExactFileSystemName(const std::string& path, const std::string& fileNameIn, string& fileNameOut)
{
  error_code ec;
  for (auto& item : fs::directory_iterator(path, ec)) {
    const string fn = item.path().generic_string();
    const string fsFn = RteUtils::ExtractFileName(fn);
    if(!AlnumCmp::CompareLen(fileNameIn, fsFn, false)) {
      fileNameOut = fsFn;
      return true;
    }
  }

  return false;
}

/**
 * @brief check name as written in PDSC against it's counterpart on the filesystem, for case sensitivity
 * @param fileName filename as written in PDSC
 * @param lineNo line number for error reporting
 * @return
*/
bool CheckFiles::CheckCaseSense(const string& fileName, int lineNo)
{
  if(fileName.empty()) {
    return true;
  }

  LogMsg("M058", PATH(fileName));

  string pdscPath = RteUtils::BackSlashesToSlashes(RteUtils::RemoveTrailingBackslash(fileName));
  string path = pdscPath;
  string systemPath;
  vector<string> pathVect;

  do {
    string actualPath = path;
    path = RteUtils::ExtractFilePath(actualPath, 0);
    string checkPath = RteUtils::ExtractFileName(actualPath);
    string testPath = GetPackagePath();
    testPath += "/";
    testPath += path;
    string outPath;

    if(FindGetExactFileSystemName(testPath, checkPath, outPath)) {
      pathVect.push_back(outPath);
    }
  } while(!path.empty() && path != ".");

  for(auto it2 = pathVect.rbegin(); it2 != pathVect.rend(); it2++) {
    if(!systemPath.empty()) {
      systemPath += "/";
    }
    systemPath += *it2;
  }

  string::size_type pos;
  string::size_type endPos = pdscPath.find_first_not_of("./");
  do {
    pos = pdscPath.find_first_of("./");
    if(pos < endPos && pos != string::npos)
      pdscPath.erase(pos, 1);
  } while(pos < endPos && pos != string::npos);

  bool ok = true;
  if(pdscPath.compare(systemPath)) {
    LogMsg("M310", VAL("PDSC", pdscPath), VAL("SYSTEM", systemPath), lineNo);
    ok = false;
  }

  if(ok) {
    LogMsg("M010");
  }

  return ok;
}

/**
 * @brief check if a file item has a version
 * @param item RteItem
 * @return passed / failed
*/
bool CheckFiles::CheckFileHasVersion(RteItem* item)
{
  if(!item) {
    return false;
  }

  LogMsg("M086", PATH(item->GetName()));

  bool ok = true;
  string ver = item->GetAttribute("version");
  if(ver.empty()) {
    LogMsg("M334", PATH(item->GetName()), item->GetLineNumber());
    ok = false;
  }
  if(ok) {
    LogMsg("M010");
  }

  return ok;
}

/**
 * @brief find condition (recursive)
 * @param filter string name of condition
 * @param cond starting condition
 * @param exList list to add condition to
 * @return
*/
bool CheckFiles::FilterConditions(const string& filter, RteCondition* cond, list<RteItem*>* exList)
{
  if(!cond || !cond->IsValid() || !exList) {
    return true;
  }

  for(auto expression : cond->GetChildren()) {
    FilterConditions(filter, expression->GetCondition(), exList);

    string result = expression->GetAttribute(filter);
    if(!result.empty()) {
      exList->push_back(expression);
    }
  }

  return true;
}

/**
 * @brief verify template items
 * @param item RteItem template item
 * @return passed / failed
*/
bool CheckFiles::CheckTemplate(RteItem* item)
{
  if(!item) {
    return true;
  }

  string name = item->GetAttribute("name");
  string category = item->GetAttribute("category");
  string attr = item->GetAttribute("attr");
  string select = item->GetAttribute("select");
  int lineNo = item->GetLineNumber();

  LogMsg("M057", VAL("ATTR", attr), PATH(name));

  bool ok = true;
  if(category == "include") {
    LogMsg("M342", VAL("ATTR", attr), VAL("CAT", category), PATH(name), lineNo);
    ok = false;
  }

  if(select.empty()) {
    LogMsg("M343", VAL("ATTR", attr), VAL("ATTR2", "select"), PATH(name), lineNo);
    ok = false;
  }

  if(ok) {
    LogMsg("M010");
  }

  return ok;
}

/**
 * @brief Check and filter conditions
 * @param item RteItem
 * @param parentName string
 * @param condFilter string
 * @param condValue string
 * @return passed / failed
*/
bool CheckFiles::CheckForCondition(RteItem* item, const string& parentName, const string& condFilter, const string& condValue)
{
  if(!item) {
    return false;
  }

  RteCondition* cond = item->GetCondition();
  int cnt = 0;

  bool ok = false;
  do {      // if condition has not been found, loop must be reentered searching the parent
    if(cnt++ > 2) {
      break;
    }

    if(!cond) {
      RteItem* parent = item->GetParent();
      do {
        if(parent) {
          parent = parent->GetParent();

          string tag = parent->GetTag();
          if(tag == parentName) {
            cond = parent->GetCondition();
            break;
          }
        }
      } while(parent);

      if(!cond || !cond->IsValid()) {
        return ok;
      }
    }

    list<RteItem*> exList;
    FilterConditions(condFilter, cond, &exList);
    for(auto exItem : exList) {
      RteConditionExpression* expression = dynamic_cast<RteConditionExpression*> (exItem);
      if(!expression) {
        continue;
      }

      string value = expression->GetAttribute(condFilter);
      RteConditionExpression::RTEConditionExpressionType type = expression->GetExpressionType();
      if(!value.empty() && type != RteConditionExpression::DENY) {
        if(condValue.empty()) {
          ok = true;      // found condition, condition has a value
          break;
        }
        else {
          if(value == condValue) {
            ok = true;    // found value
            break;
          }
        }
      }
    }

    cond = 0;
  } while(!ok);

  return ok;
}

/**
 * @brief check gcc compiler dependencies
 * @param item RteItem item to test
 * @return passed / failed
*/
bool CheckFiles::CheckAsmGccCompilerDependency(RteItem* item)
{
  if(!item) {
    return true;
  }

  string name = item->GetAttribute("name");
  string extension = RteUtils::ExtractFileExtension(name);
  string category = item->GetAttribute("category");
  int lineNo = item->GetLineNumber();

  RteCondition* cond = item->GetCondition();
  if(!cond) {
    return true;
  }

  string filter = "Tcompiler";
  list<RteItem*> exList;
  FilterConditions(filter, cond, &exList);

  bool ok = true;
  for(auto exItem : exList) {
    RteConditionExpression* expression = dynamic_cast<RteConditionExpression*> (exItem);
    if(!expression) {
      continue;
    }

    string compiler = expression->GetAttribute(filter);
    RteConditionExpression::RTEConditionExpressionType type = expression->GetExpressionType();
    if(type != RteConditionExpression::DENY) {
      if(!_stricmp(compiler.c_str(), "gcc")) {
        LogMsg("M081", VAL("CAT", category), VAL("COMP", compiler), PATH(name));
        if(extension == "s") {
          LogMsg("M341", VAL("COMP", compiler), PATH(name), EXT(".S"), lineNo);
          ok = false;
          break;
        }
        if(ok) {
          LogMsg("M010");
        }
      }
    }
  }

  return ok;
}

/**
 * @brief generic check of compiler dependencies
 * @param item RteItem item to test
 * @return passed / failed
 */
bool CheckFiles::CheckCompilerDependency(RteItem* item)
{
  if(!item) {
    return true;
  }

  string name = item->GetAttribute("name");
  string category = item->GetAttribute("category");
  string attr = item->GetAttribute("attr");
  int lineNo = item->GetLineNumber();
  const string parentName = "component";
  const string condFilter = "Tcompiler";

  LogMsg("M059", VAL("CAT", category), COND(condFilter), PATH(name));

  if(!CheckForCondition(item, parentName, condFilter)) {
    LogMsg("M344", VAL("CAT", category), COND(condFilter), PATH(name), lineNo);    // "File with category '%CAT%' must have condition '%COND%': '%PATH%'"
    return false;
  }

  LogMsg("M010");

  return true;
}

/**
 * @brief check file extension due to file category
 * @param item RteItem item to test
 * @return passed / failed
*/
bool CheckFiles::CheckFileExtension(RteItem* item)
{
  if(!item) {
    return true;
  }

  string name = RteUtils::BackSlashesToSlashes(item->GetAttribute("name"));
  string category = item->GetAttribute("category");
  string extension = RteUtils::ExtractFileExtension(name);
  int lineNo = item->GetLineNumber();

  if(!name.length() || !category.length()) {
    return true;
  }

  // skip categories that are not tested yet
  if(!(category == "include" || category == "header" || category == "sourceAsm" || category == "sourceC" || category == "sourceCpp")) {
    return true;
  }

  LogMsg("M056", VAL("CAT", category), PATH(name));

  bool ok = true;
  if(category == "include") {
    if(RteFsUtils::IsDirectory(name)) {
      LogMsg("M339", PATH(name), lineNo);
      ok = false;
    }
    else if(name.at(name.length() - 1) != '\\' && name.at(name.length() - 1) != '/') {    // if DIR test already fails, skip this test
      LogMsg("M340", PATH(name), lineNo);
      ok = false;
    }
  }
  else if(category == "header") {
    if(_stricmp(extension.c_str(), "h") && _stricmp(extension.c_str(), "hpp")) {
      LogMsg("M337", VAL("CAT", category), PATH(name), EXT(extension), lineNo);
      ok = false;
    }
  }
  else if(category == "sourceAsm") {
    if(_stricmp(extension.c_str(), "s") && _stricmp(extension.c_str(), "asm")) {
      LogMsg("M337", VAL("CAT", category), PATH(name), EXT(extension), lineNo);
      ok = false;
    }
  }
  else if(category == "sourceC") {
    if(_stricmp(extension.c_str(), "c")) {
      LogMsg("M337", VAL("CAT", category), PATH(name), EXT(extension), lineNo);
      ok = false;
    }
  }
  else if(category == "sourceCpp") {
    if(_stricmp(extension.c_str(), "cpp")) {
      LogMsg("M337", VAL("CAT", category), PATH(name), EXT(extension), lineNo);
      ok = false;
    }
  }

  if(ok) {
    LogMsg("M010");
  }

  return ok;
}
