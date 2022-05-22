#pragma once

#include <vector>
#include <mutex>
#include <map>

template <typename Key, typename Value>
class ConcurrentMap {
public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    struct Bucket_
    {
        std::map<Key, Value> map_;
        std::mutex mutex_;

    };

    struct Access {
        std::lock_guard<std::mutex> lock;
        Value& ref_to_value;
        Access( std::map<Key, Value> &maps,const Key &key, std::mutex &mut) :lock(mut), ref_to_value(maps[key])
        {

        }
    };

    explicit ConcurrentMap(size_t bucket_count): bucket_(bucket_count){}

    Access operator[](const Key& key)
    {
        auto& bucket = bucket_[static_cast<uint64_t>(key) % bucket_.size()];
        return {bucket.map_,key ,bucket.mutex_};
    }

    std::map<Key, Value> BuildOrdinaryMap()
    {
        std::map<Key, Value> result_map;
        for( auto& [map, mut]:bucket_)
        {
            std::lock_guard guard(mut);
            result_map.insert(map.begin(),map.end());

        }
        return result_map;
    }

    void erase(const Key &key)
    {
        auto& bucket = bucket_[static_cast<uint64_t>(key) % bucket_.size()];
        std::lock_guard guard(bucket.mutex_);
        bucket.map_.erase(key);
    }

private:
    size_t size_map;
    std::vector<Bucket_> bucket_;

};



