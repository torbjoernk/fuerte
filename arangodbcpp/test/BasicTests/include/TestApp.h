////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2016 ArangoDB GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author John Bufton
////////////////////////////////////////////////////////////////////////////////

#ifndef TESTAPP_H
#define TESTAPP_H

#include <gtest/gtest.h>
#include <arangodbcpp/Server.h>
#include <arangodbcpp/Database.h>
#include <velocypack/Slice.h>

class TestApp {
 public:
  typedef arangodb::dbinterface::Server Server;
  typedef arangodb::dbinterface::Database DataBase;
  typedef arangodb::dbinterface::Connection ConnectionBase;
  typedef arangodb::velocypack::Slice Slice;

  TestApp(int argc, char* argv[]);
  int run();

  static const std::string& hostUrl();
  static const std::string& dbVersion();
  static uint16_t hostPort();
  static std::string string(Slice& slice);

 private:
  void init();

  int _argc;
  char** _argv;

  static std::string _ver;
  static std::string _url;
};

inline const std::string& TestApp::hostUrl() { return _url; }

inline const std::string& TestApp::dbVersion() { return _ver; }

#endif  // TESTAPP_H
