#include <string>
#include <utility>

struct Annotation
{
    Annotation(const std::string s) : data_(std::move(s)) {} // doesnt do what it seems to, becareful of const object that dont actually move
    std::string data_;
};

/*
class string
{
    string(const string& ); // 
    string(string&& ); // 
    // const string s;
    // std::move(s) cast s to const rvalue?
    // then copy construct will be called
};

*/


int main()
{
    return 0;
}