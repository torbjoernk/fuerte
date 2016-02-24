////////////////////////////////////////////////////////////////////////////////
/// @brief C++ Library to interface to Arangodb.
///
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
/// @author Copyright 2016, ArangoDB GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////
#include <cstring>
#include <velocypack/Slice.h>
#include <velocypack/Parser.h>
#include <velocypack/Builder.h>
#include <velocypack/Sink.h>
#include <velocypack/Dumper.h>

#include <curlpp/Infos.hpp>

#include "arangodbcpp/Connection.h"

namespace arangodb {

namespace dbinterface {

std::string Connection::json(const VPack& v, bool bSort) {
  using arangodb::velocypack::Slice;
  using arangodb::velocypack::Dumper;
  using arangodb::velocypack::StringSink;
  using arangodb::velocypack::Options;
  Slice slice{v->data()};
  std::string tmp;
  StringSink sink(&tmp);
  Options opts;
  opts.sortAttributeNames = bSort;
  Dumper::dump(slice, &sink, &opts);
  return tmp;
}

//
//	Get string attribute if available
//
std::string Connection::strValue(const VPack res, std::string attrib) {
  using arangodb::velocypack::Slice;
  using arangodb::velocypack::ValueType;
  Slice slice{res->data()};
  std::string ret;
  slice = slice.get(attrib);
  if (slice.type() == ValueType::String) {
    ret = slice.toString();
    ret = ret.substr(1, ret.length() - 2);
  }
  return ret;
}

void Connection::setPostField(const VPack data) {
  std::string field{"{}"};
  if (data.get() != nullptr) {
    field = json(data, false);
  }
  setPostField(field);
}

//
//
//
void Connection::setBuffer() {
  setBuffer(this, &Connection::WriteMemoryCallback);
}

//
//
//
void Connection::setJsonContent(HttpHeaderList& headers) {
  headers.push_back("Content-Type: application/json");
}

//
// Clears the buffer that holds received data and
// any error messages
//
// Configures whether the next operation will be done
// synchronously or asyncronously
//
// IMPORTANT
// This should be the last configuration item to be set
//
void Connection::setReady(bool bAsync) {
  _buf.clear();
  _flgs = 0;
  if (bAsync) {
    _flgs = F_Multi | F_Running;
    _async.add(&_request);
  }
}

void Connection::reset() {
  _request.reset();
  _flgs = F_Clear;
}

void Connection::run() {
  if (_flgs & F_Multi) {
    asyncRun();
    if (_flgs & F_Running) {
      return;
    }
  } else {
    syncRun();
  }
  if (_buf.empty()) {
    httpResponse();
  }
  if (_flgs & F_Multi) {
    _async.remove(&_request);
  }
}

//
// Synchronous operation which will complete before returning
//
void Connection::syncRun() {
  try {
    _request.perform();
  } catch (curlpp::LogicError& e) {
    errFound(e.what(), F_LogicError);
    return;
  } catch (curlpp::LibcurlRuntimeError& e) {
    errFound(e.what());
    return;
  } catch (curlpp::RuntimeError& e) {
    errFound(e.what());
    return;
  }
}

//
// Asynchronous operation which may need to be run multiple times
// before completing
//
void Connection::asyncRun() {
  try {
    {
      int nLeft = 1;
      if (!_async.perform(&nLeft)) {
        return;
      }
      if (!nLeft) {
        _flgs ^= F_Running;
        return;
      }
    }
    {
      struct timeval timeout;
      int rc;  // select() return code
      fd_set fdread;
      fd_set fdwrite;
      fd_set fdexcep;
      int maxfd;
      FD_ZERO(&fdread);
      FD_ZERO(&fdwrite);
      FD_ZERO(&fdexcep);
      // set a suitable timeout to play around with
      timeout.tv_sec = 1;
      timeout.tv_usec = 0;
      // get file descriptors from the transfers
      _async.fdset(&fdread, &fdwrite, &fdexcep, &maxfd);
      rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
      if (rc == -1) {
        errFound("Asynchronous select error");
      }
    }
  } catch (curlpp::LogicError& e) {
    _flgs ^= F_Running;
    errFound(e.what(), F_LogicError);
    return;
  } catch (curlpp::LibcurlRuntimeError& e) {
    _flgs ^= F_Running;
    errFound(e.what());
    return;
  } catch (curlpp::RuntimeError& e) {
    _flgs ^= F_Running;
    errFound(e.what());
    return;
  }
}

void Connection::httpResponse() {
  long res = httpResponseCode();
  if (!res && (_flgs & F_Multi)) {
    typedef curlpp::Multi::Msgs M_Msgs;
    M_Msgs msgs = _async.info();
    CURLcode code = msgs.front().second.code;
    std::string msg{curl_easy_strerror(code)};
    errFound(msg);
    return;
  }
  {
    // Create a JSon response
    std::ostringstream os;
    os << "{\"result\":true,\"error\":false,\"code\":\"" << res << "\"}";
    setBuffer(os.str());
  }
}

void Connection::setPostField(const std::string& inp) {
  setOpt(curlpp::options::PostFields(inp));
  setOpt(curlpp::options::PostFieldSize(inp.length()));
}

//
// Sets the curlpp callback function that receives the data returned
// from the operation performed
//
void Connection::setBuffer(size_t (*f)(char* p, size_t sz, size_t m)) {
  curlpp::types::WriteFunctionFunctor fnc(f);
  setOpt(curlpp::options::WriteFunction(fnc));
}

//
//	Curlpp callback function that receives the data returned
//	from the operation performed into the default write buffer
//
size_t Connection::WriteMemoryCallback(char* ptr, size_t size, size_t nmemb) {
  size_t realsize = size * nmemb;
  if (realsize != 0) {
    size_t offset = _buf.size();
    _buf.resize(offset + realsize);
    memcpy(&_buf[offset], ptr, realsize);
  }
  return realsize;
}

Connection::VPack Connection::notProcessed() const {
  using arangodb::velocypack::Builder;
  using arangodb::velocypack::Value;
  using arangodb::velocypack::ValueType;
  using arangodb::velocypack::Options;
  Builder b;
  b.add(Value(ValueType::Object));  // Start building an object
  b.add("Result", Value("Not processed"));
  b.close();  // Finish the object
  return b.steal();
}

Connection::VPack Connection::noHost() const {
  using arangodb::velocypack::Builder;
  using arangodb::velocypack::Value;
  using arangodb::velocypack::ValueType;
  using arangodb::velocypack::Options;
  Builder b;
  b.add(Value(ValueType::Object));  // Start building an object
  b.add("Error", Value("No host url"));
  b.close();  // Finish the object
  return b.steal();
}

//
// Converts JSon held in the default write buffer
// to a shared velocypack buffer
//
Connection::VPack Connection::fromJSon(bool bSorted) const {
  if (!_buf.empty()) {
    using arangodb::velocypack::Builder;
    using arangodb::velocypack::Parser;
    using arangodb::velocypack::Options;
    Options options;
    options.sortAttributeNames = bSorted;
    Parser parser{&options};
    parser.parse(&_buf[0], _buf.size());
    std::shared_ptr<Builder> vp{parser.steal()};
    return vp->buffer();
  }
  return VPack{new VBuffer()};
}

Connection::Connection() {}

Connection::~Connection() {}
}
}
