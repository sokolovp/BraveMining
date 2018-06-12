// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/fido/ctap_constants.h"
#include "device/fido/ctap_empty_authenticator_request.h"
#include "device/fido/ctap_get_assertion_request.h"
#include "device/fido/ctap_make_credential_request.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

// Leveraging example 4 of section 6.1 of the spec
// https://fidoalliance.org/specs/fido-v2.0-rd-20170927/fido-client-to-authenticator-protocol-v2.0-rd-20170927.html
TEST(CTAPRequestTest, TestConstructMakeCredentialRequestParam) {
  static constexpr uint8_t kClientDataHash[] = {
      0x68, 0x71, 0x34, 0x96, 0x82, 0x22, 0xec, 0x17, 0x20, 0x2e, 0x42,
      0x50, 0x5f, 0x8e, 0xd2, 0xb1, 0x6a, 0xe2, 0x2f, 0x16, 0xbb, 0x05,
      0xb8, 0x8c, 0x25, 0xdb, 0x9e, 0x60, 0x26, 0x45, 0xf1, 0x41};

  static constexpr uint8_t kUserId[] = {
      0x30, 0x82, 0x01, 0x93, 0x30, 0x82, 0x01, 0x38, 0xa0, 0x03, 0x02,
      0x01, 0x02, 0x30, 0x82, 0x01, 0x93, 0x30, 0x82, 0x01, 0x38, 0xa0,
      0x03, 0x02, 0x01, 0x02, 0x30, 0x82, 0x01, 0x93, 0x30, 0x82};

  static constexpr uint8_t kSerializedRequest[] = {
      // clang-format off
      0x01,        // authenticatorMakeCredential command
      0xa5,        // map(5)
      0x01,        //  clientDataHash
      0x58, 0x20,  // bytes(32)
      0x68, 0x71, 0x34, 0x96, 0x82, 0x22, 0xec, 0x17, 0x20, 0x2e, 0x42, 0x50,
      0x5f, 0x8e, 0xd2, 0xb1, 0x6a, 0xe2, 0x2f, 0x16, 0xbb, 0x05, 0xb8, 0x8c,
      0x25, 0xdb, 0x9e, 0x60, 0x26, 0x45, 0xf1, 0x41,

      0x02,        // unsigned(2) - rp
      0xa2,        // map(2)
      0x62,        // text(2)
      0x69, 0x64,  // "id"
      0x68,        // text(8)
      // "acme.com"
      0x61, 0x63, 0x6d, 0x65, 0x2e, 0x63, 0x6f, 0x6d,
      0x64,                    // text(4)
      0x6e, 0x61, 0x6d, 0x65,  // "name"
      0x64,                    // text(4)
      0x41, 0x63, 0x6d, 0x65,  // "Acme"

      0x03,        // unsigned(3) - user
      0xa4,        // map(4)
      0x62,        // text(2)
      0x69, 0x64,  // "id"
      0x58, 0x20,  // bytes(32) - user id
      0x30, 0x82, 0x01, 0x93, 0x30, 0x82, 0x01, 0x38, 0xa0, 0x03, 0x02, 0x01,
      0x02, 0x30, 0x82, 0x01, 0x93, 0x30, 0x82, 0x01, 0x38, 0xa0, 0x03, 0x02,
      0x01, 0x02, 0x30, 0x82, 0x01, 0x93, 0x30, 0x82,
      0x64,                    // text(4)
      0x69, 0x63, 0x6f, 0x6e,  // "icon"
      0x78, 0x28,              // text(40)
      // "https://pics.acme.com/00/p/aBjjjpqPb.png"
      0x68, 0x74, 0x74, 0x70, 0x73, 0x3a, 0x2f, 0x2f, 0x70, 0x69, 0x63, 0x73,
      0x2e, 0x61, 0x63, 0x6d, 0x65, 0x2e, 0x63, 0x6f, 0x6d, 0x2f, 0x30, 0x30,
      0x2f, 0x70, 0x2f, 0x61, 0x42, 0x6a, 0x6a, 0x6a, 0x70, 0x71, 0x50, 0x62,
      0x2e, 0x70, 0x6e, 0x67,
      0x64,                    // text(4)
      0x6e, 0x61, 0x6d, 0x65,  // "name"
      0x76,                    // text(22)
      // "johnpsmith@example.com"
      0x6a, 0x6f, 0x68, 0x6e, 0x70, 0x73, 0x6d, 0x69, 0x74, 0x68, 0x40, 0x65,
      0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d,
      0x6b,  // text(11)
      // "displayName"
      0x64, 0x69, 0x73, 0x70, 0x6c, 0x61, 0x79, 0x4e, 0x61, 0x6d, 0x65,
      0x6d,  // text(13)
      // "John P. Smith"
      0x4a, 0x6f, 0x68, 0x6e, 0x20, 0x50, 0x2e, 0x20, 0x53, 0x6d, 0x69, 0x74,
      0x68,

      0x04,                    // unsigned(4) - pubKeyCredParams
      0x82,                    // array(2)
      0xa2,                    // map(2)
      0x63,                    // text(3)
      0x61, 0x6c, 0x67,        // "alg"
      0x07,                    // 7
      0x64,                    // text(4)
      0x74, 0x79, 0x70, 0x65,  // "type"
      0x6a,                    // text(10)
      // "public-key"
      0x70, 0x75, 0x62, 0x6C, 0x69, 0x63, 0x2D, 0x6B, 0x65, 0x79,
      0xa2,                    // map(2)
      0x63,                    // text(3)
      0x61, 0x6c, 0x67,        // "alg"
      0x19, 0x01, 0x01,        // 257
      0x64,                    // text(4)
      0x74, 0x79, 0x70, 0x65,  // "type"
      0x6a,                    // text(10)
      // "public-key"
      0x70, 0x75, 0x62, 0x6C, 0x69, 0x63, 0x2D, 0x6B, 0x65, 0x79,

      0x07,        // unsigned(7) - options
      0xa2,        // map(2)
      0x62,        // text(2)
      0x72, 0x6b,  // "rk"
      0xf5,        // True(21)
      0x62,        // text(2)
      0x75, 0x76,  // "uv"
      0xf5         // True(21)
      // clang-format on
  };

  PublicKeyCredentialRpEntity rp("acme.com");
  rp.SetRpName("Acme");

  PublicKeyCredentialUserEntity user(
      std::vector<uint8_t>(kUserId, std::end(kUserId)));
  user.SetUserName("johnpsmith@example.com")
      .SetDisplayName("John P. Smith")
      .SetIconUrl(GURL("https://pics.acme.com/00/p/aBjjjpqPb.png"));

  CtapMakeCredentialRequest make_credential_param(
      std::vector<uint8_t>(kClientDataHash, std::end(kClientDataHash)),
      std::move(rp), std::move(user),
      PublicKeyCredentialParams({{"public-key", 7}, {"public-key", 257}}));
  auto serialized_data = make_credential_param.SetResidentKeySupported(true)
                             .SetUserVerificationRequired(true)
                             .EncodeAsCBOR();
  EXPECT_THAT(serialized_data, testing::ElementsAreArray(kSerializedRequest));
}

TEST(CTAPRequestTest, TestConstructGetAssertionRequest) {
  static constexpr uint8_t kClientDataHash[] = {
      0x68, 0x71, 0x34, 0x96, 0x82, 0x22, 0xec, 0x17, 0x20, 0x2e, 0x42,
      0x50, 0x5f, 0x8e, 0xd2, 0xb1, 0x6a, 0xe2, 0x2f, 0x16, 0xbb, 0x05,
      0xb8, 0x8c, 0x25, 0xdb, 0x9e, 0x60, 0x26, 0x45, 0xf1, 0x41};

  static constexpr uint8_t kSerializedRequest[] = {
      // clang-format off
      0x02,  // authenticatorGetAssertion command
      0xa4,  // map(4)

      0x01,  // rpId
      0x68,  // text(8)
      // "acme.com"
      0x61, 0x63, 0x6d, 0x65, 0x2e, 0x63, 0x6f, 0x6d,

      0x02,        // unsigned(2) - client data hash
      0x58, 0x20,  // bytes(32)
      0x68, 0x71, 0x34, 0x96, 0x82, 0x22, 0xec, 0x17, 0x20, 0x2e, 0x42, 0x50,
      0x5f, 0x8e, 0xd2, 0xb1, 0x6a, 0xe2, 0x2f, 0x16, 0xbb, 0x05, 0xb8, 0x8c,
      0x25, 0xdb, 0x9e, 0x60, 0x26, 0x45, 0xf1, 0x41,

      0x03,        // unsigned(3) - allow list
      0x82,        // array(2)
      0xa2,        // map(2)
      0x62,        // text(2)
      0x69, 0x64,  // "id"
      0x58, 0x40,
      // credential ID
      0xf2, 0x20, 0x06, 0xde, 0x4f, 0x90, 0x5a, 0xf6, 0x8a, 0x43, 0x94, 0x2f,
      0x02, 0x4f, 0x2a, 0x5e, 0xce, 0x60, 0x3d, 0x9c, 0x6d, 0x4b, 0x3d, 0xf8,
      0xbe, 0x08, 0xed, 0x01, 0xfc, 0x44, 0x26, 0x46, 0xd0, 0x34, 0x85, 0x8a,
      0xc7, 0x5b, 0xed, 0x3f, 0xd5, 0x80, 0xbf, 0x98, 0x08, 0xd9, 0x4f, 0xcb,
      0xee, 0x82, 0xb9, 0xb2, 0xef, 0x66, 0x77, 0xaf, 0x0a, 0xdc, 0xc3, 0x58,
      0x52, 0xea, 0x6b, 0x9e,

      0x64,                    // text(4)
      0x74, 0x79, 0x70, 0x65,  // "type"
      0x6a,                    // text(10)
      // "public-key"
      0x70, 0x75, 0x62, 0x6C, 0x69, 0x63, 0x2D, 0x6B, 0x65, 0x79,
      0xa2,        // map(2)
      0x62,        // text(2)
      0x69, 0x64,  // "id"
      0x58, 0x32,  // text(22)
      // credential ID
      0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
      0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
      0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
      0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
      0x03, 0x03,
      0x64,                    // text(4)
      0x74, 0x79, 0x70, 0x65,  // "type"
      0x6a,                    // text(10)
      // "public-key"
      0x70, 0x75, 0x62, 0x6C, 0x69, 0x63, 0x2D, 0x6B, 0x65, 0x79,

      0x07,        // unsigned(7) - options
      0xa2,        // map(2)
      0x62,        // text(2)
      0x75, 0x70,  // "up"
      0xf5,        // True(21)
      0x62,        // text(2)
      0x75, 0x76,  // "uv"
      0xf5         // True(21)

      // clang-format on
  };

  CtapGetAssertionRequest get_assertion_req(
      "acme.com",
      std::vector<uint8_t>(kClientDataHash, std::end(kClientDataHash)));

  std::vector<PublicKeyCredentialDescriptor> allowed_list;
  allowed_list.push_back(PublicKeyCredentialDescriptor(
      "public-key",
      {0xf2, 0x20, 0x06, 0xde, 0x4f, 0x90, 0x5a, 0xf6, 0x8a, 0x43, 0x94,
       0x2f, 0x02, 0x4f, 0x2a, 0x5e, 0xce, 0x60, 0x3d, 0x9c, 0x6d, 0x4b,
       0x3d, 0xf8, 0xbe, 0x08, 0xed, 0x01, 0xfc, 0x44, 0x26, 0x46, 0xd0,
       0x34, 0x85, 0x8a, 0xc7, 0x5b, 0xed, 0x3f, 0xd5, 0x80, 0xbf, 0x98,
       0x08, 0xd9, 0x4f, 0xcb, 0xee, 0x82, 0xb9, 0xb2, 0xef, 0x66, 0x77,
       0xaf, 0x0a, 0xdc, 0xc3, 0x58, 0x52, 0xea, 0x6b, 0x9e}));
  allowed_list.push_back(PublicKeyCredentialDescriptor(
      "public-key",
      {0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
       0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
       0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
       0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
       0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03}));

  get_assertion_req.SetAllowList(std::move(allowed_list))
      .SetUserPresenceRequired(true)
      .SetUserVerificationRequired(true);

  auto serialized_data = get_assertion_req.EncodeAsCBOR();
  EXPECT_THAT(serialized_data, testing::ElementsAreArray(kSerializedRequest));
}

TEST(CTAPRequestTest, TestConstructCtapAuthenticatorRequestParam) {
  static constexpr uint8_t kSerializedGetInfoCmd = 0x04;
  static constexpr uint8_t kSerializedGetNextAssertionCmd = 0x08;
  static constexpr uint8_t kSerializedCancelCmd = 0x03;
  static constexpr uint8_t kSerializedResetCmd = 0x07;

  EXPECT_THAT(AuthenticatorGetInfoRequest().Serialize(),
              testing::ElementsAre(kSerializedGetInfoCmd));
  EXPECT_THAT(AuthenticatorGetNextAssertionRequest().Serialize(),
              testing::ElementsAre(kSerializedGetNextAssertionCmd));
  EXPECT_THAT(AuthenticatorCancelRequest().Serialize(),
              testing::ElementsAre(kSerializedCancelCmd));
  EXPECT_THAT(AuthenticatorResetRequest().Serialize(),
              testing::ElementsAre(kSerializedResetCmd));
}

}  // namespace device