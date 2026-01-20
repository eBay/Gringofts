/**
* Copyright (c) 2023 eBay Software Foundation. All rights reserved.
*/

#pragma once

#include "../util/Signal.h"
#include "../util/ClusterInfo.h"

namespace gringofts {
namespace forward {

class ForwardReconfigureSignal : public Signal {
 public:
  explicit ForwardReconfigureSignal(const ClusterInfo &clusterConfiguration) :
           mClusterConfiguration(clusterConfiguration) {}
  ClusterInfo getClusterConfiguration() const { return mClusterConfiguration; }

 private:
  ClusterInfo mClusterConfiguration;
};

}  /// namespace forward
}  /// namespace gringofts
