#include "Utils/Filters/SlidingCounter/SlidingCounter.h"
#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER(s_root_logger, EDM_LOGGER_ROOT());

edm::util::SlidingCounter<100> sc;

static void test () {
    for (int i = 0; i < 400; ++i) {

        
        // auto t = (i % 4 != 0);
        auto t = (i > 200);
        sc.push_back(t);
        // s_root_logger->debug("push {}, valid rate {}", t, sc.valid_rate());    sc.test_print();

        // if (i == 70) {
        //     sc.resize(10);
        //     s_root_logger->warn("resize to {}", 10);
        // }

        // if (i == 85) {
        //     sc.clear();
        //     s_root_logger->warn("clear");
        // }
    } 


}

// edm::utils::SlidingCounter<0> failed;

int main () {
    test();
    return 0;
}