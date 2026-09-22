#pragma once

#include "utterance.hpp"
#include "../normalization/normalized_text.hpp"
#include "../entities/entity_resolver.hpp"
#include <memory>

namespace vani::voice::intent {

class IntentPreparer {
public:
    explicit IntentPreparer(entities::EntityResolverPtr resolver = nullptr);
    ~IntentPreparer() = default;

    [[nodiscard]] Utterance prepare(
        const std::string& session_id,
        const normalization::NormalizedText& normalized,
        const std::vector<entities::ResolvedEntity>& entities = {}
    ) const;
};

using IntentPreparerPtr = std::shared_ptr<IntentPreparer>;

} // namespace vani::voice::intent
