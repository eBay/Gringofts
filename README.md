# Table of Contents
- [Introduction](#introduction)
- [Features](#features)
- [Get Started](#get-started)
- [Core Developers](#core-developers)
- [License Information](#license-information)
- [Use of 3rd Party Code](#use-of-3rd-party-code)

# Introduction
Goblin is an infrastructure that built on top of [Gringofts](https://github.com/eBay/Gringofts). It aims to provide a coordination service to facilitate distributed applications with high availability, scalability and performance.

Some typical scenarios that can be powered by Goblin includes (but not limited to):
- A kv store with high availability
- A way to notify downstream on data changes
- A central place for service registry
- A distributed lock for cooperation
- ...

# Features
- Support basic kv operations (put/get/delete) <br/>
- Support CAS (compare-and-swap) operation <br/>
- Support mini-transaction <br/>
- Support key expiration mechanism
- Support Publish/Subscribe <br/>
(new features are developing !) <br/>

# Get Started

## Supported Platforms
Currently the only recommended platform is Ubuntu 16.04. We plan to support more platforms in the near future.

## Set up Source Dependencies
```
git clone https://github.com/eBay/Goblin
cd Goblin
cd .git/hooks && ln -s ../../hooks/prepare-commit-msg prepare-commit-msg
cd server
bash ./scripts/addSubmodules.sh
sudo bash ./scripts/setupDevEnvironment.sh
```

## Build
### Build directly on local OS
```
cd server
./script/build.sh
```

# Core Developers
- Qiawu (Chandler) Cai <qiacai@ebay.com>
- Zhiyuan (Liam) Zhang <zhiyuazhang@ebay.com>
- Wenbin Liu <wenbliu@ebay.com>

Please see [here](CONTRIBUTORS.md) for all contributors.

# Acknowledgements
Special thanks to [people](ACKNOWLEDGEMENTS.md) who give your support on this project.

# License Information
Copyright 2021-2022 eBay Inc.

Authors/Developers: Qiawu(Chandler) Cai

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at

https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.

# Use of 3rd Party Code
Some parts of this software include 3rd party code licensed under open source licenses (in alphabetic order):

1. OpenSSL<br/>
   URL: https://www.openssl.org/<br/>
   License: https://www.openssl.org/source/license.html<br/>
   Originally licensed under the Apache 2.0 license.

1. RocksDB<br/>
   URL: https://github.com/facebook/rocksdb<br/>
   License: https://github.com/facebook/rocksdb/blob/master/LICENSE.Apache<br/>
   Apache 2.0 license selected.

1. abseil-cpp<br/>
   URL: https://github.com/abseil/abseil-cpp<br/>
   License: https://github.com/abseil/abseil-cpp/blob/master/LICENSE<br/>
   Originally licensed under the Apache 2.0 license.

1. cpplint<br/>
   URL: https://github.com/google/styleguide<br/>
   License: https://github.com/google/styleguide/blob/gh-pages/LICENSE<br/>
   Originally licensed under the Apache 2.0 license.

1. inih<br/>
   URL: https://github.com/benhoyt/inih<br/>
   License: https://github.com/benhoyt/inih/blob/master/LICENSE.txt
   Originally licensed under the New BSD license.

1. gRPC<br/>
   URL: https://github.com/grpc/grpc<br/>
   License: https://github.com/grpc/grpc/blob/master/LICENSE<br/>
   Originally licensed under the Apache 2.0 license.

1. googletest<br/>
   URL: https://github.com/google/googletest<br/>
   License: https://github.com/google/googletest/blob/master/LICENSE<br/>
   Originally licensed under the BSD 3-Clause "New" or "Revised" license.

1. prometheus-cpp<br/>
   URL: https://github.com/jupp0r/prometheus-cpp<br/>
   License: https://github.com/jupp0r/prometheus-cpp/blob/master/LICENSE
   Originally licensed under the MIT license.

1. spdlog<br/>
   URL: https://github.com/gabime/spdlog<br/>
   License: https://github.com/gabime/spdlog/blob/master/LICENSE<br/>
   Originally licensed under the MIT license.
