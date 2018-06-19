#include "c4/regen/source_file.hpp"

#include <algorithm>
#include <c4/c4_push.hpp>

namespace c4 {
namespace regen {

size_t SourceFile::extract(Generator c$ c$ gens, size_t num_gens)
{
    size_t num_chunks = m_pos.size();

    // extract all entities
    for(size_t i = 0; i < num_gens; ++i)
    {
        auto c$ g_ = gens[i];
        switch(g_->m_entity_type)
        {
        case ENT_CLASS:    _extract(&m_classes  , ENT_CLASS   , *g_); break;
        case ENT_ENUM:     _extract(&m_enums    , ENT_ENUM    , *g_); break;
        case ENT_FUNCTION: _extract(&m_functions, ENT_FUNCTION, *g_); break;
        default:
            C4_NOT_IMPLEMENTED();
        }
    }

    // reorder the chunks so that they are in the same order as the originating entities
    std::sort(m_pos.begin(), m_pos.end(), [this](EntityPos c$$ l_, EntityPos c$$ r_){
        return this->resolve(l_)->m_region < this->resolve(r_)->m_region;
    });

    num_chunks = m_pos.size() - num_chunks;

    return num_chunks;
}

void SourceFile::gencode(Generator c$ c$ gens, size_t num_gens, c4::yml::NodeRef workspace)
{
    for(size_t i = 0; i < num_gens; ++i)
    {
        auto c$ g_ = gens[i];
        switch(g_->m_entity_type)
        {
        case ENT_CLASS:    _gencode(&m_classes  , ENT_CLASS   , *g_, workspace); break;
        case ENT_ENUM:     _gencode(&m_enums    , ENT_ENUM    , *g_, workspace); break;
        case ENT_FUNCTION: _gencode(&m_functions, ENT_FUNCTION, *g_, workspace); break;
        default:
            C4_NOT_IMPLEMENTED();
        }
    }
}


} // namespace regen
} // namespace c4
