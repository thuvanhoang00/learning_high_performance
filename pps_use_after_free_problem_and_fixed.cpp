#if 0
class MyView {
public:
    MyView(Foo& model) {
        // BUG SOURCE: We capture 'this' to access member variables
        model.add_cb<int>([this](const int& val) {
            this->onUpdate(val); 
        });
    }
    
    ~MyView() { std::cout << "View Destroyed\n"; }

    void onUpdate(int val) {
        // If this runs after destructor, 'name' is garbage memory -> Segfault
        std::cout << "View received: " << val << " Name: " << name << std::endl;
    }

    std::string name = "Dashboard";
};

int main() {
    Foo f;
    
    {
        MyView* v = new MyView(f);
        f.call_cb<int>(1); // Works fine
        delete v;          // View is dead now
    } 

    std::cout << "triggering callback after death..." << std::endl;
    f.call_cb<int>(2); // CRASH! 'this' inside lambda is invalid.
    
    return 0;
}



// FIXED:
// 1. View must inherit from enable_shared_from_this
class SafeView : public std::enable_shared_from_this<SafeView> {
public:
    void registerWith(Foo& f) {
        // 2. Get a weak reference to myself
        std::weak_ptr<SafeView> weak_self = shared_from_this();

        f.add_cb<int>([weak_self](const int& val) {
            // 3. Try to lock (promote) to strong pointer
            if (auto shared_self = weak_self.lock()) {
                // If we get here, object is alive and guaranteed to stay alive 
                // for the duration of this function
                shared_self->onUpdate(val);
            } else {
                std::cout << "Callback ignored: View is dead.\n";
            }
        });
    }

    void onUpdate(int val) { std::cout << "Updated: " << val << "\n"; }
};


// Solution 2:
struct Foo {
    using GenericCallback = std::function<void(const void*)>;
    
    // Store ID + Callback pairs
    struct Entry {
        int id;
        GenericCallback cb;
    };
    
    std::unordered_map<std::type_index, std::vector<Entry>> cbs_;
    int next_id_ = 0;

    // Returns an ID that can be used to unsubscribe
    template<typename T>
    int add_cb(std::function<void(const T&)> user_cb) { 
        int id = next_id_++;
        
        GenericCallback wrapper = [user_cb](const void* data) {
            const T* typedData = static_cast<const T*>(data);
            user_cb(*typedData); 
        };

        cbs_[typeid(T)].push_back({id, wrapper});
        return id;
    }

    template<typename T>
    void remove_cb(int id) {
        auto& vec = cbs_[typeid(T)];
        // Erase-remove idiom
        vec.erase(std::remove_if(vec.begin(), vec.end(), 
            [id](const Entry& e){ return e.id == id; }), vec.end());
    }

    template<typename T>
    void call_cb(const T& data) {
        if (cbs_.count(typeid(T))) {
            for (const auto& entry : cbs_[typeid(T)]) {
                entry.cb(&data);
            }
        }
    }
};

// View Usage
class View {
    int sub_id_;
    Foo& model_;
public:
    View(Foo& f) : model_(f) {
        sub_id_ = model_.add_cb<int>([this](const int& v){ 
             // ... logic ...
        });
    }
    ~View() {
        // Crucial: Unsubscribe when dying
        model_.remove_cb<int>(sub_id_);
    }
};
#endif