#include <iostream>
#include <cstring>
#include <set>
#include <map>
#include <vector>
#include <cstdint>
#include <array>
#include <string_view>
#include <random>
#include <fstream>
#include <algorithm>
#include <iomanip>
#define HEX( x, len ) std::setw(2 * len) << std::setfill('0') << std::hex << std::uppercase << (((1ll << (8 * len)) - 1) & (unsigned int)( x )) << std::dec
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

using uint64 = aggregate<std::uint32_t>;
using uint128_t = aggregate<std::uint64_t>;
using uint256_t = aggregate<uint128_t>;

template <typename T>
struct ring
{
private:
    const std::size_t ring_node_count;
    const std::size_t replication_count;
    set<T> vnode_tree;
    map<string, vector<T>> node_to_vnode; // node, vnodes
    map<T, string> vnode_to_node; // vnode, node

    T hash(string input) 
    {
        SHA256 sha;
        sha.update(input);
        auto arr = sha.digest();
        std::array<uint8_t, sizeof(T)> tbr;
        for (int i = 0; i < tbr.size(); i++) {
            tbr[i] = arr[i];
        }
        return tbr;
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
        T ip_hash = hash(string(input));
        if (lower_bound(vnode_tree.begin(), vnode_tree.end(), ip_hash) != vnode_tree.end())
            return vnode_to_node[*lower_bound(vnode_tree.begin(), vnode_tree.end(), ip_hash)];
        else
            return vnode_to_node[*vnode_tree.begin()];
    }
};

void hex_view(const char* str, const size_t len)
{
    for (int i = 0; i < ((int)len >> 4) + (len % 16 ? 1 : 0); ++i)
    {
        int start = i * 16;
        int end = std::min(i * 16 + 15, (int)len - 1);
        std::cout << "0x" << HEX(start, 4) << " |  ";
        for (int j = start; j <= end; ++j)
            std::cout << HEX(str[j], 1) << (j % 8 == 7 ? "  " : " ");

        for (size_t j = end; j < i * 16 + 15; ++j)
            std::cout << (j % 8 == 7 ? "    " : "   ");
        std::cout << "| ";
        for (size_t j = start; j <= end; ++j)
            std::cout << (std::isprint(str[j]) ? str[j] : '.');
        std::cout << '\n';
    }
}


std::mt19937_64& get_rnd_generator() {
    static std::random_device rd{};
    static std::seed_seq ss{ rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd() };
    static std::mt19937_64 mt{ ss };
    return mt;
}

string rnd_string() {
    std::uniform_int_distribution char_gen{ 33, 126 }, size_gen{ 1, 1000 };
    string s; int str_size = size_gen(get_rnd_generator());
    for (int i = 0; i < str_size; i++) {
        s += (char)char_gen(get_rnd_generator());
    }
    return s;
}

template <typename U>
void do_runs(ring<U>& x, int runs) {
    std::mt19937_64& mt = get_rnd_generator();
    std::uniform_int_distribution nums{ 1, 3 };
    std::ofstream out(std::format("C:\\Users\\91730\\source\\repos\\Ring\\tests\\%.iruns_64.txt", runs));
    set<string> IPs;
    for (int i = 0; i < runs; i++) {
        int choice = nums(mt);
        if (choice == 1) {
            // add a destination
            string ip = rnd_string();
            while (IPs.find(ip) != IPs.end())
                ip = rnd_string();
            IPs.insert(ip);
            x.add_destination(ip);
            out << "added destination: " << ip << "\n";
        }
        else if (choice == 2) {
            // remove a destination
            if (IPs.size() == 0) {
                out << "No destination to remove.\n";
                continue;
            }
            auto ip = IPs.lower_bound(rnd_string());
            if (ip == IPs.end())
                ip = IPs.begin();
            x.remove_destination(*ip);
            out << "removed destiantion: " << *ip << "\n";
            IPs.erase(*ip);
        }
        else {
            if (IPs.size() == 0) {
                out << "No destination to find.\n";
                continue;
            }
            string val = rnd_string();
            auto ip = x.find_destination(val);
            out << "nearest destination for " << val << " is: " << ip << "\n";
        }
    }
    out.close();
}

int main() {
    ring<uint64> x(1ll << 32, 5);
    int runs{ 0 }; std::cin >> runs;
    do_runs(x, runs);
    ring<uint128_t> y(1ll << 32, 5);
    do_runs(y, runs);
    ring<uint256_t> z(1ll << 32, 5);
    do_runs(z, runs);
}