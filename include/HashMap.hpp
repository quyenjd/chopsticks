#ifndef HASHMAP_HPP_INCLUDED
#define HASHMAP_HPP_INCLUDED

#include "Thread.hpp"
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace Thread
{
    template< typename K, typename V >
    class HashMap
    {
    private:
        std::unordered_map<K, Atomic<V>* > table;
        std::mutex mutex;

    public:
        Atomic<V>& operator[] (const K& key)
        {
            auto it = table.find(key);
            if (it == table.end())
            {
                std::unique_lock<std::mutex> lock(mutex);
                table[key] = new Atomic<V>();
                return *table[key];
            }
            return *it->second;
        }

        Atomic<V>& operator[] (const K& key) const
        {
            auto it = table.find(key);
            if (it == table.end())
                throw std::runtime_error("Accessing unknown key in constant hash map is not allowed");
            return *it->second;
        }

        bool has_key (const K& key) const
        {
            auto it = table.find(key);
            return it != table.end();
        }

        void clear()
        {
            std::unique_lock<std::mutex> lock(mutex);
            for (auto it : table)
                delete table[it.first];
            table.clear();
        }

        std::vector<K> keys()
        {
            std::unique_lock<std::mutex> lock(mutex);
            std::vector<K> ret;

            for (auto it = table.begin(); it != table.end(); ++it)
                ret.push_back(it->first);

            return ret;
        }

        std::vector<std::pair<K, V> > entities()
        {
            std::unique_lock<std::mutex> lock(mutex);
            std::vector<std::pair<K, V> > ret;

            for (auto it = table.begin(); it != table.end(); ++it)
                ret.push_back(std::make_pair(it->first, it->second->get()));

            return ret;
        }
    };
}

#endif // HASHMAP_HPP_INCLUDED
