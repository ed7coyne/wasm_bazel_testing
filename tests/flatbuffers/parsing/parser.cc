#include <iostream>
#include <vector>
#include <iterator>
#include "tests/flatbuffers/parsing/message_generated.h"

int main() {
    // Read all of stdin into a vector
    std::cin >> std::noskipws;
    std::vector<uint8_t> buffer((std::istream_iterator<char>(std::cin)), std::istream_iterator<char>());

    if (buffer.empty()) {
        std::cerr << "Error: Empty input" << std::endl;
        return 1;
    }

    // Verify parser
    auto verifier = flatbuffers::Verifier(buffer.data(), buffer.size());
    if (!tests::parsing::VerifyMessageBuffer(verifier)) {
        std::cerr << "Error: Invalid buffer" << std::endl;
        return 1;
    }

    // Parse message
    auto message = tests::parsing::GetMessage(buffer.data());
    
    // Print payload
    if (message->payload()) {
        std::cout << message->payload()->str() << std::endl;
    } else {
        std::cout << "(empty)" << std::endl;
    }

    return 0;
}
