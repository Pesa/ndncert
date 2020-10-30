/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2017-2020, Regents of the University of California.
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

#include <configuration.hpp>
#include <detail/info-encoder.hpp>
#include <detail/error-encoder.hpp>
#include <detail/probe-encoder.hpp>
#include <detail/new-renew-revoke-encoder.hpp>
#include <detail/challenge-encoder.hpp>
#include <identity-management-fixture.hpp>
#include "test-common.hpp"

namespace ndn {
namespace ndncert {
namespace tests {

BOOST_FIXTURE_TEST_SUITE(TestProtocolEncoding, IdentityManagementTimeFixture)
BOOST_AUTO_TEST_CASE(InfoEncoding)
{
    ca::CaConfig config;
    config.load("tests/unit-tests/config-files/config-ca-1");

    requester::ProfileStorage caCache;
    caCache.load("tests/unit-tests/config-files/config-client-1");
    auto& cert = caCache.m_caItems.front().m_cert;

    auto b = InfoEncoder::encodeDataContent(config.m_caItem, *cert);
    auto item = InfoEncoder::decodeDataContent(b);

    BOOST_CHECK_EQUAL(*item.m_cert, *cert);
    BOOST_CHECK_EQUAL(item.m_caInfo, config.m_caItem.m_caInfo);
    BOOST_CHECK_EQUAL(item.m_caPrefix, config.m_caItem.m_caPrefix);
    BOOST_CHECK_EQUAL(item.m_probeParameterKeys.size(), config.m_caItem.m_probeParameterKeys.size());
    for (auto it1 = item.m_probeParameterKeys.begin(), it2 = config.m_caItem.m_probeParameterKeys.begin();
    it1 != item.m_probeParameterKeys.end() && it2 != config.m_caItem.m_probeParameterKeys.end(); it1 ++, it2 ++) {
        BOOST_CHECK_EQUAL(*it1, *it2);
    }
    BOOST_CHECK_EQUAL(item.m_maxValidityPeriod, config.m_caItem.m_maxValidityPeriod);
}

BOOST_AUTO_TEST_CASE(ErrorEncoding)
{
    std::string msg = "Just to test";
    auto b = ErrorEncoder::encodeDataContent(ErrorCode::NAME_NOT_ALLOWED, msg);
    auto item = ErrorEncoder::decodefromDataContent(b);
    BOOST_CHECK_EQUAL(std::get<0>(item),  ErrorCode::NAME_NOT_ALLOWED);
    BOOST_CHECK_EQUAL(std::get<1>(item),  msg);
}

BOOST_AUTO_TEST_CASE(ProbeEncodingAppParam)
{
    std::vector<std::tuple<std::string, std::string>> parameters;
    parameters.emplace_back("key1", "value1");
    parameters.emplace_back("key2", "value2");
    auto appParam = ProbeEncoder::encodeApplicationParameters(parameters);
    auto param1 = ProbeEncoder::decodeApplicationParameters(appParam);
    BOOST_CHECK_EQUAL(parameters.size(),  param1.size());
    BOOST_CHECK_EQUAL(std::get<0>(parameters[0]),  std::get<0>(param1[0]));
    BOOST_CHECK_EQUAL(std::get<1>(parameters[0]),  std::get<1>(param1[0]));
    BOOST_CHECK_EQUAL(std::get<0>(parameters[1]),  std::get<0>(param1[1]));
    BOOST_CHECK_EQUAL(std::get<1>(parameters[1]),  std::get<1>(param1[1]));
}

BOOST_AUTO_TEST_CASE(ProbeEncodingData)
{
    ca::CaConfig config;
    config.load("tests/unit-tests/config-files/config-ca-5");
    std::vector<Name> names;
    names.emplace_back("/ndn/1");
    names.emplace_back("/ndn/2");
    auto b = ProbeEncoder::encodeDataContent(names, 2, config.m_redirection);
    std::vector<std::pair<Name, int>> retNames;
    std::vector<Name> redirection;
    ProbeEncoder::decodeDataContent(b, retNames, redirection);
    BOOST_CHECK_EQUAL(retNames.size(), names.size());
    auto it1 = retNames.begin(); auto it2 = names.begin();
    for (; it1 != retNames.end() && it2 != names.end(); it1 ++, it2 ++) {
        BOOST_CHECK_EQUAL(it1->first, *it2);
        BOOST_CHECK_EQUAL(it1->second, 2);
    }
    BOOST_CHECK_EQUAL(redirection.size(), config.m_redirection->size());
    auto it3 = redirection.begin(); auto it4 = config.m_redirection->begin();
    for (; it3 != redirection.end() && it4 != config.m_redirection->end(); it3 ++, it4 ++) {
        BOOST_CHECK_EQUAL(*it3, (*it4)->getFullName());
    }
}

BOOST_AUTO_TEST_CASE(NewRevokeEncodingParam)
{
    requester::ProfileStorage caCache;
    caCache.load("tests/unit-tests/config-files/config-client-1");
    auto& certRequest = caCache.m_caItems.front().m_cert;
    std::vector<uint8_t> pub = ECDHState().getSelfPubKey();
    auto b = NewRenewRevokeEncoder::encodeApplicationParameters(RequestType::REVOKE, pub, *certRequest);
    std::vector<uint8_t> returnedPub;
    std::shared_ptr<security::Certificate> returnedCert;
    NewRenewRevokeEncoder::decodeApplicationParameters(b, RequestType::REVOKE, returnedPub, returnedCert);

    BOOST_CHECK_EQUAL(returnedPub.size(), pub.size());
    for (auto it1 = returnedPub.begin(), it2 = pub.begin();
         it1 != returnedPub.end() && it2 != pub.end(); it1 ++, it2 ++) {
        BOOST_CHECK_EQUAL(*it1, *it2);
    }
    BOOST_CHECK_EQUAL(*returnedCert, *certRequest);
}

BOOST_AUTO_TEST_CASE(NewRevokeEncodingData)
{
    std::vector<uint8_t> pub = ECDHState().getSelfPubKey();
    std::array<uint8_t, 32> salt = {101};
    RequestId id = {102};
    std::list<std::string> list;
    list.emplace_back("abc");
    list.emplace_back("def");
    auto b = NewRenewRevokeEncoder::encodeDataContent(pub, salt, id, Status::BEFORE_CHALLENGE, list);
    std::vector<uint8_t> returnedPub;
    std::array<uint8_t, 32> returnedSalt;
    RequestId returnedId;
    Status s;
    auto retlist = NewRenewRevokeEncoder::decodeDataContent(b, returnedPub, returnedSalt, returnedId, s);
    BOOST_CHECK_EQUAL(returnedPub.size(), pub.size());
    for (auto it1 = returnedPub.begin(), it2 = pub.begin();
         it1 != returnedPub.end() && it2 != pub.end(); it1 ++, it2 ++) {
        BOOST_CHECK_EQUAL(*it1, *it2);
    }
    BOOST_CHECK_EQUAL(returnedSalt.size(), salt.size());
    for (auto it1 = returnedSalt.begin(), it2 = salt.begin();
         it1 != returnedSalt.end() && it2 != salt.end(); it1 ++, it2 ++) {
        BOOST_CHECK_EQUAL(*it1, *it2);
    }
    BOOST_CHECK_EQUAL(returnedId.size(), id.size());
    for (auto it1 = returnedId.begin(), it2 = id.begin();
         it1 != returnedId.end() && it2 != id.end(); it1 ++, it2 ++) {
        BOOST_CHECK_EQUAL(*it1, *it2);
    }
    BOOST_CHECK_EQUAL(static_cast<size_t>(s), static_cast<size_t>(Status::BEFORE_CHALLENGE));
}

BOOST_AUTO_TEST_CASE(ChallengeEncoding)
{
    time::system_clock::TimePoint t = time::system_clock::now();
    requester::ProfileStorage caCache;
    caCache.load("tests/unit-tests/config-files/config-client-1");
    security::Certificate certRequest = *caCache.m_caItems.front().m_cert;
    RequestId id = {102};
    ca::RequestState state(Name("/ndn/akdnsla"), id, RequestType::NEW, Status::PENDING,
                           certRequest, "hahaha", "Just a test", t, 3, time::seconds(321), JsonSection(),
                           Block(), 0);
    auto b = ChallengeEncoder::encodeDataContent(state);
    b.push_back(makeNestedBlock(tlv::IssuedCertName, Name("/ndn/akdnsla/a/b/c")));

    requester::RequestContext context(m_keyChain, caCache.m_caItems.front(), RequestType::NEW);
    ChallengeEncoder::decodeDataContent(b, context);

    BOOST_CHECK_EQUAL(static_cast<size_t>(context.m_status), static_cast<size_t>(Status::PENDING));
    BOOST_CHECK_EQUAL(context.m_challengeStatus, "Just a test");
    BOOST_CHECK_EQUAL(context.m_remainingTries, 3);
    BOOST_ASSERT(context.m_freshBefore > time::system_clock::now() + time::seconds(321) - time::milliseconds(100));
    BOOST_ASSERT(context.m_freshBefore < time::system_clock::now() + time::seconds(321) + time::milliseconds(100));
    BOOST_CHECK_EQUAL(context.m_issuedCertName, "/ndn/akdnsla/a/b/c");
}



BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace ndncert
} // namespace ndn


