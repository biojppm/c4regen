#ifndef _c4_REGEN_SOURCE_FILE_HPP_
#define _c4_REGEN_SOURCE_FILE_HPP_

#include "c4/regen/enum.hpp"
#include "c4/regen/class.hpp"
#include "c4/regen/function.hpp"
#include "c4/regen/extractor.hpp"

#include <c4/c4_push.hpp>

namespace c4 {
namespace regen {


struct SourceFile : public Entity
{
public:

    // originator entities extracted from this source file. Given a
    // generator, these entities will originate the code chunks.

    std::vector<Enum>     m_enums;      ///< enum originator entities
    std::vector<Class>    m_classes;    ///< class originator entities
    std::vector<Function> m_functions;  ///< function originator entities

    /// this is used to make sure that the generated code chunks are in the
    /// same order as given in the source file
    struct EntityPos
    {
        Generator c$ generator;
        EntityType_e entity_type;
        size_t pos;
    };
    std::vector<EntityPos> m_pos;    ///< the map to the chunks array
    std::vector<CodeChunk> m_chunks; ///< the code chunks originated from the source code

public:

    void init_source_file(ast::Index $$ idx, ast::TranslationUnit c$$ tu)
    {
        ast::Entity e{tu.root(), {}, &tu, &idx};
        this->Entity::init(e);
    }

    void clear()
    {
        m_enums.clear();
        m_classes.clear();
        m_functions.clear();
        m_pos.clear();
        m_chunks.clear();
    }

    size_t extract(Generator c$ c$ gens, size_t num_gens);
    void gencode(Generator c$ c$ gens, size_t num_gens, c4::yml::NodeRef workspace);

private:


    template<class EntityT>
    void _extract(std::vector<EntityT> $ entities, EntityType_e type, Generator c$$ g)
    {
        struct _visitor_data
        {
            SourceFile $ sf;
            std::vector<EntityT> $ entities;
            EntityType_e type;
            Generator c$ gen;
        };
        _visitor_data vd{this, entities, type, &g};

        auto visitor = [](ast::Cursor c, ast::Cursor parent, void *data)
        {
            auto vd = (_visitor_data $)data;
            Extractor::Data ret = vd->gen->m_extractor.extract(*vd->sf->m_index, c);
            if(ret.extracted)
            {
                EntityPos pos{vd->gen, vd->type, vd->entities->size()};
                vd->sf->m_pos.emplace_back(pos);
                vd->sf->m_chunks.emplace_back();
                vd->entities->emplace_back();
                EntityT $$ e = vd->entities->back();
                ast::Entity ae;
                ae.tu = vd->sf->m_tu;
                ae.idx = vd->sf->m_index;
                ae.cursor = ret.cursor;
                ae.parent = parent;
                e.init(ae);
                if(ret.has_tag)
                {
                    e.set_tag(ret.tag, parent);
                }
            }
            return CXChildVisit_Recurse;
        };

        m_tu->visit_children(visitor, &vd);
    }

    template<class EntityT>
    void _gencode(std::vector<EntityT> $ entities, EntityType_e type, Generator c$$ g, c4::yml::NodeRef workspace)
    {
        C4_ASSERT(workspace.is_root());
        for(size_t i = 0, e = m_pos.size(); i < e; ++i)
        {
            auto c$$ p = m_pos[i];
            if(p.generator == &g && p.entity_type == type)
            {
                g.generate((*entities)[p.pos], workspace, &m_chunks[i]);
            }
        }
    }

public:

    struct const_iterator
    {
        const_iterator(SourceFile c$ s, size_t pos) : s(s), pos(pos) {}
        SourceFile c$ s;
        size_t pos;

        using value_type = Entity const;

        value_type $$ operator*  () const { C4_ASSERT(pos >= 0 && pos < s->m_pos.size()); return *s->resolve(s->m_pos[pos]); }
        value_type  $ operator-> () const { C4_ASSERT(pos >= 0 && pos < s->m_pos.size()); return  s->resolve(s->m_pos[pos]); }
    };

    const_iterator begin() const { return const_iterator(this, 0); }
    const_iterator end  () const { return const_iterator(this, m_pos.size()); }

public:

    Entity c$ resolve(EntityPos c$$ p) const
    {
        switch(p.entity_type)
        {
        case ENT_ENUM:     return &m_enums    [p.pos];
        case ENT_CLASS:    return &m_classes  [p.pos];
        case ENT_FUNCTION: return &m_functions[p.pos];
        default:
            C4_ERROR("unknown entity type");
            break;
        }
        return nullptr;
    }
};


} // namespace regen
} // namespace c4

#include <c4/c4_pop.hpp>

#endif /* _c4_REGEN_SOURCE_FILE_HPP_ */
