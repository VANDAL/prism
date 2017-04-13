#ifndef STGEN_ADDRSET_H
#define STGEN_ADDRSET_H

#include "STTypes.hpp" // Addr, TID, EID
#include "MemoryPool.h"
#include <set>

namespace STGen
{

struct AddrSet
{
    /* Helper class to track unique ranges of addresses */

    using AddrRange = std::pair<Addr, Addr>;
    using Ranges = std::multiset<AddrRange, std::less<AddrRange>, MemoryPool<AddrRange>>;

    AddrSet(){}
    AddrSet(const AddrRange &range) { ms.insert(range); }
    AddrSet(const AddrSet &other) { ms = other.ms; }
    AddrSet &operator=(const AddrSet &) = delete;
    const Ranges &get() const { return ms; }
    void clear() { ms.clear(); }


    void insert(const AddrRange &range)
    {
        /* A range of addresses is specified by the pair.
         * This call inserts that range and merges existing ranges
         * in order to keep the set of addresses unique */
        /* TODO(someday) Someone please clean up flow control */

        assert(range.first <= range.second);

        /* insert if this is the first addr */
        if (ms.empty() == true)
        {
            ms.insert(range);
            return;
        }

        /* get the first addr pair that is not less than range */
        /* see http://en.cppreference.com/w/cpp/utility/pair/operator_cmp */
        auto it = ms.lower_bound(range);

        if (it != ms.cbegin())
        {
            if (it == ms.cend())
                /* if no address range starts at a higher address,
                 * check the last element */
            {
                it = --ms.cend();
            }
            else
                /* check if the previous addr pair overlaps with range */
            {
                --it;

                if (range.first > it->second + 1)
                {
                    ++it;
                }
            }
        }

        if (range.first == it->second + 1)
        {
            /* extend 'it' by 'range'; recheck, may overrun other addresses */
            auto tmp = std::make_pair(it->first, range.second);
            ms.erase(it);
            insert(tmp);
        }
        else if (range.second + 1 == it->first)
        {
            /* extend 'it' by 'range'; recheck, may overrun other addresses */
            auto tmp = std::make_pair(range.first, it->second);
            ms.erase(it);
            insert(tmp);
        }
        else if (range.first > it->second)
        {
            /* can't merge, just insert (at end) */
            ms.insert(range);
        }
        else if (range.first >= it->first)
        {
            if (range.second > it->second)
                /* extending 'it' to the end of 'range' */
            {
                /* merge, delete, and recheck, may overrun other addresses */
                auto tmp = std::make_pair(it->first, range.second);
                ms.erase(it);
                insert(tmp);
            }

            /* else do not insert; 'it' encompasses 'range' */
        }
        else /* if (range.first < it->first) */
        {
            if (range.second < it->first)
                /* no overlap */
            {
                /* nothing to merge */
                ms.insert(range);
            }
            else if (range.second <= it->second)
                /* begin address is extended */
            {
                /* merge, delete, and insert; no need to recheck */
                Addr second = it->second;
                ms.erase(it);
                ms.emplace(range.first, second);
            }
            else /* if(range.second > it->second) */
                /* 'range' encompasses 'it' */
            {
                /* delete old range and insert bigger range; recheck */
                ms.erase(it);
                insert(range);
            }
        }
    }

  private:
    Ranges ms;
};

}; //end namespace STGen

#endif
