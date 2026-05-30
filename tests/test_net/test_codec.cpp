// Smoke test for net::codec.
#include <gtest/gtest.h>

#include "hanabi/net/codec.h"

using namespace hanabi::net;
using nlohmann::json;

TEST(Codec, DecodeCommandOnly) {
  auto m = decode("ping");
  EXPECT_EQ(m.command, "ping");
  EXPECT_TRUE(m.payload.is_object());
  EXPECT_TRUE(m.payload.empty());
}

TEST(Codec, DecodeCommandWithObject) {
  auto m = decode(R"(init {"tableID":42,"player":"alice"})");
  EXPECT_EQ(m.command, "init");
  EXPECT_EQ(m.payload["tableID"], 42);
  EXPECT_EQ(m.payload["player"], "alice");
}

TEST(Codec, DecodeCommandWithArray) {
  auto m = decode(R"(tableList [1,2,3])");
  EXPECT_EQ(m.command, "tableList");
  ASSERT_TRUE(m.payload.is_array());
  EXPECT_EQ(m.payload.size(), 3u);
}

TEST(Codec, EncodeNoPayload) {
  EXPECT_EQ(encode("ping"), "ping");
  EXPECT_EQ(encode("ping", json::object()), "ping");
}

TEST(Codec, EncodeWithObject) {
  std::string s = encode("init", json{{"x", 1}});
  EXPECT_EQ(s, R"(init {"x":1})");
}

TEST(Codec, RoundTrip) {
  json payload = {{"tableID", 7}, {"type", 0}};
  std::string wire = encode("perform", payload);
  auto m = decode(wire);
  EXPECT_EQ(m.command, "perform");
  EXPECT_EQ(m.payload, payload);
}

TEST(Codec, DecodeBadJsonThrows) {
  EXPECT_THROW(decode("init {bad json}"), std::invalid_argument);
}
