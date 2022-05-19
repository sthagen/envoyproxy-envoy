#pragma once

#include <memory>

#include "source/common/http/http1/parser.h"

namespace Envoy {
namespace Http {
namespace Http1 {

class LegacyHttpParserImpl : public Parser {
public:
  LegacyHttpParserImpl(MessageType type, ParserCallbacks* data);
  ~LegacyHttpParserImpl() override;

  // Http1::Parser
  size_t execute(const char* slice, int len) override;
  void resume() override;
  ParserStatus pause() override;
  ParserStatus getStatus() override;
  uint16_t statusCode() const override;
  int httpMajor() const override;
  int httpMinor() const override;
  absl::optional<uint64_t> contentLength() const override;
  bool isChunked() const override;
  absl::string_view methodName() const override;
  absl::string_view errorMessage() const override;
  int hasTransferEncoding() const override;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace Http1
} // namespace Http
} // namespace Envoy
