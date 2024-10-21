/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017-2024, Regents of the University of California.
 *
 * This file is part of ndncert, a certificate management system based on NDN.
 *
 * ndncert is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * ndncert is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received copies of the GNU General Public License along with
 * ndncert, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 *
 * See AUTHORS.md for complete list of ndncert authors and contributors.
 */

#include "detail/ca-configuration.hpp"
#include "detail/profile-storage.hpp"
#include "detail/info-encoder.hpp"

#include "tests/boost-test.hpp"

namespace ndncert::tests {

BOOST_AUTO_TEST_SUITE(TestConfig)

BOOST_AUTO_TEST_CASE(CaConfigFile)
{
  ca::CaConfig config;
  config.load("tests/unit-tests/config-files/config-ca-1");
  BOOST_CHECK_EQUAL(config.caProfile.caPrefix, "/ndn");
  BOOST_CHECK_EQUAL(config.caProfile.forwardingHint, "/repo");
  BOOST_CHECK_EQUAL(config.caProfile.caInfo, "ndn testbed ca");
  BOOST_CHECK_EQUAL(config.caProfile.maxValidityPeriod, time::seconds(864000));
  BOOST_CHECK_EQUAL(*config.caProfile.maxSuffixLength, 3);
  BOOST_CHECK_EQUAL(config.caProfile.probeParameterKeys.size(), 1);
  BOOST_CHECK_EQUAL(config.caProfile.probeParameterKeys.front(), "full name");
  BOOST_CHECK_EQUAL(config.caProfile.supportedChallenges.size(), 1);
  BOOST_CHECK_EQUAL(config.caProfile.supportedChallenges.front(), "pin");

  config.load("tests/unit-tests/config-files/config-ca-2");
  BOOST_CHECK_EQUAL(config.caProfile.caPrefix, "/ndn");
  BOOST_CHECK_EQUAL(config.caProfile.forwardingHint, "/ndn/CA");
  BOOST_CHECK_EQUAL(config.caProfile.caInfo, "missing max validity period, max suffix length, and probe");
  BOOST_CHECK_EQUAL(config.caProfile.maxValidityPeriod, time::seconds(86400));
  BOOST_CHECK(!config.caProfile.maxSuffixLength.has_value());
  BOOST_CHECK_EQUAL(config.caProfile.probeParameterKeys.size(), 0);
  BOOST_CHECK_EQUAL(config.caProfile.supportedChallenges.size(), 1);
  BOOST_CHECK_EQUAL(config.caProfile.supportedChallenges.front(), "pin");

  config.load("tests/unit-tests/config-files/config-ca-5");
  BOOST_CHECK_EQUAL(config.redirection[0].first->getName(),
                    "/ndn/edu/ucla/KEY/m%08%98%C2xNZ%13/self/v=1646441513929");
  BOOST_CHECK_EQUAL(config.nameAssignmentFuncs.size(), 3);
  BOOST_CHECK_EQUAL(config.nameAssignmentFuncs[0]->m_nameFormat[0], "group");
  BOOST_CHECK_EQUAL(config.nameAssignmentFuncs[0]->m_nameFormat[1], "email");
  std::multimap<std::string, std::string> params;
  params.emplace("email", "1@1.edu");
  params.emplace("group", "irl");
  params.emplace("name", "ndncert");
  std::vector<Name> names;
  for (auto& assignment : config.nameAssignmentFuncs) {
    auto results = assignment->assignName(params);
    BOOST_CHECK_EQUAL(results.size(), 1);
    names.insert(names.end(), results.begin(), results.end());
  }
  BOOST_CHECK_EQUAL(names.size(), 3);
  BOOST_CHECK_EQUAL(names[0], Name("/irl/1@1.edu"));
  BOOST_CHECK_EQUAL(names[1], Name("/irl/ndncert"));
  BOOST_CHECK_EQUAL(names[2].size(), 1);
}

BOOST_AUTO_TEST_CASE(CaConfigFileWithErrors)
{
  ca::CaConfig config;
  // nonexistent file
  BOOST_CHECK_THROW(config.load("tests/unit-tests/config-files/Nonexist"), std::runtime_error);
  // missing challenge
  BOOST_CHECK_THROW(config.load("tests/unit-tests/config-files/config-ca-3"), std::runtime_error);
  // unsupported challenge
  BOOST_CHECK_THROW(config.load("tests/unit-tests/config-files/config-ca-4"), std::runtime_error);
  // unsupported name assignment
  BOOST_CHECK_THROW(config.load("tests/unit-tests/config-files/config-ca-6"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(ProfileStorageConfigFile)
{
  requester::ProfileStorage profileStorage;
  profileStorage.load("tests/unit-tests/config-files/config-client-1");
  BOOST_CHECK_EQUAL(profileStorage.getKnownProfiles().size(), 2);

  auto& profile1 = profileStorage.getKnownProfiles().front();
  BOOST_CHECK_EQUAL(profile1.caPrefix, "/ndn/edu/ucla");
  BOOST_CHECK_EQUAL(profile1.caInfo, "ndn testbed ca");
  BOOST_CHECK_EQUAL(profile1.maxValidityPeriod, time::seconds(864000));
  BOOST_CHECK_EQUAL(*profile1.maxSuffixLength, 3);
  BOOST_CHECK_EQUAL(profile1.probeParameterKeys.size(), 1);
  BOOST_CHECK_EQUAL(profile1.probeParameterKeys.front(), "email");
  BOOST_CHECK_EQUAL(profile1.cert->getName(),
                    "/ndn/site1/KEY/B%B2%60F%07%88%1C2/self/v=1646441889090");

  auto& profile2 = profileStorage.getKnownProfiles().back();
  BOOST_CHECK_EQUAL(profile2.caPrefix, "/ndn/edu/ucla/zhiyi");
  BOOST_CHECK_EQUAL(profile2.caInfo, "");
  BOOST_CHECK_EQUAL(profile2.maxValidityPeriod, time::seconds(86400));
  BOOST_CHECK(!profile2.maxSuffixLength);
  BOOST_CHECK_EQUAL(profile2.probeParameterKeys.size(), 0);
  BOOST_CHECK_EQUAL(profile2.cert->getName(),
                    "/ndn/site1/KEY/B%B2%60F%07%88%1C2/self/v=1646441889090");
}

BOOST_AUTO_TEST_CASE(ProfileStorageWithErrors)
{
  requester::ProfileStorage profileStorage;
  // nonexistent file
  BOOST_CHECK_THROW(profileStorage.load("tests/unit-tests/config-files/Nonexist"), std::runtime_error);
  // missing certificate
  BOOST_CHECK_THROW(profileStorage.load("tests/unit-tests/config-files/config-client-2"), std::runtime_error);
  // missing ca prefix
  BOOST_CHECK_THROW(profileStorage.load("tests/unit-tests/config-files/config-client-3"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(ProfileStorageAddAndRemoveProfile)
{
  requester::ProfileStorage profileStorage;
  profileStorage.load("tests/unit-tests/config-files/config-client-1");

  CaProfile item;
  item.caPrefix = Name("/test");
  item.caInfo = "test";

  profileStorage.addCaProfile(item);
  BOOST_CHECK_EQUAL(profileStorage.getKnownProfiles().size(), 3);
  auto lastItem = profileStorage.getKnownProfiles().back();
  BOOST_CHECK_EQUAL(lastItem.caPrefix, "/test");

  profileStorage.removeCaProfile(Name("/test"));
  BOOST_CHECK_EQUAL(profileStorage.getKnownProfiles().size(), 2);
  lastItem = profileStorage.getKnownProfiles().back();
  BOOST_CHECK_EQUAL(lastItem.caPrefix, "/ndn/edu/ucla/zhiyi");
}

BOOST_AUTO_TEST_SUITE_END() // TestConfig

} // namespace ndncert::tests
