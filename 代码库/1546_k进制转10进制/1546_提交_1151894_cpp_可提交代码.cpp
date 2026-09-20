#include <iostream>

int main() {
    long long base;
    long long digit1;
    long long digit2;
    long long digit3;
    long long digit4;
    if (!(std::cin >> base >> digit1 >> digit2 >> digit3 >> digit4)) {
        return 0;
    }

    std::cout << digit4 + digit3 * base + digit2 * base * base
              + digit1 * base * base * base << '\n';
    return 0;
}
