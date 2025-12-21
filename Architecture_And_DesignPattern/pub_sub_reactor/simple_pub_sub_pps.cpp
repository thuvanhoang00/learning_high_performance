#include "PPSManager.h"
#include <thread>
#include "Model.h"
#include "View.h"
int main(){
    View1 v;
    // PPSManager::instance().addViewer()
    auto cb = [&v](std::string r_data){
        v.existed_function(r_data);
    };
    PPSManager::instance().addViewer("/pps/thuhv4", cb);

    while(true){
        std::this_thread::sleep_for(std::chrono::seconds(2)); 
        Model::onModelInfoAvailable();
    }
    return 0;
}