#pragma once

#include "../vocabulary/vocabulary_engine.hpp"
#include <string>
#include <vector>
#include <memory>

namespace vani::voice::entities {

enum class EntityType : uint8_t {
    Application,
    Project,
    Technology,
    Contact,
    Device,
    FilePath,
    Custom
};

struct ResolvedEntity {
    std::string raw_span;
    std::string canonical_name;
    EntityType type{EntityType::Custom};
    float confidence{1.0f};
    bool is_ambiguous{false};
    std::vector<std::string> candidate_alternatives;
};

class EntityResolver {
public:
    explicit EntityResolver(vocabulary::VocabularyEnginePtr vocab_engine);
    ~EntityResolver() = default;

    [[nodiscard]] std::vector<ResolvedEntity> resolve_entities(
        const std::string& normalized_text,
        const std::string& active_scope = ""
    );

private:
    vocabulary::VocabularyEnginePtr vocab_engine_;
};

using EntityResolverPtr = std::shared_ptr<EntityResolver>;

} // namespace vani::voice::entities
