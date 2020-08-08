#pragma once

#include <tuple>
#include <vector>
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace lxecs {

	using std::tuple;
	using std::vector;
	using std::unordered_map;
	using std::unordered_set;
	using std::optional;
	using std::reference_wrapper;
	using std::nullopt;
	using std::get;

	using Entity = long;

	class Component {};

	template <typename... Components>
	class System;

	template <typename... Components>
	class ComponentManager;

	template <typename... Systems>
	class SystemManager;

	//TODO: Concepts

	template <typename T>
	unordered_set<T> set_intersection(unordered_set<T>& a, unordered_set<T>& b) {
		unordered_set<T> result;
		auto it = a.begin();
		for (auto& entry : b) {
			if (a.count(entry) > 0) {
				result.insert(entry);
			}
		}
		return result;
	}

	template
		unordered_set<Entity> set_intersection(unordered_set<Entity>& a, unordered_set<Entity>& b);

	template <typename... Components>
	class System {
	public:
		template <typename SpecializedComponentManager>
		void run(SpecializedComponentManager& cm) {};

		template <typename SpecializedComponentManager>
		static unordered_set<Entity> select(SpecializedComponentManager& cm) {
			return cm.select<Components...>();
		}
	};

	template <typename... Components>
	class ComponentManager {
	public:
		tuple<unordered_map<Entity, Components>...> m_components;
		Entity m_next_entity = 0;

		template <typename Component>
		void add_to_entity(Entity e, Component c) {
			get<unordered_map<Entity, Component>>(m_components)[e] = c;
		}

		template <typename Component>
		optional<reference_wrapper<Component>> get_entity(Entity e) {
			unordered_map<Entity, Component>& storage = get<unordered_map<Entity, Component>>(m_components);
			auto it = storage.find(e);
			if (it != storage.end()) {
				Component& c = it->second;
				return { c };
			}
			return nullopt;
		}

		Entity create_entity() {
			return m_next_entity++;
		}

		template <typename Component>
		unordered_set<Entity> __select() {
			unordered_set<Entity> results;
			unordered_map<Entity, Component>& storage = get<unordered_map<Entity, Component>>(m_components);
			for (auto& entry : storage) {
				results.insert(entry.first);
			}
			return results;
		}

		template <typename Component>
		void __select_into(unordered_set<Entity>& results, bool& first) {
			auto selection = __select<Component>();
			if (first) {
				results = selection;
				first = false;
			}
			results = set_intersection(results, selection);
		}

		template <typename... QueryComponents>
		unordered_set<Entity> select() {
			unordered_set<Entity> result;
			bool first = true;
			(..., __select_into<QueryComponents>(result, first));
			return result;
		}
	};

	template <typename... Systems>
	class SystemManager {
		tuple<Systems...> m_systems;
	public:
		SystemManager() = delete;

		//initializer list to dodge the default constructors
		SystemManager(tuple<Systems...> systems) : m_systems(systems) {}

		template <typename System, typename SpecializedComponentManager>
		void __dispatch(SpecializedComponentManager& cm) {
			get<System>(m_systems).run<SpecializedComponentManager>(cm);
		}

		template <typename SpecializedComponentManager>
		void dispatch(SpecializedComponentManager& cm) {
			(..., __dispatch<Systems, SpecializedComponentManager>(cm));
		}
	};

	template <typename... Resources>
	class ResourceManager {
		tuple<Resources...> m_resources;
	public:
		template <typename Resource>
		Resource& get() {
			return get<Resource>(m_resources);
		}
	};

	template <typename SpecializedSystemManager, typename SpecializedComponentManager>
	class World {
	public:
		SpecializedComponentManager m_component_mgr;
		SpecializedSystemManager m_system_mgr;

		World(SpecializedSystemManager sys_mgr) : m_system_mgr(sys_mgr) {}

		void tick() {
			m_system_mgr.dispatch(m_component_mgr);
		}
	};

};