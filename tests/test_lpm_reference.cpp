#include "lpm.hpp"
#include "common.hpp"

#include <algorithm>
#include <random>
#include <vector>

struct Route { uint32_t prefix; int length; int port; };

static uint32_t mask(int length)
{
    return length == 0 ? 0U : (~0U << (32 - length));
}

static int reference(const std::vector<Route>& routes, uint32_t ip)
{
    int best = -1;
    int best_length = -1;
    for (const auto& route : routes) {
        if ((ip & mask(route.length)) == route.prefix &&
            route.length >= best_length) {
            best_length = route.length;
            best = route.port;
        }
    }
    return best;
}

int main()
{
    long total = 0;
    long mismatches = 0;
    for (int seed = 1; seed <= 6; ++seed) {
        std::mt19937 rng(seed);
        std::vector<uint32_t> bases;
        for (int i = 0; i < 6; ++i) bases.push_back(rng());
        std::vector<Route> routes;
        for (int i = 0; i < 300; ++i) {
            const int length = (rng() % 20 == 0) ? 0 : 1 + rng() % 32;
            const uint32_t prefix =
                (bases[rng() % bases.size()] ^ (rng() & 0xFFFFU)) & mask(length);
            routes.push_back({prefix, length, static_cast<int>(rng() % 8)});
        }
        std::vector<Route> unique;
        for (auto it = routes.rbegin(); it != routes.rend(); ++it) {
            if (std::none_of(unique.begin(), unique.end(), [&](const Route& route) {
                    return route.prefix == it->prefix && route.length == it->length;
                })) {
                unique.push_back(*it);
            }
        }
        std::shuffle(unique.begin(), unique.end(), rng);
        switchmodel::lpmtable table;
        for (const auto& route : unique) {
            table.insert(route.prefix, static_cast<uint8_t>(route.length), route.port);
        }
        for (int i = 0; i < 10000; ++i) {
            const uint32_t ip = (i & 1)
                ? rng()
                : (bases[rng() % bases.size()] ^ (rng() & 0x3FFFFU));
            const int expected = reference(unique, ip);
            const int actual = table.lookup(ip);
            ++total;
            if (expected != actual) ++mismatches;
        }
    }
    CHECKF(mismatches == 0, "%ld / %ld lookups differ", mismatches, total);
    return report();
}
