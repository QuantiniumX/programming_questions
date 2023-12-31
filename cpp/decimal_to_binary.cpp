#include <bitset>
#include <iostream>

int main() {
  std::string binary = std::bitset<8>(12).to_string(); // to binary
  std::cout << binary << "\n";

  unsigned long decimal = std::bitset<8>(binary).to_ulong();
  std::cout << decimal << "\n";
  return 0;
}
