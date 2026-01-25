#include <iostream>
#include "value_semantics_observer.h"
enum class Tag{
    INT,
    BOOL,
    DOUBLE,
    STRING
};
struct Publisher{
    int getInt() const{
        return rand()%10;
    }
};


int main(){
    Publisher pub;
    Observer<Publisher, Tag> int_observer([](const Publisher& pub, Tag tag){
        if(tag == Tag::INT){
            std::cout << "onUpdate got: " << pub.getInt() << std::endl;
        }
    });


    // Simulate publiser call observer callback
    // on notify
    int_observer.update(pub, Tag::INT);
    return 0;
}