#include "c4/regen/writer.hpp"

#include <cctype>
#include <c4/c4_push.hpp>

namespace c4 {
namespace regen {

void WriterBase::load(c4::yml::NodeRef const n)
{
    auto ntpl = n.find_child("tpl");
    m_tpl_chunk.load(ntpl, "chunk", s_default_tpl_chunk);
    m_file_tpl.m_hdr.load(ntpl, "hdr", s_default_tpl_hdr);
    m_file_tpl.m_inl.load(ntpl, "inl", s_default_tpl_inl);
    m_file_tpl.m_src.load(ntpl, "src", s_default_tpl_src);
}

void WriterBase::write(SourceFile c$$ src, set_type $ output_names)
{
    _begin_file(src);

    for(auto c$$ chunk : src.m_chunks)
    {
        _request_preambles(chunk);
    }

    // header code
    for(auto c$ gen : m_contributors.m_hdr)
    {
        _append_preamble(to_csubstr(gen->m_preambles.m_hdr.preamble), HDR);
    }
    for(auto c$$ chunk : src.m_chunks)
    {
        _append_code_chunk(chunk, chunk.m_hdr, HDR);
    }

    // inline code
    for(auto c$ gen : m_contributors.m_inl)
    {
        _append_preamble(to_csubstr(gen->m_preambles.m_inl.preamble), INL);
    }
    for(auto c$$ chunk : src.m_chunks)
    {
        _append_code_chunk(chunk, chunk.m_inl, INL);
    }

    // source code
    for(auto c$ gen : m_contributors.m_src)
    {
        _append_preamble(to_csubstr(gen->m_preambles.m_src.preamble), SRC);
    }
    for(auto c$$ chunk : src.m_chunks)
    {
        _append_code_chunk(chunk, chunk.m_src, SRC);
    }

    _render_files();

    _end_file(src);
}


void WriterBase::_request_preambles(CodeChunk c$$ chunk)
{
    if( ! chunk.m_hdr.empty())
    {
        m_contributors.m_hdr.insert(chunk.m_generator);
    }
    if( ! chunk.m_inl.empty())
    {
        m_contributors.m_inl.insert(chunk.m_generator);
    }
    if( ! chunk.m_src.empty())
    {
        m_contributors.m_src.insert(chunk.m_generator);
    }
}

void WriterBase::_append_preamble(csubstr s, Destination_e dst)
{
    _append_to(s, dst, &m_file_preambles);
}

void WriterBase::_append_to(csubstr s, Destination_e dst, CodeInstances<std::string> $ code)
{
    switch(dst)
    {
    case HDR: code->m_hdr.append(s.str, s.len); break;
    case INL: code->m_inl.append(s.str, s.len); break;
    case SRC: code->m_src.append(s.str, s.len); break;
    default: C4_ERROR("unknown destination");
    }
}

void WriterBase::_append_code_chunk(CodeChunk c$$ ch, c4::tpl::Rope c$$ r, Destination_e dst)
{
    // linearize the chunk's rope into a temporary buffer
    r.chain_all_resize(&m_tpl_ws_str);

    // render the chunk template using that buffer
    m_tpl_ws_tree.clear();
    m_tpl_ws_tree.clear_arena();
    c4::yml::NodeRef root = m_tpl_ws_tree.rootref();
    root |= c4::yml::MAP;
    c4::yml::NodeRef gen = root["generator"];
    gen |= c4::yml::MAP;
    C4_ASSERT(ch.m_generator != nullptr);
    gen["name"] = ch.m_generator->m_name;
    c4::yml::NodeRef ent = root["entity"];
    ent |= c4::yml::MAP;
    ent["name"] = ch.m_originator->m_name;
    ent["file"] = ch.m_originator->m_region.m_file;
    ent["line"] << ch.m_originator->m_region.m_start.line;
    root["gencode"] = to_csubstr(m_tpl_ws_str);
    m_tpl_chunk.render(root, &m_tpl_ws_rope);

    // append the templated chunk to the file contents
    for(csubstr entry : m_tpl_ws_rope.entries())
    {
        _append_to(entry, dst, &m_file_contents);
    }
}

void WriterBase::_render_files()
{
    m_tpl_ws_tree.clear();
    m_tpl_ws_tree.clear_arena();
    c4::yml::NodeRef root = m_tpl_ws_tree.rootref();
    root |= c4::yml::MAP;

    root["has_hdr"] << ( ! m_file_contents.m_hdr.empty());
    root["has_inl"] << ( ! m_file_contents.m_inl.empty());
    root["has_src"] << ( ! m_file_contents.m_src.empty());
    c4::yml::NodeRef hdr = root["hdr"];
    c4::yml::NodeRef inl = root["inl"];
    c4::yml::NodeRef src = root["src"];
    hdr |= c4::yml::MAP;
    inl |= c4::yml::MAP;
    src |= c4::yml::MAP;
    hdr["preamble"] = to_csubstr(m_file_preambles.m_hdr);
    inl["preamble"] = to_csubstr(m_file_preambles.m_inl);
    src["preamble"] = to_csubstr(m_file_preambles.m_src);
    hdr["gencode"] = to_csubstr(m_file_contents.m_hdr);
    inl["gencode"] = to_csubstr(m_file_contents.m_inl);
    src["gencode"] = to_csubstr(m_file_contents.m_src);
    hdr["include_guard"] = _incguard(to_csubstr(m_file_names.m_hdr));
    inl["include_guard"] = _incguard(to_csubstr(m_file_names.m_inl));
    //src["include_guard"] = _incguard(to_csubstr(m_file_names.m_src));
    hdr["filename"] = to_csubstr(m_file_names.m_hdr);
    inl["filename"] = to_csubstr(m_file_names.m_inl);
    src["filename"] = to_csubstr(m_file_names.m_src);

    // note that the properties are using the file contents
    // strings, so we can't chain the rope directly into those
    // strings. That's the reason for chaining into the temp
    // string and then copying it to the file contents strings.

    m_file_tpl.m_hdr.render(root, &m_tpl_ws_rope);
    m_tpl_ws_rope.chain_all_resize(&m_tpl_ws_str);
    m_file_contents.m_hdr = m_tpl_ws_str;

    m_file_tpl.m_inl.render(root, &m_tpl_ws_rope);
    m_tpl_ws_rope.chain_all_resize(&m_tpl_ws_str);
    m_file_contents.m_inl = m_tpl_ws_str;

    m_file_tpl.m_src.render(root, &m_tpl_ws_rope);
    m_tpl_ws_rope.chain_all_resize(&m_tpl_ws_str);
    m_file_contents.m_src = m_tpl_ws_str;
}

csubstr WriterBase::_incguard(csubstr filename)
{
    substr incg = m_tpl_ws_tree.alloc_arena(filename.len + 7);
    cat(incg, filename, "_GUARD_");
    for(auto $$ c : incg)
    {
        if(c == '.' || c == '/' || c == '\\') c = '_';
        else c = std::toupper(c);
    }
    return incg;
}


} // namespace regen
} // namespace c4

#include <c4/c4_pop.hpp>
