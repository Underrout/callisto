#include "process_info.h"

namespace callisto {
	std::string ProcessInfo::determineSharedMemoryName() {
		return fmt::format(SHARED_MEMORY_NAME, std::chrono::high_resolution_clock::now().time_since_epoch().count());
	}

	void ProcessInfo::setUpSharedMemory() {
		shared_memory_object = bi::shared_memory_object(
			bi::create_only,
			shared_memory_name.data(),
			bi::read_write
		);

		shared_memory_object.truncate(sizeof(SharedMemory));

		mapped_region = bi::mapped_region(shared_memory_object, bi::read_write);

		void* addr = mapped_region.get_address();

		shared_memory = new (addr) SharedMemory;
	}

	ProcessInfo::ProcessInfo() : shared_memory_name(determineSharedMemoryName()) {
		setUpSharedMemory();
	}

	ProcessInfo::ProcessInfo(ProcessInfo&& other) noexcept {
		shared_memory_name = std::move(other.shared_memory_name);
		shared_memory = other.shared_memory;
		other.shared_memory = nullptr;
		mapped_region = std::move(other.mapped_region);
		shared_memory_object = std::move(other.shared_memory_object);
	}

	ProcessInfo& ProcessInfo::operator=(ProcessInfo&& other) noexcept {
		shared_memory_name = std::move(other.shared_memory_name);
		shared_memory = other.shared_memory;
		other.shared_memory = nullptr;
		mapped_region = std::move(other.mapped_region);
		shared_memory_object = std::move(other.shared_memory_object);

		return *this;
	}

#ifdef _WIN32
	std::optional<HWND> ProcessInfo::getLunarMagicMessageWindowHandle() {
		bi::scoped_lock<bi::interprocess_mutex>(shared_memory->mutex);

		if (!shared_memory->set) {
			return {};
		}

		return (HWND)shared_memory->hwnd;
	}
#endif

	std::optional<uint16_t> ProcessInfo::getLunarMagicVerificationCode() {
		bi::scoped_lock<bi::interprocess_mutex>(shared_memory->mutex);

		if (!shared_memory->set) {
			return {};
		}

		return shared_memory->verification_code;
	}

	std::optional<fs::path> ProcessInfo::getCurrentLunarMagicRomPath() {
		bi::scoped_lock<bi::interprocess_mutex>(shared_memory->mutex);

		if (!shared_memory->set) {
			return {};
		}

		return shared_memory->current_rom;
	}

	const std::string& ProcessInfo::getSharedMemoryName() const {
		return shared_memory_name;
	}

	ProcessInfo::~ProcessInfo() {
		bi::shared_memory_object::remove(shared_memory_name.data());
	}
}
