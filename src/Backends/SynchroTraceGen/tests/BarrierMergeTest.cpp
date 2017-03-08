#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "SynchroTraceGen/BarrierMerge.hpp"

using namespace STGen;

bool equal(const BarrierStats &l, const BarrierStats &r)
{
    return (l.iops == r.iops &&
            l.flops == r.flops &&
            l.instrs == r.instrs &&
            l.memAccesses == r.memAccesses &&
            l.locks == r.locks);
}

TEST_CASE("merge empty barriers", "[EmptyBarriers]")
{
    SECTION("two empty barriers")
    {
        AllBarriersStats T1;
        AllBarriersStats T2;
        BarrierMerge::merge(T1, T2);
        REQUIRE(T1.empty());
        REQUIRE(T2.empty());
    }

    SECTION("one barrier stats with an empty barriers")
    {
        AllBarriersStats T1;
        AllBarriersStats T2;
        BarrierStats stats;
        stats.iops = 10;
        stats.flops = 20;
        stats.instrs = 30;
        stats.memAccesses = 40;
        stats.locks = 50;
        T2.push_back(std::make_pair(1000, stats));

        BarrierMerge::merge(T2, T1);
        REQUIRE(T1.empty() == false);
        REQUIRE(T1.size() == 1);
        REQUIRE(T1.front().first == 1000);
        REQUIRE(equal(T1.front().second, stats));

        REQUIRE(T2.empty() == false);
        REQUIRE(T2.size() == 1);
        REQUIRE(T2.front().first == 1000);
        REQUIRE(equal(T2.front().second, stats));
    }

    SECTION("an empty barrier stats with a non empty")
    {
        AllBarriersStats T1;
        AllBarriersStats T2;
        BarrierStats stats;
        stats.iops = 10;
        stats.flops = 20;
        stats.instrs = 30;
        stats.memAccesses = 40;
        stats.locks = 50;
        T2.push_back(std::make_pair(1000, stats));

        BarrierMerge::merge(T1, T2);
        REQUIRE(T1.empty());
        REQUIRE(T2.size() == 1);
        REQUIRE(T2.front().first == 1000);
        REQUIRE(equal(T2.front().second, stats));
    }
}

TEST_CASE("merge of one repeated barrier", "[SimpleBarriers]")
{
    SECTION("one set of repeated barriers")
    {
        constexpr Addr B1 = 1000;

        AllBarriersStats T1;
        AllBarriersStats T2;
        AllBarriersStats T3;
        AllBarriersStats merged;
        BarrierStats stats;

        stats.iops = 10;
        stats.flops = 10;
        stats.instrs = 10;
        stats.memAccesses = 10;
        stats.locks = 10;
        T1.push_back(std::make_pair(B1, stats));
        T1.push_back(std::make_pair(B1, stats));
        T1.push_back(std::make_pair(B1, stats));

        stats.iops = 20;
        stats.flops = 20;
        stats.instrs = 20;
        stats.memAccesses = 20;
        stats.locks = 20;
        T2.push_back(std::make_pair(B1, stats));
        T2.push_back(std::make_pair(B1, stats));
        T2.push_back(std::make_pair(B1, stats));

        stats.iops = 30;
        stats.flops = 30;
        stats.instrs = 30;
        stats.memAccesses = 30;
        stats.locks = 30;
        T3.push_back(std::make_pair(B1, stats));
        T3.push_back(std::make_pair(B1, stats));

        /* make our third barrier different */
        stats.iops = 40;
        stats.flops = 40;
        stats.instrs = 40;
        stats.memAccesses = 40;
        stats.locks = 40;
        T3.push_back(std::make_pair(B1, stats));

        BarrierMerge::merge(T1, merged);
        BarrierMerge::merge(T2, merged);
        BarrierMerge::merge(T3, merged);
        REQUIRE(merged.size() == 3);

        auto it = merged.begin(); // first barrier
        REQUIRE(it->first == B1);
        REQUIRE(it->second.iops == 60);
        REQUIRE(it->second.flops == 60);
        REQUIRE(it->second.instrs == 60);
        REQUIRE(it->second.memAccesses == 60);
        REQUIRE(it->second.locks == 60);

        ++it; // second barrier
        REQUIRE(it->first == B1);
        REQUIRE(it->second.iops == 60);
        REQUIRE(it->second.flops == 60);
        REQUIRE(it->second.instrs == 60);
        REQUIRE(it->second.memAccesses == 60);
        REQUIRE(it->second.locks == 60);

        ++it; // third barrier
        REQUIRE(it->first == B1);
        REQUIRE(it->second.iops == 70);
        REQUIRE(it->second.flops == 70);
        REQUIRE(it->second.instrs == 70);
        REQUIRE(it->second.memAccesses == 70);
        REQUIRE(it->second.locks == 70);

        ++it;
        REQUIRE(it == merged.end());
    }

    SECTION("two sets of repeated barriers")
    {
        constexpr Addr B1 = 1000;
        constexpr Addr B2 = 1001;

        AllBarriersStats T1;
        AllBarriersStats T2;
        AllBarriersStats T3;
        AllBarriersStats merged;
        BarrierStats stats;

        stats.iops = 10;
        T1.push_back(std::make_pair(B1, stats));
        T1.push_back(std::make_pair(B1, stats));
        T1.push_back(std::make_pair(B1, stats));
        stats.iops = 100;
        T1.push_back(std::make_pair(B2, stats));
        T1.push_back(std::make_pair(B2, stats));
        T1.push_back(std::make_pair(B2, stats));

        stats.iops = 20;
        T2.push_back(std::make_pair(B1, stats));
        T2.push_back(std::make_pair(B1, stats));
        T2.push_back(std::make_pair(B1, stats));
        stats.iops = 200;
        T2.push_back(std::make_pair(B2, stats));
        T2.push_back(std::make_pair(B2, stats));
        T2.push_back(std::make_pair(B2, stats));

        stats.iops = 30;
        T3.push_back(std::make_pair(B1, stats));
        T3.push_back(std::make_pair(B1, stats));
        T3.push_back(std::make_pair(B1, stats));
        stats.iops = 300;
        T3.push_back(std::make_pair(B2, stats));
        T3.push_back(std::make_pair(B2, stats));
        T3.push_back(std::make_pair(B2, stats));

        BarrierMerge::merge(T1, merged);
        BarrierMerge::merge(T2, merged);
        BarrierMerge::merge(T3, merged);

        REQUIRE(merged.size() == 6);

        auto it = merged.begin();
        REQUIRE(it->first == B1);
        REQUIRE(it->second.iops == 60);
        ++it;
        REQUIRE(it->first == B1);
        REQUIRE(it->second.iops == 60);
        ++it;
        REQUIRE(it->first == B1);
        REQUIRE(it->second.iops == 60);

        ++it; // next barrier id
        REQUIRE(it->first == B2);
        REQUIRE(it->second.iops == 600);
        ++it;
        REQUIRE(it->first == B2);
        REQUIRE(it->second.iops == 600);
        ++it;
        REQUIRE(it->first == B2);
        REQUIRE(it->second.iops == 600);

        ++it;
        REQUIRE(it == merged.end());
    }
}

TEST_CASE("more complex cases", "[ComplexBarriers]")
{
    SECTION("Example 1")
    {
        /* Test Case:
         *
         * T1  T2  T3  T4
         * B1      B1
         * B2  B2  B2  B2
         * B2  B2  B2  B2
         * B3      B3
         * B2  B2  B2  B2
         * B4          B4
         */

        /* thread 1 */
        constexpr Addr B1 = 1;
        constexpr Addr B2 = 2;
        constexpr Addr B3 = 3;
        constexpr Addr B4 = 4;

        AllBarriersStats T1;
        AllBarriersStats T2;
        AllBarriersStats T3;
        AllBarriersStats T4;
        AllBarriersStats merged;
        BarrierStats stats;
        stats.iops = 10;

        T1.push_back(std::make_pair(B1, stats));
        T1.push_back(std::make_pair(B2, stats));
        T1.push_back(std::make_pair(B2, stats));
        T1.push_back(std::make_pair(B3, stats));
        T1.push_back(std::make_pair(B2, stats));
        T1.push_back(std::make_pair(B4, stats));

        T2.push_back(std::make_pair(B2, stats));
        T2.push_back(std::make_pair(B2, stats));
        T2.push_back(std::make_pair(B2, stats));

        T3.push_back(std::make_pair(B1, stats));
        T3.push_back(std::make_pair(B2, stats));
        T3.push_back(std::make_pair(B2, stats));
        T3.push_back(std::make_pair(B3, stats));
        T3.push_back(std::make_pair(B2, stats));

        T4.push_back(std::make_pair(B2, stats));
        T4.push_back(std::make_pair(B2, stats));
        T4.push_back(std::make_pair(B2, stats));
        T4.push_back(std::make_pair(B4, stats));

        BarrierMerge::merge(T4, merged);
        BarrierMerge::merge(T3, merged);
        BarrierMerge::merge(T2, merged);
        BarrierMerge::merge(T1, merged);

        REQUIRE(merged.size() == 6);

        auto it = merged.begin();
        REQUIRE(it->first == B1);
        REQUIRE(it->second.iops == 20);

        ++it;
        REQUIRE(it->first == B2);
        REQUIRE(it->second.iops == 40);

        ++it;
        REQUIRE(it->first == B2);
        REQUIRE(it->second.iops == 40);

        ++it;
        REQUIRE(it->first == B3);
        REQUIRE(it->second.iops == 20);

        ++it;
        REQUIRE(it->first == B2);
        REQUIRE(it->second.iops == 40);

        ++it;
        REQUIRE(it->first == B4);
        REQUIRE(it->second.iops == 20);

        ++it;
        REQUIRE(it == merged.end());
    }
}
