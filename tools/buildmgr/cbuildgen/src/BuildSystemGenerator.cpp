/*
 * Copyright (c) 2020-2021 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 // BuildSystemGenerator.cpp

#include "BuildSystemGenerator.h"

#include "Cbuild.h"
#include "CbuildKernel.h"
#include "CbuildUtils.h"

#include "ErrLog.h"
#include "RteFsUtils.h"
#include "RteUtils.h"

#include <ctime>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>

using namespace std;

BuildSystemGenerator::BuildSystemGenerator(void) {
  // Reserved
}

BuildSystemGenerator::~BuildSystemGenerator(void) {
  // Reserved
}

bool BuildSystemGenerator::Collect(const string& inputFile, const CbuildModel *model, const string& outdir, const string& intdir) {
  error_code ec;
  m_projectDir = StrConv(RteFsUtils::AbsolutePath(inputFile).remove_filename().generic_string());
  m_workingDir = fs::current_path(ec).generic_string() + SS;

  // Find toolchain config
  m_toolchainConfig = StrNorm(model->GetToolchainConfig());
  m_toolchain = model->GetCompiler();

  // Output and intermediate directories
  if (!outdir.empty()) {
    m_outdir = StrConv(outdir);
    if (fs::path(m_outdir).is_relative()) {
      m_outdir = m_workingDir + m_outdir;
    }
  } else if (!model->GetOutDir().empty()) {
    m_outdir = StrConv(model->GetOutDir());
    if (fs::path(m_outdir).is_relative()) {
      m_outdir = m_projectDir + m_outdir;
    }
  } else {
    m_outdir = m_projectDir + "OutDir";
  }
  if (!fs::exists(m_outdir, ec)) {
    fs::create_directories(m_outdir, ec);
  }
  m_outdir = fs::canonical(m_outdir, ec).generic_string() + SS;

  if (!intdir.empty()) {
    m_intdir = StrConv(intdir);
    if (fs::path(m_intdir).is_relative()) {
      m_intdir = m_workingDir + m_intdir;
    }
  } else if (!model->GetIntDir().empty()) {
    m_intdir = StrConv(model->GetIntDir());
    if (fs::path(m_intdir).is_relative()) {
      m_intdir = m_projectDir + m_intdir;
    }
  } else {
    m_intdir = m_projectDir + "IntDir";
  }
  if (!fs::exists(m_intdir, ec)) {
    fs::create_directories(m_intdir, ec);
  }
  m_intdir = fs::canonical(m_intdir, ec).generic_string() + SS;

  // Target attributes
  m_projectName = fs::path(inputFile).stem().generic_string();
  m_targetName = StrNorm(model->GetOutputName());
  const RteTarget* target = model->GetTarget();
  m_targetCpu = target->GetAttribute("Dcore");
  m_targetFpu = target->GetAttribute("Dfpu");
  m_targetDsp = target->GetAttribute("Ddsp");
  m_byteOrder = target->GetAttribute("Dendian");
  m_targetTz = target->GetAttribute("Dtz");
  m_targetSecure = target->GetAttribute("Dsecure");
  m_targetMve = target->GetAttribute("Dmve");
  m_linkerScript = StrNorm(model->GetLinkerScript());
  m_outputType = model->GetOutputType();

  m_ccMscGlobal = GetString(model->GetTargetCFlags());
  m_cxxMscGlobal = GetString(model->GetTargetCxxFlags());
  m_asMscGlobal = GetString(model->GetTargetAsFlags());
  m_linkerMscGlobal = GetString(model->GetTargetLdFlags());

  for (auto inc : model->GetIncludePaths())
  {
    CbuildUtils::PushBackUniquely(m_incPathsList, StrNorm(inc));
  }

  for (auto preinc : model->GetPreIncludeFilesGlobal())
  {
    CbuildUtils::PushBackUniquely(m_preincGlobal, StrNorm(preinc));
  }

  for (auto group : model->GetPreIncludeFilesLocal())
  {
    set<string> preincSet;
    for (auto preinc : group.second) {
      preincSet.insert(StrNorm(preinc));
    }
    m_objGroupsList[StrNorm(group.first)].preinc = preincSet;
  }

  for (auto lib : model->GetLibraries())
  {
    CbuildUtils::PushBackUniquely(m_libFilesList, StrNorm(lib));
  }

  for (auto obj : model->GetObjects())
  {
    CbuildUtils::PushBackUniquely(m_libFilesList, StrNorm(obj));
  }

  for (auto def : model->GetDefines())
  {
    CbuildUtils::PushBackUniquely(m_definesList, def);
  }

  for (auto list : model->GetCSourceFiles())
  {
    string group = list.first;
    string cGFlags;
    const map<string, std::vector<string>>& cFlags = model->GetCFlags();
    string groupName = group;
    do {
      if (cFlags.find(groupName) != cFlags.end()) {
        cGFlags = GetString(cFlags.at(groupName));
        break;
      }
      groupName = fs::path(groupName).parent_path().generic_string();
    } while (!groupName.empty());
    m_objGroupsList[StrNorm(group)].ccMsc = cGFlags;

    for (auto src : list.second) {
      string cFFlags;
      if (cFlags.find(src) != cFlags.end()) cFFlags = GetString(cFlags.at(src));
      string fname = fs::path(StrConv(src)).stem().generic_string();
      string obj = StrNorm(group + (group.empty() ? "" : SS) + fname + ".o");

      if (m_ccFilesList.find(obj) != m_ccFilesList.end()) {
        // Error: object filename was already inserted in a list
        LogMsg("M612", VAL("NAME", fname), VAL("GROUP", group));
        return false;
      }

      m_ccFilesList[obj].src = StrNorm(src);
      m_ccFilesList[obj].flags = cFFlags;
    }
  }

  for (auto list : model->GetCxxSourceFiles())
  {
    string group = list.first;
    string cxxGFlags;
    const map<string, std::vector<string>>& cxxFlags = model->GetCxxFlags();
    string groupName = group;
    do {
      if (cxxFlags.find(groupName) != cxxFlags.end()) {
        cxxGFlags = GetString(cxxFlags.at(groupName));
        break;
      }
      groupName = fs::path(groupName).parent_path().generic_string();
    } while (!groupName.empty());
    m_objGroupsList[StrNorm(group)].cxxMsc = cxxGFlags;

    for (auto src : list.second) {
      string cxxFFlags;
      if (cxxFlags.find(src) != cxxFlags.end()) cxxFFlags = GetString(cxxFlags.at(src));
      string fname = fs::path(StrConv(src)).stem().generic_string();
      string obj = StrNorm(group + SS + fname + ".o");

      if ((m_cxxFilesList.find(obj) != m_cxxFilesList.end()) ||
          (m_ccFilesList.find(obj) != m_ccFilesList.end())) {
        // Error: object filename was already inserted in a list
        LogMsg("M612", VAL("NAME", fname), VAL("GROUP", group));
        return false;
      }

      m_cxxFilesList[obj].src = StrNorm(src);
      m_cxxFilesList[obj].flags = cxxFFlags;
    }
  }

  const map<string, bool> assembler = model->GetAsm();
  m_asTargetAsm = (!assembler.empty()) && (assembler.find("") != assembler.end()) ? assembler.at("") : false;

  for (auto list : model->GetAsmSourceFiles())
  {
    string group = list.first;
    string asGFlags;
    const map<string, std::vector<string>>& asFlags = model->GetAsFlags();
    string groupName = group;
    do {
      if (asFlags.find(groupName) != asFlags.end()) {
        asGFlags = GetString(asFlags.at(groupName));
        break;
      }
      groupName = fs::path(groupName).parent_path().generic_string();
    } while (!groupName.empty());
    m_objGroupsList[StrNorm(group)].asMsc = asGFlags;

    bool group_asm = (assembler.find(group) != assembler.end()) ? assembler.at(group) : m_asTargetAsm;

    for (auto src : list.second) {
      string asFFlags;
      if (asFlags.find(src) != asFlags.end()) asFFlags = GetString(asFlags.at(src));
      string fname = fs::path(StrConv(src)).stem().generic_string();
      string obj = StrNorm(group + SS + fname + ".o");

      // Default assembler: armclang or gcc with gnu syntax and preprocessing
      map<string, module>* pList = &m_asFilesList;

      // Special handling: "legacy" armasm or gas assembler; armasm or gnu syntax
      if ((m_toolchain == "AC6") || (m_toolchain == "GCC")) {
        bool file_asm = (assembler.find(src) != assembler.end()) ? assembler.at(src) : group_asm;
        string flags = !asFFlags.empty() ? asFFlags : !asGFlags.empty() ? asGFlags : m_asMscGlobal;
        if (file_asm) {
          // Legacy assembler (e.g. armasm or gas)
          pList = &m_asLegacyFilesList;
        } else if ((fs::path(StrConv(src)).extension().compare(".S") != 0) && (flags.find("-x assembler-with-cpp") == string::npos)) {
          // Default assembler (e.g. armclang or gcc) without preprocessing
          if ((m_toolchain == "AC6") && ((flags.find("-masm=armasm") != string::npos) || (flags.find("-masm=auto") != string::npos))) {
            // armclang with Arm syntax or Auto
            pList = &m_asArmclangFilesList;
          } else {
            // GNU syntax
            pList = &m_asGnuFilesList;
          }
        }
      }

      if (((*pList).find(obj) != (*pList).end()) ||
          (m_cxxFilesList.find(obj) != m_cxxFilesList.end()) ||
          (m_ccFilesList.find(obj) != m_ccFilesList.end())) {
        // Error: object filename was already inserted in a list
        LogMsg("M612", VAL("NAME", fname), VAL("GROUP", group));
        return false;
      }

      (*pList)[obj].src = StrNorm(src);
      (*pList)[obj].flags = asFFlags;
    }
  }

  // Configuration files
  for (auto cfg : model->GetConfigFiles())
  {
    m_cfgFilesList.insert({ StrNorm(cfg.first), StrNorm(cfg.second) });
  }

  // Audit data
  m_auditData = model->GetAuditData();

  return true;
}

bool BuildSystemGenerator::GenAuditFile(void) {
  // Create audit file
  string filename = m_outdir + m_projectName + LOGEXT;
  ofstream auditFile(filename);
  if (!auditFile) {
    LogMsg("M210", PATH(filename));
    return false;
  }

  // Header
  auditFile << "# CMSIS Build Audit File generated on " << CbuildUtils::GetLocalTimestamp() << EOL << EOL;
  auditFile << "# Project Description File: " << m_projectDir << m_projectName << PDEXT << EOL << EOL;
  auditFile << "# Toolchain Configuration File: " << m_toolchainConfig;

  // Add audit data from RTE Model
  auditFile << m_auditData;

  // Flush and close audit file
  auditFile << std::endl;
  auditFile.close();
  return true;
}

string BuildSystemGenerator::StrNorm(string path) {
  /* StrNorm:
  - Convert backslashes into forward slashes
  - Remove double slashes, leading dot and trailing slash
  */

  size_t s = 0;
  path = StrConv(path);
  while ((s = path.find(DS, s)) != string::npos) path.replace(s, 2, SS);
  if (path.compare(0, 2, LDOT) == 0) path.replace(0, 2, EMPTY);
  if (path.empty()) {
    return path;
  }
  if (path.rfind(SS) == path.length() - 1) path.pop_back();
  return path;
}

string BuildSystemGenerator::StrConv(string path) {
  /* StrConv:
  - Convert backslashes into forward slashes
  */
  if (path.empty()) {
    return path;
  }
  size_t s = 0;
  while ((s = path.find(BS, s)) != string::npos) {
    path.replace(s, 1, SS);
  }
  return path;
}

template<typename T> string BuildSystemGenerator::GetString(T data) {
  /*
  GetString:
  Concatenate elements from a set or list of strings separated by a space
  */
  if (!data.size()) {
    return string();
  }
  ostringstream stream;
  copy(data.begin(), data.end(), ostream_iterator<string>(stream, WS));
  string s = stream.str();
  if (s.rfind(WS) == s.length() - 1) s.pop_back();
  return s;
}
template string BuildSystemGenerator::GetString<set<string>>(set<string>);
template string BuildSystemGenerator::GetString<list<string>>(list<string>);

bool BuildSystemGenerator::CompareFile(const string& filename, stringstream& buffer) const {
  /*
  CompareFile:
  Compare file contents
  */
  ifstream fileStream;
  string line1, line2;

  // open the file if it exists
  fileStream.open(filename);
  if (!fileStream.is_open()) {
    return false;
  }

  // file is empty
  if (fileStream.peek() == ifstream::traits_type::eof()) {
    fileStream.close();
    return false;
  }

  // iterate over file and buffer lines
  while (getline(fileStream, line1), getline(buffer, line2)) {
    if (line1 != line2) {
      // ignore commented lines
      if ((!line1.empty() && line1.at(0) == '#') && (!line2.empty() && line2.at(0) == '#')) continue;
      // a difference was found
      break;
    }
  }

  // check if both streams where fully scanned
  bool done = fileStream.eof() && buffer.eof() ? true : false;

  // rewind stream buffer
  buffer.clear();
  buffer.seekg(0);

  // return true if contents are identical
  fileStream.close();
  return done;
}
