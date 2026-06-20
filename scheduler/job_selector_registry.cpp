#include "job_selector_registry.hpp"
#include "selectors/fcfs_selector.hpp"
#include "selectors/sjf_selector.hpp"

void JobSelectorRegistry::init_all() {
    register_selector("fcfs", make_selector<FCFSSelector>);
    register_selector("sjf", make_selector<SJFSelector>);
}

void JobSelectorRegistry::register_selector(const std::string& name,
                                            std::unique_ptr<JobSelector> (*creator)()) {
    creators_[name] = creator;
}

std::unique_ptr<JobSelector> JobSelectorRegistry::create(const std::string& name) const {
    auto it = creators_.find(name);
    if (it != creators_.end())
        return it->second();
    return nullptr;
}
