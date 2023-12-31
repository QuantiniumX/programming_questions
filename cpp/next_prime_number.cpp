#include <cmath>
#include <iostream>

bool isPrime(u_int64_t n);

int main(void) {
  u_int64_t n{1};

  std::cout << "Enter a number: ";
  std::cin >> n;

  while (isPrime(n) == false) {
    n++;
  }
  std::cout << n << std::endl;

  return 0;
}

bool isPrime(u_int64_t n) {
  if (n <= 1) {
    return false;
  }

  for (int i = 2; i <= sqrt(n); ++i) {
    if (n % i == 0) {
      return false;
    }
  }

  return true;
}
