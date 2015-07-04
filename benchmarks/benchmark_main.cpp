#include <fstream>  // for std::ofstream
#include <iostream> // for std::cout
#include <random>   // for std::random_device, std::mt19937

int main(const int argc, char *const argv[]) {
  if (argc != 1) {
    std::cerr << "Usage: " << argv[0] << '\n';
    return -1;
  }
}
