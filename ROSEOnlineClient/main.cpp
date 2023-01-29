#include <MinHook.h>
#include "rose.h"
#include <cstdint>
#include <stdio.h>

#include <Windows.h>

#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>

typedef LRESULT (*fn_wndproc)(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

static fn_present_wrapper orig_present_wrapper;
static fn_wndproc orig_wndproc;

static char* game_base = NULL;
static bool initialized_renderer = false;

static ObjectManager* get_object_manager()
{
    return *((ObjectManager**)(game_base + OFF_GLOBAL_OBJECT_MANAGER));
}

static bool world_to_screen(Vec3 pos, Vec3* out)
{
    auto fn = (fn_world_to_screen)(game_base + OFF_FUNC_WORLD_TO_SCREEN);
    fn(pos.x, pos.y, pos.z, &out->x, &out->y, &out->z);
    return true;
}

//
// Our hook for the game's input handler.
//
static LRESULT CALLBACK hk_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam);
    return orig_wndproc(hwnd, msg, wparam, lparam);
}

//
// Initializes ImGui.
//
static void init_imgui(LPDIRECT3DDEVICE9 device)
{
    auto hwnd = FindWindowA("SDL_app", NULL);

    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(device);

    orig_wndproc = (fn_wndproc)GetWindowLongPtrW(hwnd, GWLP_WNDPROC);
    SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)hk_wndproc);

    initialized_renderer = true;
}

//
// Renders an overlay for entities.
//
static void render_esp_entities()
{
    char name[256];
    auto dl = ImGui::GetForegroundDrawList();
    Vec3 w2s_out;

    if (auto obj_manager = get_object_manager())
    {
        for (auto i = 0; i < ENTITY_COUNT; i++)
        {
            if (auto ent = obj_manager->entities[i])
            {
                ent->get_name(name, sizeof(name));
                if (world_to_screen(ent->scene_position, &w2s_out))
                {
                    dl->AddText(ImVec2(w2s_out.x, w2s_out.y), 0xff0000ff, name);
                }
            }
        }
    }
}

//
// Renders an entity list to the menu.
//
static void render_menu_entities()
{
    char name[256];
    if (auto obj_manager = get_object_manager())
    {
        for (auto i = 0; i < ENTITY_COUNT; i++)
        {
            if (auto ent = obj_manager->entities[i])
            {
                ent->get_name(name, sizeof(name));
                ImGui::Text("%s @ %p", name, ent);
            }
        }
    }
}

//
// Renders our custom menu to ImGui.
//
static void render_menu()
{
    ImGui::Begin("ROSE Online Client");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    
    render_menu_entities();
    render_esp_entities();

    ImGui::End();
}

//
// Our hook for the game function that calls device->Present
//
static void hk_present_wrapper(Renderer* renderer, HWND hwnd)
{
    auto device = renderer->device;
    if (!initialized_renderer)
    {
        init_imgui(device);
    }

    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    render_menu();

    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

    return orig_present_wrapper(renderer, hwnd);
}

//
// Creates and enables a new hook using minhook.
//
static void create_and_enable_hook(void* target, void* detour, void* orig)
{
    if (MH_CreateHook(target, detour, (void**)orig) != MH_OK)
    {
        return;
    }
     
    if (MH_EnableHook(target) != MH_OK)
    {
        return;
    }
}

//
// Installs all hooks needed for the client to function.
//
static void install_hooks()
{
    create_and_enable_hook(game_base + OFF_FUNC_PRESENT, hk_present_wrapper, &orig_present_wrapper);
}

//
// Initializes the client.
//
static void init()
{
    if (MH_Initialize() != MH_OK)
    {
        return;
    }

    game_base = (char*)GetModuleHandleA(NULL);
    install_hooks();
}

BOOL APIENTRY DllMain(HMODULE mod, DWORD reason_for_call, LPVOID reserved)
{
    if (reason_for_call == DLL_PROCESS_ATTACH)
    {
        init();
    }

    return TRUE;
}

