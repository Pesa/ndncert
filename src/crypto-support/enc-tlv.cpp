/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2017-2019, Regents of the University of California.
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

#include "enc-tlv.hpp"
#include "crypto-helper.hpp"
#include <ndn-cxx/util/random.hpp>
#include <ndn-cxx/security/transform/stream-sink.hpp>
#include <ndn-cxx/encoding/buffer-stream.hpp>
#include <ndn-cxx/security/transform/buffer-source.hpp>
#include <ndn-cxx/security/transform/block-cipher.hpp>

namespace ndn {
namespace ndncert {

const size_t DEFAULT_IV_SIZE = 16;

Block
genEncBlock(uint32_t tlv_type, const uint8_t* key, size_t keyLen, const uint8_t* payload, size_t payloadSize)
{
  Buffer iv;
  iv.resize(DEFAULT_IV_SIZE);
  random::generateSecureBytes(iv.data(), iv.size());

  OBufferStream os;
  security::transform::bufferSource(payload, payloadSize)
    >> security::transform::blockCipher(BlockCipherAlgorithm::AES_CBC,
                                        CipherOperator::ENCRYPT,
                                        key, keyLen, iv.data(), iv.size())
    >> security::transform::streamSink(os);
    auto encryptedPayload = *os.buf();

  // create the content block
  auto content = makeEmptyBlock(tlv_type);
  content.push_back(makeBinaryBlock(ENCRYPTED_PAYLOAD, encryptedPayload.data(), encryptedPayload.size()));
  content.push_back(makeBinaryBlock(INITIAL_VECTOR, iv.data(), iv.size()));
  content.encode();
  return content;
}

Buffer
parseEncBlock(const uint8_t* key, size_t keyLen, const Block& block)
{
  block.parse();
  Buffer iv(block.get(INITIAL_VECTOR).value(),
            block.get(INITIAL_VECTOR).value_size());
  Buffer encryptedPayload(block.get(ENCRYPTED_PAYLOAD).value(),
                          block.get(ENCRYPTED_PAYLOAD).value_size());

  OBufferStream os;
  security::transform::bufferSource(encryptedPayload.data(), encryptedPayload.size())
    >> security::transform::blockCipher(BlockCipherAlgorithm::AES_CBC,
                                        CipherOperator::DECRYPT,
                                        key, keyLen, iv.data(), iv.size())
    >> security::transform::streamSink(os);

  auto payload = *os.buf();
  return payload;
}

} // namespace ndncert
} // namespace ndn
