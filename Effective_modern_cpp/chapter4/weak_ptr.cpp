#include <memory>
#include <unordered_map>

enum class WidgetID;

struct Widget{


};

std::shared_ptr<const Widget> loadWidget(WidgetID id);

// use-case of weak_ptr
std::shared_ptr<const Widget> fastLoadWidget(WidgetID id){
    static std::unordered_map<WidgetID, std::weak_ptr<const Widget>> cache;

    auto objPtr = cache[id].lock(); // objPtr is std::shared_ptr to cached object(or nullptr if object is not in cache)
    if(!objPtr){
        objPtr = loadWidget(id);
        cache[id] = objPtr;
    }
    return objPtr;
}

int main()
{

    return 0;
}