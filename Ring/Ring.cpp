#include <iostream>
#include <cstring>
#include <set>
#include <map>
#include <vector>
#include <cstdint>
#include <array>
#include <string_view>
#include <random>
#include "SHA256.h"

using std::string_view;
using std::string;
using std::vector;
using std::set;
using std::map;
using std::array;

template <typename T>
class aggregate
{
private:
    T lsb;
    T msb;
public:
    aggregate() : lsb{ 0 }, msb{ 0 }
    {

    }
    aggregate(T val) : lsb{ val }, msb{ 0 }
    {

    }
    aggregate(const char* arr) : lsb{ 0 }, msb{ 0 }
    {
        std::memcpy(this, arr, std::min(sizeof *this, std::strlen(arr)));
    }
    template <typename U>
    aggregate(U val) : lsb{ 0 }, msb{ 0 }
    {
        static_assert(sizeof U <= sizeof(*this), "source size is more than destination");
        std::memcpy(this, &val, sizeof val);
    }
    bool operator==(const aggregate& rhs) const
    {
        return ((this->msb == rhs.msb) && (this->lsb == rhs.lsb));
    }
    bool operator!=(const aggregate& rhs) const
    {
        return !(*this == rhs);
    }
    bool operator>(const aggregate& rhs) const
    {
        return ((this->msb > rhs.msb) || ((this->msb == rhs.msb) && (this->lsb > rhs.lsb)));
    }
    bool operator<(const aggregate& rhs) const
    {
        return (!(*this > rhs) && (*this != rhs));
    }
    bool operator>=(const aggregate& rhs) const
    {
        return ((*this > rhs) || (*this == rhs));
    }
    bool operator<=(const aggregate& rhs) const
    {
        return ((*this < rhs) || (*this == rhs));
    }
};

using uint128_t = aggregate<std::uint64_t>;
using uint256_t = aggregate<uint128_t>;

struct ring
{
    using type = uint256_t;
private:
    const std::size_t ring_node_count;
    const std::size_t replication_count;
    set<type> vnode_tree;
    map<string, vector<type>> node_to_vnode; // node, vnodes
    map<type, string> vnode_to_node; // vnode, node

    type hash(string input) 
    {
        SHA256 sha;
        sha.update(input);
        return sha.digest();
    }
public:
    ring(std::size_t ring_node_count, std::size_t replication_count) : ring_node_count{ ring_node_count }, replication_count{ replication_count }
    {

    }
    void add_destination(string IP)
    {
        auto& vnodes = node_to_vnode[IP];

        int i = 0;
        while(vnodes.size() < replication_count)
        {
            auto vnode = hash(IP + std::to_string(i++));
            if (vnode_to_node.find(vnode) != vnode_to_node.end())
                continue;

            vnodes.push_back(vnode);
            vnode_tree.insert(vnode);
            vnode_to_node[vnode] = IP;
        }
    }
    void remove_destination(std::string_view IP) 
    {
        string IP_s = string(IP);
        for (auto& vnode : node_to_vnode[IP_s]) {
            vnode_to_node.erase(vnode);
            vnode_tree.erase(vnode);
        }

        node_to_vnode.erase(IP_s);
    }
    std::string_view find_destination(std::string_view input) 
    {
        uint256_t ip_hash = hash(string(input));
        if (lower_bound(vnode_tree.begin(), vnode_tree.end(), ip_hash) != vnode_tree.end())
            return vnode_to_node[*lower_bound(vnode_tree.begin(), vnode_tree.end(), ip_hash)];
        else
            return vnode_to_node[*vnode_tree.begin()];
    }
};


std::mt19937_64& get_rnd_generator() {
    static std::random_device rd{};
    static std::seed_seq ss{ rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd() };
    static std::mt19937_64 mt{ ss };
    return mt;
}

int main() {
    ring x(1ll << 32, 5);
    x.add_destination("ab");
    x.find_destination("ab");
    x.remove_destination("ab");
    std::mt19937_64& mt = get_rnd_generator();
    std::uniform_int_distribution nums{ 1, 6 };
    int arr[7];
    memset(arr, 0, sizeof arr);
    for (int i = 0; i < 20000; i++) {
        arr[nums(mt)]++;
    }
    for (auto x : arr) {
        std::cout << x << std::endl;
    }
}