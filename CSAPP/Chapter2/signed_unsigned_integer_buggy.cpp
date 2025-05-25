#include <iostream>
#include <cstring>
typedef unsigned char *byte_pointer;

void show_bytes(byte_pointer start, size_t len){
    for(int i=0; i<len; i++){
        printf(" %.2x", start[i]);
    }
    printf("\n");
}

void show_int(int x){
    show_bytes((byte_pointer)&x, sizeof(int));
}


// Buggy code 1
float sum_elements(float a[], unsigned length){
    int i=0;
    float result = 0;
    for(;i<length-1;i++){ // Fix: i<length
        result += a[i];
    }
    return result;
}

// Buggy code 2
bool strlonger(const char *s, const char *t){
    return strlen(s)-strlen(t) > 0; // Fix: static_cast<int>(strlen(s)-strlen(t)) > 0
}

int main()
{
    unsigned int a = 0;
/*  Test Buggy 1
    float arr[]={};
    std::cout << sum_elements(arr, 0);
    
*/
/*  Test Buggy 2
*/
    const char *s1 = "aaae";
    const char *s2 = "abcd";
    std::cout  << strlonger(s1, s2) << "\n";
    return 0;
}