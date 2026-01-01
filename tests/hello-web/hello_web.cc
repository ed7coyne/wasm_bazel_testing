#include <fmt/core.h>
#include <string>

int main(int argc, char** argv) {
    std::string name = "Web";
    if (argc > 1) {
        name = argv[1];
    }
    fmt::print("Hello {}!\n", name);
    return 0;
}