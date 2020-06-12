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

#ifndef SRC_INFRA_MONITOR_MONITORABLE_H_
#define SRC_INFRA_MONITOR_MONITORABLE_H_

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../util/Util.h"

namespace gringofts {

struct MetricTag {
  using LabelType = std::map<std::string, std::string>;
  using FuncType = std::function<double()>;

  MetricTag(const std::string &name_, FuncType getFunctor)
      : name(name_), collectFunction(getFunctor) {}
  MetricTag(const std::string &name_, FuncType getFunctor, const LabelType &labels_)
      : name(name_), labels(labels_), collectFunction(getFunctor) {}

  std::string name;  // the name for the metrics
  LabelType labels;  // the labels
  FuncType collectFunction;  // function to get
};

class Monitorable {
 public:
  virtual ~Monitorable() = default;
  virtual double getValue(const MetricTag &tag) {
    return tag.collectFunction();
  }
  const std::vector<MetricTag> &monitorTags() const {
    return mTags;
  }
 protected:
  std::vector<MetricTag> mTags;
};

}  /// namespace gringofts

#endif  // SRC_INFRA_MONITOR_MONITORABLE_H_
