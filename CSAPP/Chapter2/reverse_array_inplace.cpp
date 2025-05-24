#include <iostream>
#include <array>

void swap_inplace(int& a, int& b){
    a = a ^ b;
    b = a ^ b;
    a = a ^ b;
}

void reverse(std::array<int, 5>& arr){
    for(int first = 0, last = arr.size()-1; first < last; first++, last--){
        swap_inplace(arr[first], arr[last]);
    }
}

void reverse(std::array<int, 4>& arr){
    for(int first = 0, last = arr.size()-1; first < last; first++, last--){
        swap_inplace(arr[first], arr[last]);
    }
}

void print(std::array<int, 5> arr){
    for(auto e : arr){
        std::cout << e  << ' ';
    }
    std::cout << "\n";
}

void print(std::array<int, 4> arr){
    for(auto e : arr){
        std::cout << e  << ' ';
    }
    std::cout << "\n";
}

int main()
{
    std::array<int, 5> arr{1,2,3,4,5};
    reverse(arr);
    print(arr);

    std::array<int, 4> arr2{1,2,3,4};
    reverse(arr2);
    print(arr2);

    // std::cout << ~0 << "\n";
    printf("%x\n", ~0);
    return 0;
}