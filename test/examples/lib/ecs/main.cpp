#include <ecs/ecs.h>
#include <iostream>

using namespace ecs;

struct a {
	int aa;
};

struct b {
	int aa{5};
};

struct c {
	int aa{-1};
};

struct d {
	int aa{ 15 };
};


struct Example
{
	int a, b, c;
};


int main() {
	init_ecs();
	world wrld{ world::new_world_instance() };
	//entity entt1{ wrld.add_entity() };
	//entity entt2{ wrld.add_entity() };
	//entity entt3{ wrld.add_entity() };
	//entity entt4{ wrld.add_entity() };
	////wrld.remove_entity(entt1);
	//wrld.add_component_to_entity<a>(entt2);
	//wrld.add_component_to_entity<b>(entt2);
	//wrld.add_component_to_entity<d>(entt2);

	//auto [t1, t2] = wrld.get_component_from_entity<a, d>(entt2);
	//t1.aa = 30;

	//wrld.add_component_to_entity<a>(entt1);
	//wrld.add_component_to_entity<d>(entt1);

	//wrld.add_component_to_entity<a>(entt4);
	//wrld.add_component_to_entity<d>(entt4);

	//auto [t3, t4] = wrld.get_component_from_entity<a, d>(entt4);
	//t3.aa = -10;

	//auto flt = wrld.filter_entities<a, d>(entt::exclude<b>);


	//entt1.name() = "entity13";

	//for (auto& enttu : flt) {
	//	enttu.get<d>().aa += enttu.get_uid();
	//	auto [va, vbd] = enttu.get<a, d>();
	//	std::cout << enttu.name() << ' ';
	//	std::cout << va.aa << " " << vbd.aa << '\n';
	//}


	//entt1.duplicate();

	//entt1.name() = "entity1";

	//std::cout << "\n2\n\n";

	//auto flt2 = wrld.filter_entities<a, d>(entt::exclude<b>);

	//for (auto& enttu : flt2) {
	//	auto [va, vbd] = enttu.get<a, d>();
	//	std::cout << enttu.name() << ' ';
	//	std::cout << va.aa << " " << vbd.aa << '\n';
	//}

	//auto entt5 = wrld.add_entity();
	//auto entt6 = wrld.add_entity();

	int i{};

	wrld.add_system([&i](world w) {
		w.add_entity().name() += std::to_string(i++);
		});

	wrld.impl.get_scheduler().compile();

	for (int i{}; i < 16; i++) {
		wrld.update(0.f);
	}

	wrld.impl.get_scheduler().add_system([&i](world w) {
		for (ecs::entity e : w.filter_entities<entity::entity_name_t>()) {
			std::cout << e.name() << "\n";
		}
		}).set_name("print_sys");

	wrld.impl.get_scheduler().add_system([&i](world w) {
		std::cout << "im asys";
		for (ecs::entity e : w.filter_entities<entity::entity_name_t>()) {
			bool a1 =  w.has_any_components_in_entity<a>(e);
			if (a1)
			{
				w.get_component_from_entity<a>(e).aa += 10;
			}
		}
		}).set_name("asys").write<a>();

	wrld.impl.get_scheduler().add_system([&i](world w) {
		std::cout << "bbim asys";
		}).set_name("basys").write<a>();

	wrld.impl.get_scheduler().add_system([&i](world w) {
		std::cout << "f a im asys";
		}).set_name("aa sys").write<Example>();

	wrld.impl.get_scheduler().compile();

	wrld.update(0.f);

	
	shutdown_ecs();
	return 0;
}