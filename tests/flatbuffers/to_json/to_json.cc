#include <iostream>
#include <vector>
#include <iterator>
#include <string>
#include <cstring>

#include "flatbuffers/idl.h"

#include "tests/flatbuffers/to_json/robot_fbs_h.h"

int main() {
    std::cin >> std::noskipws;
    std::vector<uint8_t> buffer((std::istream_iterator<char>(std::cin)), std::istream_iterator<char>());

    if (buffer.empty()) {
        std::cerr << "Error: Empty input" << std::endl;
        return 1;
    }

    const uint8_t* buf_ptr = buffer.data();
    size_t buf_size = buffer.size();

    // 2. Setup Parser with Schema
    flatbuffers::Parser parser;
    parser.opts.strict_json = true;
    if (!parser.Parse(kRobotFbs)) {
        std::cerr << "Error parsing schema: " << parser.error_ << std::endl;
        return 1;
    }

    // 3. Generate JSON
    std::string json_output;
    const char* err = flatbuffers::GenerateText(parser, buf_ptr, &json_output);
    if (err) {
        std::cerr << "Error generating JSON text: " << err << std::endl;
        return 1;
    }

    // 4. Print JSON
    std::cout << json_output << std::endl;

    return 0;
}
