#include <MinHook.h>
#include "rose.h"

#include <cstdint>
#include <stdio.h>
#include <vector>
#include <Windows.h>

#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>

typedef LRESULT (*fn_wndproc)(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

static fn_present_wrapper orig_present_wrapper;
static fn_wndproc orig_wndproc;

static fn_send_packet orig_send_packet;

static char* game_base = NULL;
static bool initialized_renderer = false;

static bool menu_open = false;
static bool menu_entity_names = false;

static Packet our_packet;
static std::vector<Packet> sent_packets;

static ObjectManager* get_object_manager()
{
    return *((ObjectManager**)(game_base + OFF_GLOBAL_OBJECT_MANAGER));
}

static Network* get_network()
{
    auto ptr = *((uint64_t*)(game_base + OFF_GLOBAL_NETWORK));
    return (Network*)(ptr + OFF_NETWORK_2);
}

static bool world_to_screen(Vec3 pos, Vec3* out)
{
    auto fn = (fn_world_to_screen)(game_base + OFF_FUNC_WORLD_TO_SCREEN);
    fn(pos.x, pos.y, pos.z, &out->x, &out->y, &out->z);
    return true;
}

static void attack(Entity* e, uint16_t ability_id)
{
    if (auto obj_manager = get_object_manager())
    {
        our_packet.hdr.type = pid_ability;
        our_packet.hdr.size = sizeof(PacketBodyAbility);

        memset(&our_packet.ability, 0, sizeof(our_packet.ability));
        our_packet.ability.entity_id = obj_manager->client_to_index_map[e->client_index];
        our_packet.ability.ability_id = ability_id;

        orig_send_packet(get_network(), &our_packet, false);
    }
}

//
// Our hook for the game's input handler.
//
static LRESULT CALLBACK hk_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_KEYDOWN && wparam == VK_INSERT)
    {
        menu_open = !menu_open;
    }

    if (menu_open && ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
    {
        return 0;
    }

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
    char fmt[512];
    auto dl = ImGui::GetBackgroundDrawList();
    Vec3 w2s_out;

    if (auto obj_manager = get_object_manager())
    {
        for (auto i = 0; i < ENTITY_COUNT; i++)
        {
            if (auto ent = obj_manager->entities[i])
            {
                if (menu_entity_names)
                {
                    if (world_to_screen(ent->scene_position, &w2s_out))
                    {
                        ent->get_name(name, sizeof(name));
                        sprintf_s(fmt, "%s: %d: %p", name, (int)obj_manager->client_to_index_map[ent->client_index], ent);

                        dl->AddText(ImVec2(w2s_out.x, w2s_out.y), 0xff0000ff, fmt);
                    }
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
                if (ImGui::BeginChild((ImGuiID)ent, ImVec2(0, 25)))
                {
                    ent->get_name(name, sizeof(name));
                    ImGui::Text("%s @ %p", name, ent);
                    ImGui::SameLine();
                    if (ImGui::Button("Attack"))
                    {
                        attack(ent, AbilityId::aid_dual_scratch);
                    }
                }
                ImGui::EndChild();
            }
        }
    }
}

//
// Renders all sent packets to the menu.
//
static void render_menu_packets()
{
    for (auto& pkt : sent_packets)
    {
        if (ImGui::BeginChild((ImGuiID)&pkt, ImVec2(0, 25)))
        {
            ImGui::Text("Packet %d %p", (int)pkt.hdr.type, pkt.gs.padding);
            ImGui::SameLine();
            if (ImGui::Button("Resend"))
            {
                orig_send_packet(get_network(), &pkt, false);
            }
        }
        ImGui::EndChild();
    }
}

//
// Renders our custom menu to ImGui.
//
static void render_menu()
{
    ImGui::Begin("ROSE Online Client");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    
    if (ImGui::BeginTabBar("MainTabs"))
    {
        if (ImGui::BeginTabItem("Overlay"))
        {
            if (ImGui::Checkbox("Entity Names", &menu_entity_names));
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Entities"))
        {
            render_menu_entities();
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Packets"))
        {
            render_menu_packets();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

//
// Renders our custom overlay to ImGui.
//
static void render_esp()
{
    render_esp_entities();
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
    if (menu_open)
    {
        render_menu();
    }

    render_esp();
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

    return orig_present_wrapper(renderer, hwnd);
}

//
// Our hook for the game function that sends a packet to the server.
//
static void hk_send_packet(Network* network, Packet* packet, bool send_to_world)
{
    Packet copied;
    memcpy(&copied, packet, packet->hdr.size);
    sent_packets.push_back(copied);
    
    orig_send_packet(network, packet, send_to_world);
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
    create_and_enable_hook(game_base + OFF_FUNC_SEND_PACKET, hk_send_packet, &orig_send_packet);
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

