#include "process_info.h"

namespace callisto {
	std::string ProcessInfo::determineSharedMemoryName(const bp::pid_t lunar_magic_pid) {
		return fmt::format(SHARED_MEMORY_NAME, lunar_magic_pid);
	}

	void ProcessInfo::setUpSharedMemory(bool open_only) {
		if (open_only) {
			shared_memory_object = bi::shared_memory_object(
				bi::open_only,
				shared_memory_name.data(),
				bi::read_write
			);
		}
		else {
			shared_memory_object = bi::shared_memory_object(
				bi::open_or_create,
				shared_memory_name.data(),
				bi::read_write
			);
		}

		if (!open_only) {
			shared_memory_object.truncate(sizeof(SharedMemory));
		}

		mapped_region = bi::mapped_region(shared_memory_object, bi::read_write);

		void* addr = mapped_region.get_address();

		if (!open_only) {
			shared_memory = new (addr) SharedMemory;
		}
		else {
			shared_memory = static_cast<SharedMemory*>(addr);
		}
	}

	ProcessInfo::ProcessInfo() {};

	void ProcessInfo::setPid(const bp::pid_t lunar_magic_pid, bool open_only) {
		shared_memory_name = determineSharedMemoryName(lunar_magic_pid);
		setUpSharedMemory(open_only);
	}

	ProcessInfo::ProcessInfo(const bp::pid_t lunar_magic_pid, bool open_only) : shared_memory_name(determineSharedMemoryName(lunar_magic_pid)) {
		setUpSharedMemory(open_only);
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
		bi::scoped_lock<bi::interprocess_mutex> lock(shared_memory->mutex);

		if (!shared_memory->hwnd_set) {
			return {};
		}

		return (HWND)shared_memory->hwnd;
	}

	void ProcessInfo::setLunarMagicMessageWindowHandle(HWND hwnd) {
		bi::scoped_lock<bi::interprocess_mutex> lock(shared_memory->mutex);

		shared_memory->hwnd = (uint32_t)hwnd;
		shared_memory->hwnd_set = true;
	}
#endif

	std::optional<uint16_t> ProcessInfo::getLunarMagicVerificationCode() {
		bi::scoped_lock<bi::interprocess_mutex> lock(shared_memory->mutex);

		if (!shared_memory->verification_code_set) {
			return {};
		}

		return shared_memory->verification_code;
	}

	void ProcessInfo::setLunarMagicVerificationCode(uint16_t verification_code) {
		bi::scoped_lock<bi::interprocess_mutex> lock(shared_memory->mutex);

		shared_memory->verification_code = verification_code;
		shared_memory->verification_code_set = true;
	}

	std::optional<fs::path> ProcessInfo::getCurrentLunarMagicRomPath() {
		bi::scoped_lock<bi::interprocess_mutex> lock(shared_memory->mutex);

		if (!shared_memory->current_rom_set) {
			return {};
		}

		return shared_memory->current_rom;
	}

	void ProcessInfo::setCurrentLunarMagicRomPath(const fs::path& rom_path) {
		bi::scoped_lock<bi::interprocess_mutex> lock(shared_memory->mutex);

		std::strcpy(shared_memory->current_rom, rom_path.string().data());
		shared_memory->current_rom_set = true;
	}

	std::optional<bp::group::native_handle_t> ProcessInfo::getSaveProcessPid() {
		bi::scoped_lock<bi::interprocess_mutex> lock(shared_memory->mutex);

		return shared_memory->save_process_pid_set ? std::make_optional(shared_memory->save_process_pid) : std::nullopt;
	}

	void ProcessInfo::setSaveProcessPid(bp::group::native_handle_t pid) {
		bi::scoped_lock<bi::interprocess_mutex> lock(shared_memory->mutex);

		shared_memory->save_process_pid_set = true;
		shared_memory->save_process_pid = pid;
	}

	void ProcessInfo::unsetSaveProcessPid() {
		bi::scoped_lock<bi::interprocess_mutex> lock(shared_memory->mutex);

		shared_memory->save_process_pid_set = false;
	}

	const std::string& ProcessInfo::getSharedMemoryName() const {
		return shared_memory_name;
	}

	bool ProcessInfo::sharedMemoryExistsFor(bp::pid_t pid) {
		try {
			bi::shared_memory_object shared(
				bi::open_only,
				determineSharedMemoryName(pid).data(),
				bi::read_only
			);
		}
		catch (const std::exception&) {
			// supposedly creating a shared memory object with open_only that doesn't exist will throw an 
			// exception, hoping that's true and also that this will not throw exceptions in other 
			// circumstances...
			return false;
		}
		return true;
	}

	ProcessInfo::~ProcessInfo() {
		bi::shared_memory_object::remove(shared_memory_name.data());
	}
}
