#pragma once
#include <vector>
#include <cstdint>

namespace cobs_transcoder
{

    std::vector<uint8_t> encode(const std::vector<uint8_t> &input);
    std::vector<uint8_t> decode(const std::vector<uint8_t> &input);

} // namespace cobs_transcoder