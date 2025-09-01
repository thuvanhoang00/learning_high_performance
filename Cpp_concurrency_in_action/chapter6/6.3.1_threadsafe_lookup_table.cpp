#include <utility>
#include <list>
#include <mutex>
#include <vector>
#include <memory>
#include <shared_mutex>
#include <map>
template<typename Key, typename Value, typename Hash=std::hash<Key>>
class threadsafe_lookup_table
{
private:
    class bucket_type
    {
    private:
        typedef std::pair<Key,Value> bucket_value;
        typedef std::list<bucket_value> bucket_data;
        typedef typename bucket_data::iterator bucket_iterator;
        
        bucket_data data_;
        mutable std::shared_mutex mutex_;

        bucket_iterator find_entry_for(Key const& key) const{
            return std::find_if(data_.begin(), data_.end(), [&](bucket_value const& item){return item.first == key});
        }
    public:
        Value value_for(Key const& key, Value const& default_value) const{
            std::shared_lock<std::shared_mutex> lock(mutex_);
            bucket_iterator const found_entry = find_entry_for(key);
            return (found_entry==data_.end() ? default_value : found_entry->second);
        }
        void add_or_update_mapping(Key const& key, Value const& value){
            std::unique_lock<std::shared_mutex> lock(mutex_);
            bucket_iterator const found_entry = find_entry_for(key);
            if(found_entry == data_.end()){
                data_.push_back(bucket_value(key, value));
            }
            else{
                found_entry->second = value;
            }
        }
        void remove_mapping(Key const& key){
            std::unique_lock<std::shared_mutex> lock(mutex_);
            bucket_iterator const found_entry = find_entry_for(key);
            if(found_entry != data_.end()){
                data_.erase(found_entry);
            }
        }
    };
    
private:
    std::vector<std::unique_ptr<bucket_type>> buckets_;
    Hash hasher_;
    bucket_type& get_bucket(Key const& key) const{
            std::size_t const bucket_index = hasher_(key)%buckets_.size();
            return *buckets_[bucket_index];
    }
public:
    typedef Key key_value;
    typedef Value mapped_type;
    tyepdef Hash hash_type;
    threadsafe_lookup_table(unsigned num_buckets = 19, Hash const& hasher = Hash())
        : buckets_(num_buckets), hasher_(hasher)
    {
        for(unsigned i = 0; i < num_buckets; ++i){
            buckets_[i].reset(new bucket_type);
        }
    }
    threadsafe_lookup_table(threadsafe_lookup_table const &) =delete;
    threadsafe_lookup_table& opeartor=(threadsafe_lookup_table const&) = delete;
    Value value_for(Key const& key, Value const& default_value=Value()) const{
        return get_bucket(key).value_for(key, default_value);
    }
    void add_or_update_mapping(Key const& key, Value const& value){
        get_bucket(key).add_or_update_mapping(key, value);
    }
    void remove_mapping(Key const& key){
        get_bucket(key).remove_mapping(key);
    }

    std::map<Key, Value> get_map() const{
        std::vector<std::unique_lock<std::shared_mutex>> locks;
        for(unsigned i=0; i<buckets_.size(); ++i){
            locks.push_back(std::unique_lock<std::shared_mutex>(buckets_[i].mutex_));
        }
        std::map<Key, Value> res;
        for(unsigned i=0; i<buckets_.size(); ++i){
            for(bucket_iterator it = buckets_[i].data_.begin(); it != buckets_[i].data_.end(); ++i){
                res.insert(*it);
            }
        }
        return res;
    }
};