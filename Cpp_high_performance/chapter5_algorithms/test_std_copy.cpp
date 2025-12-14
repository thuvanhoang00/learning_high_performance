#include <algorithm>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <cstdio>
int main(){
        // char str1[] = "Geeks";
    // char str2[6] = "";

    // Copies contents of str1 to str2
    // memcpy(str2, str1, sizeof(str1));

    // printf("str2 after memcpy:");
    // printf("%s",str2);

    char str1[] = "Hello world";
    alignas(64) int a;
    char str2[5] = "";
    // memcpy(str2, str1, sizeof(str2)); // Undefined behavior 
    // printf("str2: after memcpy: %s", str2);
    /*
    str2: after memcpy:
    HelloHello world
    */

    // Using std::copy
    // std::copy(std::begin(str1), std::end(str1), str2); // Undefined behavior
    // std::cout << "\nstr2 after std::copy: " << str2 << std::endl;
    /* str2 after std::copy: Hello world */

    // better use snprinf() or std::string
    // or copy_n
    std::snprintf(str2, sizeof(str2), "%s", str1);
    std::cout << "\nstr2 after use snprintf: " << str2 << std::endl;
    return 0;
}