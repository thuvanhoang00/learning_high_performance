#include "logging.h"

int main(int, char**){
    using namespace  thu;
    char c = 'd';
    int i = 3;
    unsigned long ul = 65;
    float f = 3.4;
    double d = 1.234;
    const char* s = "test C-string";
    std::string ss = "test string";
    Logger logger("logging_example.log");
    logger.log("Logging a char: % and int: % and an unsigned: %\n", c,i,ul);
    logger.log("Logging a float: % and a double: %\n", f, d);
    logger.log("Logging a C-string: '%'\n", s);
    logger.log("Logging a string: '%'\n", ss);
    
    return 0;
}