#include "process_info.h"

namespace callisto {
	std::string ProcessInfo::determineSharedMemoryName(const bp::pid_t lunar_magic_pid) {
		return fmt::format(SHARED_MEMORY_NAME, lunar_magic_pid);
	}

	void ProcessInfo::setUpSharedMemory() {
		shared_memory_object = bi::shared_memory_object(
			bi::open_or_create,
			shared_memory_name.data(),
			bi::read_write
		);

		shared_memory_object.truncate(sizeof(SharedMemory));

		mapped_region = bi::mapped_region(shared_memory_object, bi::read_write);

		void* addr = mapped_region.get_address();

		shared_memory = new (addr) SharedMemory;
	}

	ProcessInfo::ProcessInfo() {};

	void ProcessInfo::setPid(const bp::pid_t lunar_magic_pid) {
		shared_memory_name = determineSharedMemoryName(lunar_magic_pid);
		setUpSharedMemory();
	}

	ProcessInfo::ProcessInfo(const bp::pid_t lunar_magic_pid) : shared_memory_name(determineSharedMemoryName(lunar_magic_pid)) {
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

		if (!shared_memory->hwnd_set) {
			return {};
		}

		return (HWND)shared_memory->hwnd;
	}

	void ProcessInfo::setLunarMagicMessageWindowHandle(HWND hwnd) {
		bi::scoped_lock<bi::interprocess_mutex>(shared_memory->mutex);

		shared_memory->hwnd = (uint32_t)hwnd;
		shared_memory->hwnd_set = true;
	}
#endif

	std::optional<uint16_t> ProcessInfo::getLunarMagicVerificationCode() {
		bi::scoped_lock<bi::interprocess_mutex>(shared_memory->mutex);

		if (!shared_memory->verification_code_set) {
			return {};
		}

		return shared_memory->verification_code;
	}

	void ProcessInfo::setLunarMagicVerificationCode(uint16_t verification_code) {
		bi::scoped_lock<bi::interprocess_mutex>(shared_memory->mutex);

		shared_memory->verification_code = verification_code;
		shared_memory->verification_code_set = true;
	}

	std::optional<fs::path> ProcessInfo::getCurrentLunarMagicRomPath() {
		bi::scoped_lock<bi::interprocess_mutex>(shared_memory->mutex);

		if (!shared_memory->current_rom_set) {
			return {};
		}

		return shared_memory->current_rom;
	}

	void ProcessInfo::setCurrentLunarMagicRomPath(const fs::path& rom_path) {
		bi::scoped_lock<bi::interprocess_mutex>(shared_memory->mutex);

		std::strcpy(shared_memory->current_rom, rom_path.string().data());
		shared_memory->current_rom_set = true;
	}

	const std::string& ProcessInfo::getSharedMemoryName() const {
		return shared_memory_name;
	}

	ProcessInfo::~ProcessInfo() {
		bi::shared_memory_object::remove(shared_memory_name.data());
	}
}
