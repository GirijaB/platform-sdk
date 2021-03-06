// AirMap Platform SDK
// Copyright © 2018 AirMap, Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the License);
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//   http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an AS IS BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#define BOOST_TEST_MODULE client

#include <helper.h>

#include <airmap/client.h>

#include <boost/test/included/unit_test.hpp>

#include <fstream>

BOOST_AUTO_TEST_CASE(configuration_can_be_parsed_from_valid_json) {
  std::ifstream json{test::source_dir() + "/airmapd.valid.config.json"};
  auto config = airmap::Client::load_configuration_from_json(json);
  BOOST_CHECK(config.host == "api.airmap.com");
  BOOST_CHECK(config.version == airmap::Client::Version::production);
  BOOST_CHECK(config.sso.host == "sso.airmap.io");
  BOOST_CHECK(config.sso.port == 443);
  BOOST_CHECK(config.telemetry.host == "api-udp-telemetry.airmap.com");
  BOOST_CHECK(config.telemetry.port == 16060);
  BOOST_CHECK(config.traffic.host == "mqtt-prod.airmap.io");
  BOOST_CHECK(config.traffic.port == 8883);
  BOOST_CHECK(config.credentials.api_key == "lalelu");
}

BOOST_AUTO_TEST_CASE(version_is_read_correctly_from_input_stream) {
  airmap::Client::Version version{airmap::Client::Version::production};

  {
    std::stringstream ss{"staging"};
    ss >> version;
  }
  BOOST_CHECK(version == airmap::Client::Version::staging);

  {
    std::stringstream ss{"production"};
    ss >> version;
  }
  BOOST_CHECK(version == airmap::Client::Version::production);
}

BOOST_AUTO_TEST_CASE(version_is_written_correctly_to_output_stream) {
  {
    std::stringstream ss;
    ss << airmap::Client::Version::production;
    BOOST_CHECK(ss.str() == "production");
  }
  {
    std::stringstream ss;
    ss << airmap::Client::Version::staging;
    BOOST_CHECK(ss.str() == "staging");
  }
}
