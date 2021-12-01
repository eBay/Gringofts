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
#include "LogReader.h"

#include <absl/strings/match.h>
#include <absl/strings/str_split.h>
#include <spdlog/spdlog.h>
#include "../../infra/util/TimeUtil.h"


namespace gringofts::app::sync {
void LogReaderImpl::connect() {
  grpc::ChannelArguments channelArgs;
  channelArgs.SetMaxReceiveMessageSize(40 << 20);
  std::shared_ptr<Channel> channelPtr;
  if (mTlsConfOpt) {
    /// channel
    channelPtr = grpc::CreateCustomChannel(
        mEndPoint, TlsUtil::buildChannelCredentials(mTlsConfOpt), channelArgs);
  } else {
    /// channel
    channelPtr = grpc::CreateCustomChannel(mEndPoint, grpc::InsecureChannelCredentials(), channelArgs);
  }
  /// stub
  mClient = std::move(Streaming::NewStub(channelPtr));
  SPDLOG_INFO("RaftLogReader Created On {}", mEndPoint);
}

FetchStatus LogReaderImpl::fetch(uint64_t startIndex, std::vector<LogEntry> *logs) {
  GetEntries_Request request;
  request.set_start_index(startIndex);
  GetEntries_Response response;
  grpc::ClientContext context;
  /// Add deadline for client request
  std::chrono::time_point deadline = std::chrono::system_clock::now() + std::chrono::seconds(10);
  context.set_deadline(deadline);
  const auto startTimeVal = TimeUtil::currentTimeInNanos();
  grpc::Status status = mClient->GetEntries(&context, request, &response);
  auto timeCost = static_cast<double>(TimeUtil::currentTimeInNanos() - startTimeVal) / 1000000.0;
  if (timeCost > 1000) {
    SPDLOG_INFO("Grpc Call Cost {} ms", timeCost);
  }

  if (!status.ok()) {
    SPDLOG_WARN("Fetching Data Host {} From StartIndex {} Failed ErrorCode={}, ErrorMessage={}, Reply:{}",
                this->mEndPoint, startIndex, status.error_code(), status.error_message(), response.DebugString());
    connect();
    SPDLOG_ERROR("Reconnect to {}", mEndPoint);
    return ERROR;
  }

  /// In case the leader just started, mlog init haven't finished, wait for 1s, then retry again.
  if (response.first_index() == 0) {
    SPDLOG_INFO("Leader is in init procedure, wait for a second.");
    sleep(1);
    return OK;
  }

  assert(startIndex >= response.first_index());
  const auto &entries = response.entries();
  if (entries.empty()) {
    /// This branch shows that no data can be fetched any more currently,
    /// so sleep for a while wait for the new traffic to save CPU.
    SPDLOG_INFO("Can not fetch any data, entries is empty");
    return NODATA;
  }

  /// Lock persistQueue and Check LastIndex
  SPDLOG_INFO("Vec Entries StartIndex {} Entries Size {}",
              entries[0].index(), entries.size());

  logs->reserve(entries.size());
  /// avoid copy from response
  for (const auto &entry : response.entries()) {
    logs->push_back(entry);
  }
  return OK;
}
}  // namespace gringofts::app::sync
