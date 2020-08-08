#pragma once

#define SDL_MAIN_HANDLED

#include <SDL.h>
#include <iostream>
#include <string>

#include "lxecs_static.h"

using std::cerr;
using std::endl;
using namespace lxecs;

class InputComponent : public Component {};

class XYComponent : public Component {
public:
	int m_x;
	int m_y;
	XYComponent() : m_x(0), m_y(0) {};
	XYComponent(int x, int y) : m_x(x), m_y(y) {};
};

class CollisionComponent : public Component {
public:
	SDL_Rect m_rect;
	bool m_active;
	unordered_set<Entity> m_colliding;

	CollisionComponent() {
		m_rect.x = 0;
		m_rect.y = 0;
		m_rect.w = 0;
		m_rect.h = 0;
		m_active = true;
	}

	CollisionComponent(SDL_Rect& rect) : m_rect(rect) {
		m_active = true;
	}
};

class SpriteComponent : public Component {
public:
	static const Uint32 s_transparency_color = 253;
	SDL_Texture* m_texture;
	SDL_Rect m_rect;

	SpriteComponent() {
		m_texture = nullptr;
		m_rect.x = 0;
		m_rect.y = 0;
		m_rect.w = 0;
		m_rect.h = 0;
	}

	SpriteComponent(SDL_Renderer* renderer, const char* sprite_path) {
		SDL_Surface* surface = SDL_LoadBMP(sprite_path);
		SDL_SetColorKey(surface, 1, s_transparency_color);
		m_texture = SDL_CreateTextureFromSurface(renderer, surface);
		SDL_FreeSurface(surface);
		SDL_QueryTexture(m_texture, NULL, NULL, &m_rect.w, &m_rect.h);
	}
};

class RenderSystem : public System<SpriteComponent, XYComponent> {
	static const Uint32 s_render_flags = SDL_RENDERER_ACCELERATED;
public:
	SDL_Renderer* m_renderer;

	RenderSystem() = delete;

	RenderSystem(SDL_Window* window) {
		m_renderer = SDL_CreateRenderer(window, -1, s_render_flags);
	}

	template <typename SpecializedComponentManager>
	void run(SpecializedComponentManager& cm) {
		SDL_RenderClear(m_renderer);
		auto sprite_renderables = select(cm);
		for (Entity e : sprite_renderables) {
			XYComponent& xyc = *cm.get_entity<XYComponent>(e);
			SpriteComponent& sc = *cm.get_entity<SpriteComponent>(e);

			sc.m_rect.x = xyc.m_x;
			sc.m_rect.y = xyc.m_y;

			SDL_RenderCopy(m_renderer, sc.m_texture, nullptr, &sc.m_rect);
		}
		SDL_RenderPresent(m_renderer);
	}
};

class InputSystem : public System<InputComponent, XYComponent> {
public:
	template <typename SpecializedComponentManager>
	void run(SpecializedComponentManager& cm) {
		int n_keys;
		Uint8* keys = const_cast<Uint8*>(SDL_GetKeyboardState(&n_keys));
		int dx = keys[SDL_SCANCODE_RIGHT] - keys[SDL_SCANCODE_LEFT];
		int dy = keys[SDL_SCANCODE_DOWN] - keys[SDL_SCANCODE_UP];
		auto xy_input = select(cm);
		for (Entity e : xy_input) {
			XYComponent& xyc = *cm.get_entity<XYComponent>(e);

			xyc.m_x += dx;
			xyc.m_y += dy;
		}
	}
};

class CollisionSystem : public System<XYComponent, CollisionComponent> {
	static bool test_collision(SDL_Rect& r1, SDL_Rect& r2) {
		bool collides_x = (r1.x >= r2.x) && (r1.x <= r2.x + r2.w);
		collides_x |= (r1.x + r1.w >= r2.x) && (r1.x + r1.w <= r2.x + r2.w);
		bool collides_y = (r1.y >= r2.y) && (r1.y <= r2.y + r2.h);
		collides_y |= (r1.y + r1.h >= r2.y) && (r1.y + r1.h <= r2.y + r2.h);
		return collides_x && collides_y;
	}
public:
	template <typename SpecializedComponentManager>
	void run(SpecializedComponentManager& cm) {
		auto collidables = select(cm);
		for (Entity e : collidables) {
			XYComponent& xyc = *cm.get_entity<XYComponent>(e);
			CollisionComponent& cc = *cm.get_entity<CollisionComponent>(e);

			if (!cc.m_active) {
				continue;
			}

			cc.m_colliding.clear();
			cc.m_rect.x = xyc.m_x;
			cc.m_rect.y = xyc.m_y;

			for (Entity other_e : collidables) {
				if (other_e == e) {
					continue;
				}

				XYComponent& other_xyc = *cm.get_entity<XYComponent>(other_e);
				CollisionComponent& other_cc = *cm.get_entity<CollisionComponent>(other_e);

				if (!cc.m_active) {
					continue;
				}
				other_cc.m_rect.x = other_xyc.m_x;
				other_cc.m_rect.y = other_xyc.m_y;

				if (test_collision(cc.m_rect, other_cc.m_rect)) {
					cc.m_colliding.insert(other_e);
				}
			}
		}
	}
};

void lxecs_demo() {
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		cerr << "SDL_Init failed" << endl;
		return;
	}

	SDL_Window* window = SDL_CreateWindow("lxecs_demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, 0);


	typedef ComponentManager <XYComponent, SpriteComponent, InputComponent, CollisionComponent> SComponentMgr;
	typedef SystemManager<RenderSystem, InputSystem, CollisionSystem> SSystemMgr;
	RenderSystem rs = RenderSystem(window);
	std::tuple<RenderSystem, InputSystem, CollisionSystem> systems = std::make_tuple(rs, InputSystem(), CollisionSystem());
	SSystemMgr sys_mgr(systems);
	typedef World<SSystemMgr, SComponentMgr> SWorld;

	SWorld world(sys_mgr);
	Entity player = world.m_component_mgr.create_entity();
	XYComponent player_xyc(0, 0);
	SpriteComponent player_sc(rs.m_renderer, "C:\\Users\\jonah\\source\\repos\\lxecs_static\\demo\\sprite.bmp");
	world.m_component_mgr.add_to_entity(player, player_xyc);
	world.m_component_mgr.add_to_entity(player, player_sc);
	world.m_component_mgr.add_to_entity(player, InputComponent());
	world.m_component_mgr.add_to_entity(player, CollisionComponent(player_sc.m_rect));

	Entity enemy = world.m_component_mgr.create_entity();
	SpriteComponent enemy_sc(rs.m_renderer, "C:\\Users\\jonah\\source\\repos\\lxecs_static\\demo\\sprite.bmp");
	world.m_component_mgr.add_to_entity(enemy, XYComponent(400, 400));
	world.m_component_mgr.add_to_entity(enemy, enemy_sc);
	world.m_component_mgr.add_to_entity(enemy, CollisionComponent(enemy_sc.m_rect));

	bool close = false;
	while (!close) {
		world.tick();
		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
			case SDL_QUIT:
				close = true;
				break;
			}
		}
	}

	SDL_DestroyWindow(window);
}