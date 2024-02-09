#include "Utils/Filters/SlidingFilter/SlidingFilter.h"
#include "Logger/LogMacro.h"

EDM_STATIC_LOGGER_NAME(s_root_logger, "root");

static edm::utils::SlidingFilter<double> sf{10};

static void init() {
    
}

static void operate() {
    for (int i = 0; i < 40; ++i) {
        sf.push_back(i);
        s_root_logger->debug("pushback {}, avarage {}", i, sf.average());
    }

    sf.clear();
    s_root_logger->info("clear, average {}", sf.average());

    for (int i = 40; i > 0; --i) {
        sf.push_back(i);
        s_root_logger->debug("pushback {}, avarage {}", i, sf.average());
    }

    sf.resize(20);
    s_root_logger->info("resize, size {}, average {}", sf.size(), sf.average());

    for (int i = 0; i < 41; ++i) {
        sf.push_back(i);
        s_root_logger->debug("pushback {}, avarage {}", i, sf.average());
    }
}

static void test_slidingfilter() {
    init();
    operate();
}

int main() {

    test_slidingfilter();

    return 0;
}