#include <vector>
#include <cstdint>

namespace cobs_transcoder
{

    // COBS encode
    std::vector<uint8_t> encode(const std::vector<uint8_t> &input)
    {
        std::vector<uint8_t> output;
        output.reserve(input.size() + 2); // Worst case

        size_t idx = 0;
        size_t code_idx = 0;
        output.push_back(0); // Placeholder for first code byte

        uint8_t code = 1;

        while (idx < input.size())
        {
            if (input[idx] == 0)
            {
                output[code_idx] = code;
                code_idx = output.size();
                output.push_back(0); // Placeholder for next code byte
                code = 1;
            }
            else
            {
                output.push_back(input[idx]);
                code++;
                if (code == 0xFF)
                {
                    output[code_idx] = code;
                    code_idx = output.size();
                    output.push_back(0); // Placeholder for next code byte
                    code = 1;
                }
            }
            ++idx;
        }
        output[code_idx] = code;
        return output;
    }

    std::vector<uint8_t> decode(const std::vector<uint8_t> &input)
    {
        std::vector<uint8_t> output;
        size_t idx = 0;

        while (idx < input.size())
        {
            uint8_t code = input[idx];
            if (code == 0 || idx + code - 1 >= input.size())
            {
                // Invalid COBS stream
                return {};
            }
            ++idx;
            for (uint8_t i = 1; i < code; ++i)
            {
                if (idx >= input.size())
                    return {};
                output.push_back(input[idx++]);
            }
            if (code != 0xFF && idx < input.size())
            {
                output.push_back(0);
            }
        }

        return output;
    }
} // namespace cobs_transcoder