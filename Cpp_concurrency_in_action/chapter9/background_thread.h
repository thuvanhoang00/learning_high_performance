#include "interruptible_thread.h"
#include <vector>

std::mutex config_mutex;
std::vector<interruptible_thread> back_ground_threads;
void background_thread(int disk_id)
{
    while(true){
        interruption_point();
        fs_change fsc=get_fs_changes(disk_id);
        if(fsc.has_changes()){
            update_index(fsc);
        }
    }
}
//....