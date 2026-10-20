#pragma once

#define REQUIRE_TOKEN(tokenizer, token, expected) \
  if (!((token) = (tokenizer).Next(expected))) \
    return std::nullopt

#define REQUIRE_SIZE(token, expected_size) \
  if ((token)->size() != (expected_size)) \
    return std::nullopt

#define REQUIRE_DIGIT(token) \
  if ( \
    (token)->empty() || \
    !std::isdigit(static_cast<unsigned char>((token)->front())) \
  ) \
    return std::nullopt
