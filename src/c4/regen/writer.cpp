#include "c4/regen/writer.hpp"

#include <cctype>
#include <c4/c4_push.hpp>

namespace c4 {
namespace regen {
   
struct FileType
{
    std::vector<std::string> m_extensions;
    FileType(std::initializer_list<const char*> exts);
    void add(const char* extension);
    bool matches(csubstr filename) const;
};

FileType s_hdr_exts = {".h", ".hpp", ".hxx", ".h++", ".hh", ".inl"};
FileType s_src_exts = {".c", ".cpp", ".cxx", ".c++", ".cc"};

FileType::FileType(std::initializer_list<const char*> exts)
{
    m_extensions.reserve(exts.size());
    for(auto e : exts)
    {
        add(e);
    }
}

void FileType::add(const char* extension)
{
    m_extensions.push_back(extension);
}

bool FileType::matches(csubstr filename) const
{
    for(auto const& sext : m_extensions)
    {
        auto ext = to_csubstr(sext);
        if(filename.ends_with(ext))
        {
            return true;
        }
    }
    return false;
}


//-----------------------------------------------------------------------------

bool is_hdr(csubstr filename) { return s_hdr_exts.matches(filename); }
bool is_src(csubstr filename) { return s_src_exts.matches(filename); }

void add_hdr(const char* ext) { s_hdr_exts.add(ext); }
void add_src(const char* ext) { s_src_exts.add(ext); }
 

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void WriterBase::load(c4::yml::NodeRef const n)
{
    auto ntpl = n.find_child("tpl");
    m_tpl_chunk     .load(ntpl, "chunk", s_default_tpl_chunk);
    m_file_tpl.m_hdr.load(ntpl, "hdr"  , s_default_tpl_hdr);
    m_file_tpl.m_inl.load(ntpl, "inl"  , s_default_tpl_inl);
    m_file_tpl.m_src.load(ntpl, "src"  , s_default_tpl_src);
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
    if(r.str_size() == 0) return;

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
    ent["file"] = to_csubstr(ch.m_originator->m_region.m_file);
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
    const bool hdr_empty = m_file_contents.m_hdr.empty() && m_file_preambles.m_hdr.empty();
    const bool inl_empty = m_file_contents.m_inl.empty() && m_file_preambles.m_inl.empty();
    const bool src_empty = m_file_contents.m_src.empty() && m_file_preambles.m_src.empty();

    if(hdr_empty && inl_empty && src_empty) return;

    m_tpl_ws_tree.clear();
    m_tpl_ws_tree.clear_arena();
    c4::yml::NodeRef root = m_tpl_ws_tree.rootref();
    root |= c4::yml::MAP;

    root["has_hdr"] << ( ! hdr_empty);
    root["has_inl"] << ( ! inl_empty);
    root["has_src"] << ( ! src_empty);
    c4::yml::NodeRef hdr = root["hdr"];
    c4::yml::NodeRef inl = root["inl"];
    c4::yml::NodeRef src = root["src"];
    hdr |= c4::yml::MAP;
    inl |= c4::yml::MAP;
    src |= c4::yml::MAP;
    hdr["preamble"] = to_csubstr(m_file_preambles.m_hdr);
    inl["preamble"] = to_csubstr(m_file_preambles.m_inl);
    src["preamble"] = to_csubstr(m_file_preambles.m_src);
    hdr["gencode"]  = to_csubstr(m_file_contents.m_hdr);
    inl["gencode"]  = to_csubstr(m_file_contents.m_inl);
    src["gencode"]  = to_csubstr(m_file_contents.m_src);
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
    C4_ASSERT(filename.not_empty());
    substr incg = m_tpl_ws_tree.alloc_arena(filename.len + 7);
    cat(incg, filename, "_GUARD_");
    for(auto $$ c : incg)
    {
        if(c == '.' || c == '/' || c == '\\' || c == '-') c = '_';
        else c = std::toupper(c);
    }
    return incg;
}

void WriterBase::extract_filenames(csubstr name, CodeInstances<std::string> $ fn)
{
    C4_CHECK(name.not_empty());

    if(m_source_root.empty())
    {
        name = name.basename();
    }
    else
    {
        name = name.sub(m_source_root.size());
    }

    auto $$ tmp = m_file_names.m_src;
    tmp.assign(name.begin(), name.end());
    substr wname = to_substr(tmp);
    for(size_t i = 0; i < wname.len; ++i)
    {
        char $$ c = wname.str[i];
        if(c4::fs::is_sep(i, wname.str, wname.len)) c = '/';
        else c = std::tolower(c);
    }

    C4_ASSERT(is_hdr(wname) || is_src(wname));
    csubstr ext = wname.pop_right('.');
    csubstr name_wo_ext = wname.gpop_left('.');

    catrs(&fn->m_hdr, name_wo_ext, ".c4gen.hpp");
    catrs(&fn->m_inl, name_wo_ext, ".c4gen.def.hpp");
    catrs(&fn->m_src, name_wo_ext, ".c4gen.cpp");
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void Writer::load(c4::yml::NodeRef const n)
{
    csubstr s;
    n.get_if("writer", &s, csubstr("stdout"));
    m_type = str2type(s);
    switch(m_type)
    {
    case STDOUT:
        m_impl.reset(new WriterStdout());
        break;
    case GENFILE:
        m_impl.reset(new WriterGenFile());
        break;
    case GENGROUP:
        m_impl.reset(new WriterGenGroup());
        break;
    case SAMEFILE:
        m_impl.reset(new WriterSameFile());
        break;
    case SINGLEFILE:
        m_impl.reset(new WriterSingleFile());
        break;
    default:
        C4_ERROR("unknown writer type");
    }
    m_impl->load(n);
}

Writer::Type_e Writer::str2type(csubstr type_name)
{
    if(type_name == "stdout")
    {
        return STDOUT;
    }
    else if(type_name == "genfile")
    {
        return GENFILE;
    }
    else if(type_name == "gengroup")
    {
        return GENGROUP;
    }
    else if(type_name == "samefile")
    {
        return SAMEFILE;
    }
    else if(type_name == "singlefile")
    {
        return SINGLEFILE;
    }
    else
    {
        C4_ERROR("unknown writer type");
    }
    return STDOUT;
}

} // namespace regen
} // namespace c4

#include <c4/c4_pop.hpp>
