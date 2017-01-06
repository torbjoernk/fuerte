#pragma once

#include "message.h"

namespace arangodb { namespace fuerte { inline namespace v1 {

// Helper and Implementation
  std::unique_ptr<Request>
  createRequest(MessageHeader&& messageHeader
               ,mapss&& headerStrings
               ,RestVerb const& verb
               ,ContentType const& contentType
               ){

    auto request = std::unique_ptr<Request>(new Request(std::move(messageHeader),std::move(headerStrings)));
    request->header.restVerb = verb;
    if (!request->header.contentType){
      request->header.contentType = ContentType::VPack;
    }

    return request;
  }

  std::unique_ptr<Request>
  createRequest(MessageHeader const& messageHeader
               ,mapss const& headerStrings
               ,std::string const& database
               ,RestVerb const& verb
               ,ContentType const& contentType
               ){
    MessageHeader header = messageHeader;
    mapss strings = headerStrings;
    return createRequest(std::move(header), std::move(strings), database, verb, contentType);
  }

  std::unique_ptr<Request>
  createRequest(RestVerb const& verb
               ,ContentType const& contentType
               ){
    return createRequest(MessageHeader(), mapss(), verb, contentType);
  }

// For User
  std::unique_ptr<Request>
  createRequest(RestVerb verb
               ,std::string const& path
               ,mapss const& parameter
               ,std::vector<VBuffer>&& payload = std::vector<VBuffer>())
  {
    auto request = createRequest(verb, ContentType::VPack);
    request->header.path = path;
    request->header.parameter = parameter;
    request->payload = std::move(payload);
    return request;
  }

  std::unique_ptr<Request>
  createRequest(RestVerb verb
               ,std::string const& path
               ,mapss const& parameter
               ,VBuffer&& payload)
  {
    auto request = createRequest(verb, ContentType::VPack);
    request->header.path = path;
    request->header.parameter = parameter;
    request->payload.push_back(std::move(payload));
    return request;
  }

  std::unique_ptr<Request>
  createRequest(RestVerb verb
               ,std::string const& path
               ,mapss const& parameter
               ,VSlice const& payload)
  {
    auto request = createRequest(verb, ContentType::VPack);
    request->header.path = path;
    request->header.parameter = parameter;

    VBuffer buffer;
    VBuilder builder(buffer);
    builder.add(payload);
    request->payload.push_back(std::move(buffer));

    return request;
  }

}}}
