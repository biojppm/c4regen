#include "c4/regen/regen.hpp"

#include <c4/yml/parse.hpp>

namespace c4 {
namespace regen {

void Regen::_load_config_file(const char* file_name)
{
    // read the yml config and parse it
    m_config_file_name = file_name;
    fs::file_get_contents(m_config_file_name.c_str(), &m_config_file_yml);
    c4::yml::parse(to_csubstr(m_config_file_name), to_substr(m_config_file_yml), &m_config_data);
    c4::yml::NodeRef r = m_config_data.rootref();
    c4::yml::NodeRef n;

    m_writer.load(r);

    m_gens_all.clear();
    m_gens_enum.clear();
    m_gens_class.clear();
    m_gens_function.clear();

    n = r.find_child("generators");
    if( ! n.valid()) return;
    for(auto const& ch : n.children())
    {
        csubstr gtype = ch["type"].val();
        if(gtype == "enum")
        {
            _loadgen(ch, &m_gens_enum);
        }
        else if(gtype == "class")
        {
            _loadgen(ch, &m_gens_class);
        }
        else if(gtype == "function")
        {
            _loadgen(ch, &m_gens_function);
        }
        else
        {
            C4_ERROR("unknown generator type");
        }
    }
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

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

} // namespace regen
} // namespace c4
