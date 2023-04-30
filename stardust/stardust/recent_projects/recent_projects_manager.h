#pragma once

#include <filesystem>
#include <vector>
#include <chrono>
#include <fstream>

#include <nlohmann/json.hpp>
#include <fmt/format.h>

#include "../path_util.h"

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace stardust {
	struct Project {
		std::string name{};
		fs::path stardust_executable{};
		fs::path project_root{};
		std::chrono::system_clock::time_point last_accessed{};

		bool operator==(const Project& other) {
			return name == other.name && stardust_executable == other.stardust_executable &&
				project_root == other.project_root;
		}

		Project& operator=(const Project& other) {
			name = other.name;
			stardust_executable = other.stardust_executable;
			project_root = other.project_root;
			last_accessed = other.last_accessed;

			return *this;
		}

		Project(const fs::path& stardust_executable, const fs::path& project_root)
			: name(project_root.stem().string()), stardust_executable(stardust_executable), project_root(project_root),
			last_accessed(std::chrono::system_clock::now()) {}

		Project() {};

		std::string toString() const {
			return fmt::format("{} ({}) [{}]", name, project_root.string(), last_accessed);
		}

		json toJson() const {
			json j;

			j["name"] = name;
			j["stardust_executable"] = stardust_executable;
			j["project_root"] = project_root;
			j["last_accessed"] = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(last_accessed.time_since_epoch()).count());

			return j;
		}

		Project(const json& j) : name(j["name"].get<std::string>()), stardust_executable(j["stardust_executable"].get<std::string>()),
			project_root(j["project_root"].get<std::string>()), 
			last_accessed(std::chrono::milliseconds(j["last_accessed"].get<uint64_t>())) {}
	};

	class RecentProjectsManager {
	protected:
		static constexpr auto MAX_ENTRIES{ 9 };

		std::vector<Project> projects;

	public:
		const std::vector<Project>& get() const {
			return projects;
		}

		const Project& get(size_t project_index) const {
			return projects.at(project_index);
		}

		void serializeProjects() const {
			std::ofstream recent_projects_file{ PathUtil::getRecentProjectsPath() };
			json j{};

			j["recent_projects"] = std::vector<json>();

			for (const auto& project : projects) {
				j["recent_projects"].push_back(project.toJson());
			}

			recent_projects_file << std::setw(4) << j << std::endl;

			recent_projects_file.close();
		}

		void update(const fs::path& project_root, const fs::path& stardust_executable) {
			reloadList();

			Project project{ stardust_executable, project_root };

			const auto found{ std::find(projects.begin(), projects.end(), project) };
			if (found != projects.end()) {
				found->last_accessed = std::chrono::system_clock::now();
				std::rotate(projects.begin(), found, found + 1);
			}
			else {
				projects.insert(projects.begin(), project);
				if (projects.size() == MAX_ENTRIES + 1) {
					projects.pop_back();
				}
			}

			serializeProjects();
		}

		void reloadList() {
			const auto recent_projects_file{ PathUtil::getRecentProjectsPath() };
			projects = {};

			if (fs::exists(recent_projects_file)) {
				std::ifstream recent_projects_file{ PathUtil::getRecentProjectsPath() };
				try {
					const auto j{ json::parse(recent_projects_file) };
					for (const auto& entry : j["recent_projects"].array()) {
						projects.push_back(Project(entry));
					}
				}
				catch (const std::exception& e) {
					recent_projects_file.close();
					throw std::runtime_error(fmt::format("Failed to read recent projects file, resetting recent projects list:{}",
						e.what()));
				}

				recent_projects_file.close();

				if (projects.size() > MAX_ENTRIES) {
					projects.resize(MAX_ENTRIES);
				}
			}

			removeInvalidProjects();
		}

		void removeInvalidProjects() {
			std::erase_if(projects, [](const Project& project) {
				return !fs::exists(project.project_root) || !fs::exists(project.stardust_executable);
			});

			serializeProjects();
		}

		RecentProjectsManager() {
			fs::create_directories(PathUtil::getUserWideCachePath());

			reloadList();
		};
	};
}
