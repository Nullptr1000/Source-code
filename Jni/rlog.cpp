#include "rlog.h"
#include <stdio.h>
#include <string.h>

namespace rc {
static Config g_cfg;

const Config& cfg() { return g_cfg; }
void set_mode(int m) { g_cfg.mode = m; LOGI("mode -> %d", m); }

void load_config() {
    FILE* f = fopen("/sdcard/RC/rc.conf", "r");
    if (!f) { LOGI("no config, using defaults"); return; }
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        char k[32] = {0}; int v = 0;
        if (sscanf(line, "%31[^=]=%d", k, &v) == 2) {
            if      (!strcmp(k, "enabled"))  g_cfg.enabled  = v;
            else if (!strcmp(k, "mode"))     g_cfg.mode     = v;
            else if (!strcmp(k, "retry_ms")) g_cfg.retry_ms = v;
        }
    }
    fclose(f);
    LOGI("config: enabled=%d mode=%d retry_ms=%d",
         g_cfg.enabled, g_cfg.mode, g_cfg.retry_ms);
}
}