#include "dymanic_event_bus.h"

int main(){
    EventBus bus;
    MultiObjectHandler multi(bus);
    HumanPoint human(bus);
    Seatbelt seat(bus);

    multi.produce();

    return 0;
}