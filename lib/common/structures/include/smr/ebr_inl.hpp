#ifndef SMR_EPOCH_BASED_RECLAIMATION_INLINE_IMPLEMENTATION_HPP
#define SMR_EPOCH_BASED_RECLAIMATION_INLINE_IMPLEMENTATION_HPP

inline ebr_thread_record::~ebr_thread_record() {
	ebr_node_record* head = m_retired_head.exchange(nullptr, std::memory_order_acq_rel);
	while (head) {
		auto tmp = head->m_next;
		delete head;
		head = tmp;
	}
}

inline ebr_tls_thread_record_registry::~ebr_tls_thread_record_registry() {
	for (auto& [id, record_pair] : m_record_registry) {
		record_pair.second->release_thread_record(record_pair.first);
	}
}

inline void ebr_tls_thread_record_registry::tls_erase(ebr_control* e) {
	auto& reg{ tls_reg() };
	if (auto it{ reg.m_record_registry.find(e->m_instance_id) }; it != reg.m_record_registry.end()) {
		it->second.second->release_thread_record(it->second.first);
		reg.m_record_registry.erase(it);
	}
}

inline ebr_tls_thread_record_registry& ebr_tls_thread_record_registry::tls_reg() {
	static thread_local ebr_tls_thread_record_registry tls_registry{};
	return tls_registry;
}

inline ebr_thread_record& ebr_tls_thread_record_registry::tls_get(ebr_control* e) {
	auto& reg{ tls_reg()};
	if (auto res{ reg.m_record_registry.find(e->m_instance_id) }; res != reg.m_record_registry.end()) {
		return *res->second.first; //assumes ebr control never gets destroy before non owning threads;
	}
	return *reg.m_record_registry.emplace(e->m_instance_id, std::move(ebr_thread_record_pair{e->register_thread_record(), e})).first->second.first;
}

inline void vector_impl::ebr_control::enter(std::size_t idx) { //suboptimal, one extra lock
	ebr_thread_record* record_ptr{nullptr};
	{
		std::lock_guard lg{ m_record_slots_mtx };
		assert(idx < m_record_slots.size() && "subscript out of bounds");
		record_ptr = m_record_slots[idx].get();
	}
	uint64_t epoch = m_current_epoch.load(std::memory_order_acquire);
	record_ptr->m_local_epoch.store(epoch, std::memory_order_relaxed);
	record_ptr->m_active.store(true, std::memory_order_release);
}

inline void vector_impl::ebr_control::enter(ebr_thread_record& rec) {
	uint64_t epoch = m_current_epoch.load(std::memory_order_acquire);
	rec.m_local_epoch.store(epoch, std::memory_order_relaxed);
	rec.m_active.store(true, std::memory_order_release);
}

inline bool vector_impl::ebr_control::try_advance_epoch() {
	std::vector<ebr_thread_record*> snapshot{};
	{
		std::lock_guard lg{ m_record_slots_mtx };
		snapshot.reserve(m_record_slots.size());
		for (std::unique_ptr<ebr_thread_record>& uptr : m_record_slots) {
			snapshot.emplace_back(uptr.get());
		}
	}

	std::size_t curr_epoch = m_current_epoch.load(std::memory_order_relaxed);
	for (ebr_thread_record* rec : snapshot) {
		if (rec->m_active.load(std::memory_order_acquire) && rec->m_local_epoch.load(std::memory_order_relaxed) < curr_epoch) {
			return false; //previous epoch in use
		}
	}
	m_current_epoch.fetch_add(1, std::memory_order_release);
	return true;
}

inline void vector_impl::ebr_control::retire(deleter del) {
	ebr_thread_record& rec{ get_thread_record() };

	uint64_t epoch = m_current_epoch.load(std::memory_order_acquire);
	ebr_node_record* node = new ebr_node_record{ ebr_record{del, epoch}, nullptr };

	//cas until retired list updates
	ebr_node_record* head = rec.m_retired_head.load(std::memory_order_relaxed);
	do {
		node->m_next = head;
	} while (!rec.m_retired_head.compare_exchange_weak(head, node, std::memory_order_release, std::memory_order_relaxed));

	rec.m_retired_count.fetch_add(1, std::memory_order_relaxed);
	//try_advance_epoch();
	collect();
}

inline void vector_impl::ebr_control::collect() {
	ebr_thread_record& rec{ get_thread_record() };
	try_advance_epoch();
	collect_thread(rec);
}

inline std::size_t vector_impl::ebr_control::min_epoch() {
	std::vector<ebr_thread_record*> snapshot{};
	{
		std::lock_guard lg{ m_record_slots_mtx };
		snapshot.reserve(m_record_slots.size());
		for (std::unique_ptr<ebr_thread_record>& uptr : m_record_slots) {
			snapshot.emplace_back(uptr.get());
		}
	}

	std::size_t minepoch{ std::numeric_limits<std::size_t>::max() };
	for (ebr_thread_record* rec : snapshot) {
		if (rec->m_active.load(std::memory_order_acquire)) {
			std::size_t tepoch = rec->m_local_epoch.load(std::memory_order_relaxed);
			if (tepoch < minepoch) minepoch = tepoch;
		}
	}
	return minepoch == std::numeric_limits<std::size_t>::max() ? m_current_epoch.load(std::memory_order_acquire) : minepoch;
}

inline void vector_impl::ebr_control::collect_thread(ebr_thread_record& rec) {
	std::size_t min_ep{ min_epoch() };

	ebr_node_record* head = rec.m_retired_head.exchange(nullptr, std::memory_order_acq_rel);

	std::size_t list_count{};
	ebr_node_record* unclaimable = nullptr;
	while (head) {
		ebr_node_record* nd = head;
		head = head->m_next;

		if (nd->m_record.m_record_epoch < min_ep) {
			nd->m_record.fn_release();
			delete nd;
		}
		else {
			nd->m_next = unclaimable;
			unclaimable = nd;
			list_count++;
		}
	}

	//reverse build list
	if (unclaimable) {
		ebr_node_record* h = rec.m_retired_head.load(std::memory_order_relaxed);
		do {
			ebr_node_record* tail = unclaimable;
			while (tail->m_next) 
				tail = tail->m_next;
			tail->m_next = h;
		} while (!rec.m_retired_head.compare_exchange_weak(h, unclaimable, std::memory_order_release, std::memory_order_relaxed));
	}
	rec.m_retired_count.store(list_count, std::memory_order_relaxed);
}
inline void vector_impl::ebr_control::exit(std::size_t idx) {
	ebr_thread_record* record_ptr{ nullptr };
	{
		std::lock_guard lg{ m_record_slots_mtx };
		assert(idx < m_record_slots.size() && "subscript out of bounds");
		record_ptr = m_record_slots[idx].get();
	}
	record_ptr->m_active.store(false, std::memory_order_release);
}

inline void vector_impl::ebr_control::exit(ebr_thread_record& rec){
	rec.m_active.store(false, std::memory_order_release);
}

inline std::size_t vector_impl::ebr_control::register_thread_record_idx() {
	std::size_t pos{};
	{
		std::lock_guard lg{ m_record_slots_mtx };
		pos = m_record_slots.size();
		m_record_slots.emplace_back(std::make_unique<ebr_thread_record>());
	}
	return pos;
}

inline ebr_thread_record* vector_impl::ebr_control::register_thread_record() {
	std::lock_guard lg{ m_record_slots_mtx };
	return m_record_slots.emplace_back(std::make_unique<ebr_thread_record>()).get();
}

inline void vector_impl::ebr_control::release_thread_record(std::size_t sz) {
	std::lock_guard lg{ m_record_slots_mtx };
	m_record_slots[sz].swap(m_record_slots.back());
	m_record_slots.pop_back();
	//TODO:: fix indexing
}

inline void vector_impl::ebr_control::release_thread_record(ebr_thread_record* ptr) {
	std::lock_guard lg{ m_record_slots_mtx };
	std::find_if(m_record_slots.begin(), m_record_slots.end(), [ptr](std::unique_ptr<ebr_thread_record> const& uptr) {return uptr.get() == ptr; })->swap(m_record_slots.back());
	m_record_slots.pop_back();
}

inline ebr_thread_record& vector_impl::ebr_control::get_thread_record(std::size_t sz) {
	return *m_record_slots[sz];
}

inline ebr_thread_record& vector_impl::ebr_control::get_thread_record() {
	return ebr_tls_thread_record_registry::tls_get(this);
}


inline ebr_guard::ebr_guard(ebr_control& e)
	: m_ebrcontrol{ e }, m_record{ ebr_tls_thread_record_registry::tls_get(&e)} {
	m_ebrcontrol.enter(m_record);
}

inline ebr_guard::~ebr_guard() {
	m_ebrcontrol.exit(m_record);
}

inline std::atomic_size_t& ebr_control::get_instance_id() {
	static std::atomic_size_t id{};
	return id;
}

inline vector_impl::ebr_control::ebr_control()
	: m_instance_id{ get_instance_id().fetch_add(1, std::memory_order_relaxed) } {}

inline vector_impl::ebr_control::~ebr_control() {
	ebr_tls_thread_record_registry::tls_erase(this); //remove owner thread reference, if any
}

#endif