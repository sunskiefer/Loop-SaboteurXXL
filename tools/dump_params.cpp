// dump_params: writes params.json from the engine's own parameter list (src/params.h + src/engine.cpp), so the
// wrapper, the skin and the engine agree on every key, range and option label. Built and run by build.sh on the host.
#include <cstdio>
#include <string>
extern "C" {
#include "engine.h"
int lsx_param_count (void);
}
std::string lsx_param_json (int i, void* inst);

int main()
{
    const mpc_engine_t* e = mpc_engine();
    void* inst = e->create (nullptr);
    std::printf ("{\n \"name\": \"Loop-SaboteurXXL\",\n \"params\": [\n");
    const int n = lsx_param_count();
    for (int i = 0; i < n; ++i)
        std::printf ("  %s%s\n", lsx_param_json (i, inst).c_str(), i + 1 < n ? "," : "");
    std::printf (" ]\n}\n");
    e->destroy (inst);
    return 0;
}
