#include "template.h"


int main(){
    MultiObjectHandlder multiobj_handler;
    HumanPoint humanpoint_handler;
    Seatbelt seatbelt_handler;
    multiobj_handler.attach(&humanpoint_handler);
    multiobj_handler.attach(&seatbelt_handler);
    humanpoint_handler.attach(&seatbelt_handler);
    return 0;
}