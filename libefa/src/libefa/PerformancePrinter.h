#include "includes.h"
#include "ConnectionContext.h"

namespace libefa
{
    class PerformancePrinter
    {
        static char *stringSize(char *str, uint64_t size);

        static char *stringCount(char *str, size_t size, uint64_t cnt);

    public:
        static void print(char *name, int tsize, int sent, int acked, uint64_t start, uint64_t end, int xfers_per_iter);
    };
}