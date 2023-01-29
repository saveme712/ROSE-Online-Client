#pragma once
#include <cstdint>
#include <d3d9.h>

// 40 53 48 83 EC 30 48 8B D9 48 C7 44 24 ? ? ? ? ? 
// \x40\x53\x48\x83\xEC\x30\x48\x8B\xD9\x48\xC7\x44\x24\x00\x00\x00\x00\x00, xxxxxxxxxxxxx?????
#define OFF_FUNC_PRESENT 0x90B250

// sig: 40 53 48 83 EC 40 F3 0F 10 1D ? ? ? ? 
// \x40\x53\x48\x83\xEC\x40\xF3\x0F\x10\x1D\x00\x00\x00\x00, xxxxxxxxxx????
#define OFF_FUNC_WORLD_TO_SCREEN 0x8AB900

#define OFF_GLOBAL_OBJECT_MANAGER 0x10C4090

#define ENTITY_COUNT 4096

enum EntityType : int32_t
{
	et_null,
	et_morph,
	et_item,
	et_collision,
	et_ground,
	et_cnst,
	et_npc,
	et_mob,
	et_avatar,
	et_user,
	et_cart,
	et_cgear,
	et_eventobject,
	et_max
};

#pragma pack(push, 1)
struct Vec3
{
	// 0x0
	float x, y, z;
	// 0xc
};

class Entity
{
public:
	// 0x0
	virtual void v_padding000() = 0;
	// 0x8
	virtual void v_padding001() = 0;
	// 0x10
	virtual void v_padding002() = 0;
	// 0x18
	virtual void v_padding003() = 0;
	// 0x20
	virtual void v_padding004() = 0;
	// 0x28
	virtual void v_padding005() = 0;
	// 0x30
	virtual void v_padding006() = 0;
	// 0x38
	virtual void v_padding007() = 0;
	// 0x40
	virtual int32_t get_entity_type() = 0;

public:
	// 0x8
	char padding000[0x8];
	// 0x10
	Vec3 scene_position;
	// 0x1c

public:
	void get_name(char* name, size_t size);
};

struct ObjectManager
{
	// 0x0
	char padding000[0x22078];
	// 0x22078
	Entity* entities[ENTITY_COUNT];
	// 0x2a078

};

struct Renderer
{
	// 0x0
	char padding000[0x1e8];
	// 0x1e8
	LPDIRECT3DDEVICE9 device;
	// 0x1f0
};
#pragma pack(pop)

typedef void (*fn_world_to_screen)(float x, float y, float z, float* ox, float* oy, float* oz);
typedef void (*fn_present_wrapper)(Renderer* renderer, HWND hwnd);