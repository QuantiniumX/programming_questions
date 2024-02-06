#include <iostream>

int fibonacci(int num);

int main() {
    int num;
    std::cin >> num;

    std::cout << fibonacci(num) << std::endl;
}

int fibonacci(int num) {
    if (num == 1) {
        return num;
    }

    int last = fibonacci(num - 1);
    int slast = fibonacci(num - 2);

    return last + slast; 
}
