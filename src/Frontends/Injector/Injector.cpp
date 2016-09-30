#include "Injector.hpp"
#include <random>
#include <thread>
#include <vector>

constexpr uint64_t total_injected_events = (3000000000); 

namespace Injector
{

auto inject_random(uint64_t num_events, int be_idx) -> void
{
    std::mt19937_64 rng;
    rng.seed(std::random_device()());
    std::uniform_int_distribution<std::mt19937_64::result_type> dist5(1,5);

    for (uint64_t i = 0; i < num_events; ++i) {
        auto res = dist5(rng);
        switch(res)
        {
          case 1:
            Sigil::instance().addEvent(SglCompEv{ 0,0,0,0, },
                                       be_idx);
            break;
          case 2:
            Sigil::instance().addEvent(SglMemEv{ 0,0,0, },
                                       be_idx);
            break;
          case 3:
            Sigil::instance().addEvent(SglCFEv{ 0, },
                                       be_idx);
            break;
          case 4:
            Sigil::instance().addEvent(SglSyncEv{ 0,0, },
                                       be_idx);
            break;
          case 5:
            Sigil::instance().addEvent(SglCxtEv{ 0,{0}, },
                                       be_idx);
            break;
        }
    } 
}


auto inject_static(uint64_t num_events, int be_idx) -> void
{
    uint64_t events_injected = 0;

    while(events_injected < num_events)
    {
        Sigil::instance().addEvent(SglCompEv{ 0,0,0,0, },
                                   be_idx);
        Sigil::instance().addEvent(SglMemEv{ 0,0,0, },
                                   be_idx);
        Sigil::instance().addEvent(SglCFEv{ 0, },
                                   be_idx);
        Sigil::instance().addEvent(SglSyncEv{ 0,0, },
                                   be_idx);
        Sigil::instance().addEvent(SglCxtEv{ 0,{0}, },
                                   be_idx);

        events_injected += 5;
    }


}


auto inject_events(bool rand, uint64_t num_events, int be_idx) -> void
{
    if(rand == true)
    {
        inject_random(num_events, be_idx);
    }
    else if(rand == false)
    {
        inject_static(num_events, be_idx);
    }
}


auto start(const std::vector<std::string> &user_exec,
           const std::vector<std::string> &args,
           const uint16_t num_threads,
           const std::string &instance_id
           ) -> void
{
    bool rand = false;
    for(auto &arg: args)
    {
        if(arg.compare("-r") == 0)
        {
            rand = true;
        }
    }

    std::vector<std::thread> producer_threads;
    uint64_t num_events = total_injected_events/num_threads;

    for(int i=0; i<num_threads; ++i)
    {
        producer_threads.emplace_back(inject_events, rand, num_events, i);
    }

    for(int i=0; i<num_threads; ++i)
    {
        producer_threads[i].join();
    }
}

}; //end namespace RandomInj
