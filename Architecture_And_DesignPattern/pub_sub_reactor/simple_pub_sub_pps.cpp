#include "PPSManager.h"
#include <thread>
#include "Model.h"
#include "View.h"
int main(){
    View1 v;
    OtherView o_v;
    // PPSManager::instance().addViewer()
    auto cb = [&v](std::string r_data){
        v.existed_function(r_data);
    };

    auto o_cb = [&o_v](std::string r_data){
        o_v.other_function(r_data);
    };

    PPSManager::instance().addViewer("/pps/thuhv4", cb);
    PPSManager::instance().addViewer("/pps/thuhv4", o_cb);

    while(true){
        std::this_thread::sleep_for(std::chrono::seconds(2)); 
        Model::onModelInfoAvailable();
    }
    return 0;
}