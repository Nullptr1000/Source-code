#include "imgui.h"
#include "backends/imgui_impl_android.h"
#include "backends/imgui_impl_opengl3.h"
#include "key_system.h"
#include <GLES2/gl2.h>
#include <android/native_window.h>
#include <ctime>
#include <cstdio>

static bool g_imgui_initialized = false;
static bool g_menu_open = false;
static bool g_show_demo = false;
static bool g_unlocked = false;
static char g_key[96] = {};
static char g_status[128] = "Enter your license key.";
static std::int64_t g_expires = 0;

static void rc_load_license() {
    const std::string saved = rc::license::load_key();
    if (saved.empty()) return;
    std::snprintf(g_key, sizeof(g_key), "%s", saved.c_str());
    auto r = rc::license::validate_key(saved);
    if (r.valid) {
        g_unlocked = true;
        g_expires = r.expires_at;
        std::snprintf(g_status, sizeof(g_status), "License active.");
    }
}

extern "C" void rc_imgui_init(ANativeWindow* window) {
    if (g_imgui_initialized) return;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    ImGui::StyleColorsDark();

    ImGui_ImplAndroid_Init(window);
    ImGui_ImplOpenGL3_Init("#version 100");
    g_imgui_initialized = true;
    rc_load_license();
}

extern "C" void rc_imgui_shutdown() {
    if (!g_imgui_initialized) return;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplAndroid_Shutdown();
    ImGui::DestroyContext();
    g_imgui_initialized = false;
}

static void draw_license_window() {
    ImGui::SetNextWindowSize(ImVec2(360, 220), ImGuiCond_Always);
    ImGui::Begin("VAL License", nullptr,
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoSavedSettings);
    ImGui::Text("Enter your license key to continue.");
    ImGui::Separator();
    ImGui::InputText("Key", g_key, sizeof(g_key));
    if (ImGui::Button("Activate", ImVec2(-1, 42))) {
        auto r = rc::license::validate_key(g_key);
        if (r.valid) {
            g_unlocked = true;
            g_expires = r.expires_at;
            rc::license::save_key(g_key);
            std::snprintf(g_status, sizeof(g_status), "License activated.");
        } else {
            std::snprintf(g_status, sizeof(g_status), "%s", r.message.c_str());
        }
    }
    ImGui::TextWrapped("%s", g_status);
    ImGui::TextDisabled("Keys can be issued for 1 day, 1 week, 1 month, or 1 year.");
    ImGui::End();
}

extern "C" void rc_imgui_frame(ANativeWindow* window, int enabled, int mode) {
    if (!g_imgui_initialized) rc_imgui_init(window);
    if (!g_imgui_initialized) return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplAndroid_NewFrame(ANativeWindow_getWidth(window),
                               ANativeWindow_getHeight(window));
    ImGui::NewFrame();

    if (!g_unlocked) {
        draw_license_window();
    } else {
        ImGui::SetNextWindowPos(ImVec2(18, 120), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(64, 64), ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
        ImGui::Begin("##rc_float", nullptr,
                     ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoCollapse);
        if (ImGui::Button("Val", ImVec2(48, 40)))
            g_menu_open = !g_menu_open;
        ImGui::End();
        ImGui::PopStyleVar();

        if (g_menu_open) {
            ImGui::SetNextWindowSize(ImVec2(310, 220), ImGuiCond_FirstUseEver);
            ImGui::Begin("RainbowChams", &g_menu_open);
            ImGui::Text("License active");
            if (g_expires > 0) {
                const std::time_t t = static_cast<std::time_t>(g_expires);
                char buf[64] = {};
                std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M UTC", std::gmtime(&t));
                ImGui::Text("Expires: %s", buf);
            }
            ImGui::Separator();

            bool on = enabled != 0;
            ImGui::Checkbox("Enabled", &on);
            int m = mode;
            ImGui::SliderInt("Mode", &m, 0, 2);

            ImGui::Checkbox("ImGui demo", &g_show_demo);
            if (g_show_demo) ImGui::ShowDemoWindow(&g_show_demo);
            ImGui::End();
        }
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
