// The plugin: finds the stellaris-guiexpand host, registers every component as an element, and gets out of the way. DllMain only starts a thread.
// Development: the event Local\stellaris_argon_ui_unload_<pid> unregisters the elements and unloads the DLL (the launcher never unloads plugins).
#include <stddef.h>

#include "argon.h"
#include "stellaris_gui_client.h"
#include "stellaris_gui_imgui.hpp"

namespace argon {
namespace {

HMODULE g_module;
FILE* g_log;
const StlGuiApi* g_api;
int g_handles[64];
int g_count;

void Log(const char* fmt, ...) {
    if (!g_log) return;
    SYSTEMTIME t;
    GetLocalTime(&t);
    fprintf(g_log, "[%02d:%02d:%02d] ", t.wHour, t.wMinute, t.wSecond);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(g_log, fmt, ap);
    va_end(ap);
    fputc('\n', g_log);
    fflush(g_log);
}

// Every component is entered through this: bind our own ImGui to the engine's context, set the frame state, draw.
void Entry(const StlGuiCallbackCtx* ctx, const StlGuiNode* node, void* user) {
    if (!StlGuiBindImGui(ctx)) {
        static bool said = false;
        if (!said) {
            said = true;
            Log("not drawing: %s", StlGuiBindFailure());
        }
        return;
    }
    g_ctx = ctx;
    ImGui::PushID(node);  // several entries of the same component in one window must not share widget ids
    ((ComponentFn)user)(node);
    ImGui::PopID();
}

void RegisterAll() {
    size_t n = 0;
    const ComponentDef* defs = Components(&n);
    for (size_t i = 0; i < n && g_count < 64; ++i) {
        StlGuiElementDesc d;
        memset(&d, 0, sizeof(d));
        d.size = sizeof(d);
        d.name = defs[i].name;
        d.provider = "stellaris-argon-ui";
        d.draw = Entry;
        d.user = (void*)defs[i].draw;
        g_handles[g_count] = g_api->register_element(&d);
        Log("element %s: handle %d", defs[i].name, g_handles[g_count]);
        ++g_count;
    }
}

DWORD WINAPI Worker(LPVOID) {
    char path[MAX_PATH], name[64];
    GetModuleFileNameA(g_module, path, MAX_PATH);
    char* slash = strrchr(path, '\\');
    if (slash) *(slash + 1) = 0;
    std::string dir = path;
    CreateDirectoryA((dir + "logs").c_str(), nullptr);
    g_log = fopen((dir + "logs\\stellaris_argon_ui.log").c_str(), "a");
    Log("loaded; looking for the stellaris-guiexpand host");
    sprintf(name, "Local\\stellaris_argon_ui_unload_%lu", GetCurrentProcessId());
    HANDLE ev = CreateEventA(nullptr, TRUE, FALSE, name);
    ResetEvent(ev);  // a named event outlives the instance that made it
    DWORD waited = WAIT_TIMEOUT;
    for (int i = 0; i < 4800 && !g_api; ++i) {  // about twenty minutes, then give up
        g_api = stl_gui_try_connect(STL_GUI_HOST_DLL, STL_GUI_API_VERSION);
        if (g_api) break;
        waited = WaitForSingleObject(ev, 250);
        if (waited != WAIT_TIMEOUT) break;
    }
    if (g_api && waited == WAIT_TIMEOUT) {
        // the element registry was appended after the first release of the interface
        if (g_api->size < offsetof(StlGuiApi, set_theme) + sizeof(void*)) {
            Log("this host is too old for elements: update stellaris-guiexpand");
        } else {
            Log("host found (api version %u, built for exe 0x%08X)", g_api->version, g_api->game_exe_timestamp);
            RegisterAll();
            while (WaitForSingleObject(ev, 500) == WAIT_TIMEOUT) {}
            waited = WAIT_OBJECT_0;
        }
    }
    if (waited == WAIT_OBJECT_0) {
        for (int i = 0; i < g_count; ++i)
            if (g_handles[i]) g_api->unregister_element(g_handles[i]);
        Log("unregistered, leaving");
        Sleep(500);
    }
    if (g_log) fclose(g_log);
    CloseHandle(ev);
    if (waited == WAIT_OBJECT_0) FreeLibraryAndExitThread(g_module, 0);
    return 0;
}

}  // namespace

const ComponentDef* Components(size_t* count) {
    static std::vector<ComponentDef> all;
    if (all.empty()) {
        AddCoreComponents(all);
        AddShellComponents(all);
        AddPagesComponents(all);
    }
    *count = all.size();
    return all.data();
}

}  // namespace argon

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        argon::g_module = module;
        HANDLE t = CreateThread(nullptr, 0, argon::Worker, nullptr, 0, nullptr);
        if (t) CloseHandle(t);
    }
    return TRUE;
}
