#ifndef _c4_GENERATOR_HPP_
#define _c4_GENERATOR_HPP_

#include <c4/tpl/engine.hpp>
#include "c4/regen/entity.hpp"
#include "c4/regen/extractor.hpp"

#include <c4/c4_push.hpp>

namespace c4 {
namespace regen {


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


template<class T>
struct CodeInstances
{
    T m_hdr;
    T m_inl;
    T m_src;
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct CodeTemplate
{
    std::shared_ptr<c4::tpl::Engine> engine;
    c4::tpl::Rope parsed_rope;

    bool empty() const { return engine.get() == nullptr; }

    bool load(c4::yml::NodeRef const n, csubstr name, csubstr fallback_tpl={})
    {
        engine.reset();
        csubstr src = fallback_tpl;
        if(n.valid())
        {
            n.get_if(name, &src);
        }
        if(src.not_empty())
        {
            engine = std::make_shared<c4::tpl::Engine>();
            engine->parse(src, &parsed_rope);
        }
        return ! empty();
    }

    void render(c4::yml::NodeRef properties, c4::tpl::Rope *r) const
    {
        engine->render(properties, r);
    }
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct Generator;


struct CodeChunk : public CodeInstances<c4::tpl::Rope>
{
    Generator c$ m_generator;
    Entity    c$ m_originator;
};


//-----------------------------------------------------------------------------

struct CodePreamble
{
    std::string preamble;

    bool empty() const { return preamble.empty(); }

    bool load(c4::yml::NodeRef const& n, csubstr name)
    {
        csubstr s;
        n.get_if(name, &s);
        if(s.not_empty())
        {
            preamble.assign(s.begin(), s.end());
        }
        return ! empty();
    }
};

struct Generator : public CodeInstances<CodeTemplate>
{
    using Preambles = CodeInstances<CodePreamble>;

    Extractor    m_extractor;
    Preambles    m_preambles;
    EntityType_e m_entity_type;
    csubstr      m_name;
    bool         m_empty;

    Generator() :
        CodeInstances<CodeTemplate>(),
        m_extractor(),
        m_preambles(),
        m_entity_type(),
        m_name(),
        m_empty(true)
    {
    }
    virtual ~Generator() = default;

    Generator(Generator const&) = default;
    Generator& operator= (Generator const&) = default;

    Generator(Generator &&) = default;
    Generator& operator= (Generator &&) = default;

    void load(c4::yml::NodeRef n)
    {
        m_name = n["name"].val();
        m_extractor.load(n["extract"]);
        load_templates(n);
    }

    void generate(Entity c$$ o, c4::yml::NodeRef root, CodeChunk *ch) const
    {
        if(m_empty) return;
        ch->m_generator = this;
        ch->m_originator = &o;
        create_prop_tree(o, root);
        render(root, ch);
    }

    virtual void create_prop_tree(Entity c$$ o, c4::yml::NodeRef root) const = 0;

    void render(c4::yml::NodeRef const properties, CodeChunk *ch) const
    {
        m_hdr.render(properties, &ch->m_hdr);
        m_inl.render(properties, &ch->m_inl);
        m_src.render(properties, &ch->m_src);
    }

    void load_templates(c4::yml::NodeRef const n)
    {
        m_empty  = false;
        m_empty |= m_preambles.m_hdr.load(n, "hdr_preamble");
        m_empty |= m_preambles.m_inl.load(n, "inl_preamble");
        m_empty |= m_preambles.m_src.load(n, "src_preamble");
        m_empty |= m_hdr         .load(n, "hdr");
        m_empty |= m_inl         .load(n, "inl");
        m_empty |= m_src         .load(n, "src");
    }

};


} // namespace regen
} // namespace c4

#include <c4/c4_pop.hpp>

#endif /* _c4_GENERATOR_HPP_ */
