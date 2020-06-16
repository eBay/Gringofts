/************************************************************************
Copyright 2019-2020 eBay Inc.
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

#include "StreamingService.h"

#include "../util/TlsUtil.h"

namespace gringofts {
namespace raft {

StreamingService::StreamingService(const INIReader &reader,
                                   const RaftInterface &raftImpl)
    : mRaftImpl(raftImpl),
      mStreamingServiceGetEntriesInvokingNumber(getCounter("streaming_service_get_entries_invoking_number", {})) {
  mMaxConcurrency = reader.GetInteger("streaming", "max.concurrency", 10);

  auto port = reader.GetInteger("streaming", "grpc.port", 5678);
  std::string serverAddress("0.0.0.0:" + std::to_string(port));

  /// reuse the same TLS configuration as RaftService.
  auto tlsConfOpt = TlsUtil::parseTlsConf(reader, "raft.tls");

  grpc::ServerBuilder serverBuilder;
  serverBuilder.AddListeningPort(serverAddress, TlsUtil::buildServerCredentials(tlsConfOpt));
  serverBuilder.RegisterService(this);

  mServer = serverBuilder.BuildAndStart();
  SPDLOG_INFO("Streaming Service listening on {} ", serverAddress);
}

StreamingService::~StreamingService() {
  mServer->Shutdown();
}

grpc::Status StreamingService::GetMeta(grpc::ServerContext *context,
                                       const GetMeta::Request *request,
                                       GetMeta::Response *response) {
  response->set_code(0);

  /// populate meta
  auto role = resolveRole(mRaftImpl.getRaftRole());
  auto term = mRaftImpl.getCurrentTerm();
  auto lastIndex = mRaftImpl.getLastLogIndex();
  auto commitIndex = mRaftImpl.getCommitIndex();
  auto leaderHint = mRaftImpl.getLeaderHint();

  response->set_role(role);
  response->set_term(term);
  response->set_last_index(lastIndex);
  response->set_commit_index(commitIndex);
  if (leaderHint) {
    assert(*leaderHint != 0);
    response->set_leader_hint(std::to_string(*leaderHint));
  }

  return grpc::Status::OK;
}

grpc::Status StreamingService::GetEntries(grpc::ServerContext *context,
                                          const GetEntries::Request *request,
                                          GetEntries::Response *response) {
  auto currentConcurrency = ++mCurrentConcurrency;
  auto status = grpc::Status::OK;

  if (currentConcurrency <= mMaxConcurrency) {
    status = getEntries(context, request, response);

    mStreamingServiceGetEntriesInvokingNumber.increase();
    response->set_code(0);
    SPDLOG_INFO("first_index={}, last_index={}, commit_index={}, "
                "start_index={}, entries_num={}",
                response->first_index(),
                response->last_index(),
                response->commit_index(),
                request->start_index(),
                response->entries_size());
  } else {
    response->set_code(1);
    response->set_message("TooManyConnections");

    SPDLOG_INFO("trigger throttling, CurrentConcurrency={}, MaxConcurrency={}",
                currentConcurrency, mMaxConcurrency);
  }

  --mCurrentConcurrency;
  return status;
}

grpc::Status StreamingService::getEntries(grpc::ServerContext *context,
                                          const GetEntries::Request *request,
                                          GetEntries::Response *response) {
  auto commitIndex = mRaftImpl.getCommitIndex();
  response->set_commit_index(commitIndex);

  response->set_first_index(mRaftImpl.getFirstLogIndex());
  response->set_last_index(mRaftImpl.getLastLogIndex());

  auto startIndex = request->start_index();
  if (startIndex > commitIndex) {
    response->set_code(1);
    response->set_message("Reject. StartIndex > CommitIndex");
    return grpc::Status::OK;
  }

  std::vector<LogEntry> entries;
  mRaftImpl.getEntries(startIndex, commitIndex - startIndex + 1, &entries);

  for (auto &entry : entries) {
    *response->add_entries() = std::move(entry);
  }

  response->set_code(0);
  response->set_message("Success");
  return grpc::Status::OK;
}

}   /// namespace raft
}   /// namespace gringofts
