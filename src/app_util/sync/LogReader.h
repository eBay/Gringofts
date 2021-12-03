/************************************************************************
Copyright 2019-2021 eBay Inc.
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    https://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
**************************************************************************/

#ifndef SRC_APP_UTIL_SYNC_LOGREADER_H_
#define SRC_APP_UTIL_SYNC_LOGREADER_H_

#include <vector>
#include <optional>
#include <memory>

#include "../../infra/util/TlsUtil.h"
#include "../../infra/raft/generated/streaming.grpc.pb.h"

namespace gringofts::app::sync {

using raft::Streaming;
using raft::GetEntries_Request;
using raft::GetEntries_Response;
using raft::LogEntry;
using grpc::Channel;

enum FetchStatus {
  OK = 0,
  ERROR = 1,
  NODATA = 2
};

struct LogReader {
  virtual ~LogReader() = default;

  virtual void connect() = 0;

  virtual FetchStatus fetch(uint64_t startIndex, std::vector<LogEntry> *logs) = 0;
};

class LogReaderImpl : public LogReader {
 public:
  LogReaderImpl(std::string endPoint, std::optional<TlsConf> tlsConfOpt) :
      mEndPoint(std::move(endPoint)), mTlsConfOpt(std::move(tlsConfOpt)) {}

  void connect() override;

  FetchStatus fetch(uint64_t startIndex, std::vector<LogEntry> *logs) override;

 private:
  std::unique_ptr<Streaming::Stub> mClient;
  std::string mEndPoint;
  std::optional<TlsConf> mTlsConfOpt;
};

/**
 * for test perspective, so that we can replace ReaderFactory
 * to Factory of mocked Reader to test
 */
struct ReaderFactory {
  virtual ~ReaderFactory() = default;
  virtual std::unique_ptr<LogReader> create(std::string endPoint, std::optional<TlsConf> tlsConfOpt) = 0;
};

template<typename T>
struct ReaderFactory_t : public ReaderFactory {
  std::unique_ptr<LogReader> create(std::string endPoint, std::optional<TlsConf> tlsConfOpt) override {
    return std::make_unique<T>(std::move(endPoint), std::move(tlsConfOpt));
  };
};

using LogReaderFactoryImp = ReaderFactory_t<LogReaderImpl>;

}  // namespace gringofts::app::sync
#endif  // SRC_APP_UTIL_SYNC_LOGREADER_H_

