#pragma once
#ifndef ARANGO_CXX_DRIVER_COMMON_TYPES
#define ARANGO_CXX_DRIVER_COMMON_TYPES

#include <velocypack/Slice.h>
#include <velocypack/Buffer.h>
#include <velocypack/Builder.h>

#include <map>
#include <vector>
#include <string>
#include <cassert>
#include <algorithm>

namespace arangodb { namespace fuerte { inline namespace v1 {

class Request;
class Response;

using Error = std::uint32_t;
using MessageID = uint64_t; //id that identifies a vst::request
using Ticket = MessageID;

using OnSuccessCallback = std::function<void(std::unique_ptr<Request>, std::unique_ptr<Response>)>;
using OnErrorCallback = std::function<void(Error, std::unique_ptr<Request>, std::unique_ptr<Response>)>;

using VBuffer = arangodb::velocypack::Buffer<uint8_t>;
using VSlice = arangodb::velocypack::Slice;
using VBuilder = arangodb::velocypack::Builder;
using VValue = arangodb::velocypack::Value;

using mapss = std::map<std::string,std::string>;
using NetBuffer = std::string;

//move to some other place
class VpackInit {
    std::unique_ptr<arangodb::velocypack::AttributeTranslator> _translator;
  public:
    VpackInit();
};


// -----------------------------------------------------------------------------
// --SECTION--                                         enum class ErrorCondition
// -----------------------------------------------------------------------------

enum class ErrorCondition : Error {
  CouldNotConnect = 1000,
  Timeout = 1001,
  CurlError = 1002
};

// -----------------------------------------------------------------------------
// --SECTION--                                               enum class RestVerb
// -----------------------------------------------------------------------------

enum class RestVerb
{ Illegal = -1
, Delete = 0
, Get = 1
, Post = 2
, Put = 3
, Head = 4
, Patch = 5
, Options = 6
};

RestVerb to_RestVerb(std::string& val);
std::string to_string(RestVerb type);
inline RestVerb to_RestVerb(std::string const& valin) {
  std::string val(valin);
  return to_RestVerb(val);
}

// -----------------------------------------------------------------------------
// --SECTION--                                                       MessageType
// -----------------------------------------------------------------------------

enum class MessageType
{ Undefined = 0
, Request = 1
, Response = 2
, ResponseUnfinished = 3
, Authentication = 1000
};

std::string to_string(MessageType type);

// -----------------------------------------------------------------------------
// --SECTION--                                                     TransportType
// -----------------------------------------------------------------------------

enum class TransportType { Undefined = 0, Http = 1, Vst = 2 };
std::string to_string(TransportType type);

// -----------------------------------------------------------------------------
// --SECTION--                                                       ContentType
// -----------------------------------------------------------------------------

enum class ContentType { Unset, Custom, VPack, Dump, Json, Html, Text };
ContentType to_ContentType(std::string const& val);
std::string to_string(ContentType type);

// -----------------------------------------------------------------------------
// --SECTION--                                           ConnectionConfiguration
// -----------------------------------------------------------------------------

namespace detail {
  struct ConnectionConfiguration {
    ConnectionConfiguration()
      : _connType(TransportType::Vst)
      , _ssl(true)
      , _async(false)
      , _host("localhost")
      , _user("root")
      , _password("foppels")
      , _maxChunkSize(5000ul) // in bytes
      {}

    TransportType _connType; // vst or http
    bool _ssl;
    bool _async;
    std::string _host;
    std::string _port;
    std::string _user;
    std::string _password;
    std::size_t _maxChunkSize;
  };

}

}}}
#endif