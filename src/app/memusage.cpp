#include "memusage.h"

#ifdef _WIN32

// ; include order is important
// clang-format off
    #include <windows.h>
    #include <psapi.h>
// clang-format on

uint64_t reformant::memoryUsage() {
    PROCESS_MEMORY_COUNTERS_EX pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc,
                         sizeof(pmc));
    SIZE_T physMemUsedByMe = pmc.WorkingSetSize;
    return physMemUsedByMe;
}

#endif  // _WIN32

#ifdef __linux__

    #include <unistd.h>

    #include <fstream>

uint64_t reformant::memoryUsage() {
    uint64_t rss;
    {
        std::string ignore;
        std::ifstream ifs("/proc/self/stat", std::ios_base::in);
        ifs >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >>
            ignore >> ignore >> ignore >> ignore >> ignore >> ignore >>
            ignore >> ignore >> ignore >> ignore >> ignore >> ignore >>
            ignore >> ignore >> ignore >> ignore >> ignore >> rss;
    }
    uint64_t pageSizeKb = sysconf(_SC_PAGE_SIZE) / 1024_u64;
    return rss * pageSizeKb;
}

#endif  // __linux__

#ifdef __MACH__

    #include <mach/mach.h>

uint64_t reformant::memoryUsage() {
    struct task_basic_info t_info;
    mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;

    if (KERN_SUCCESS != task_info(mach_task_self(), TASK_BASIC_INFO,
                                  (task_info_t)&t_info, &t_info_count)) {
        return 0;
    }
    return t_info.resident_size;
}

#endif