#include "PPSManager.h"
#include <thread>
#include "Model.h"
#include "View.h"
int main(){
    View v;
    while(true){
        std::this_thread::sleep_for(std::chrono::seconds(2)); 
        Model::onModelInfoAvailable();
    }
    return 0;
}