#ifndef _C4_REGEN_HPP_
#define _C4_REGEN_HPP_

#include "c4/regen/enum.hpp"
#include "c4/regen/function.hpp"
#include "c4/regen/class.hpp"
#include "c4/regen/writer.hpp"

#include <c4/c4_push.hpp>

namespace c4 {
namespace regen {


//-----------------------------------------------------------------------------

bool is_hdr(csubstr filename);
bool is_src(csubstr filename);

void add_hdr(const char* ext);
void add_src(const char* ext);


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

struct Regen
{
    std::string   m_config_file_name;
    std::string   m_config_file_yml;
    c4::yml::Tree m_config_data;

    std::vector<EnumGenerator    > m_gens_enum;
    std::vector<ClassGenerator   > m_gens_class;
    std::vector<FunctionGenerator> m_gens_function;
    std::vector<Generator*       > m_gens_all;

    Writer m_writer;

public:

    Regen(const char* config_file)
    {
        _load_config_file(config_file);
    }

    bool empty() const { return m_gens_all.empty(); }

public:

    template <class SourceFileNameCollection>
    void gencode(SourceFileNameCollection c$$ collection, const char* db_dir=nullptr)
    {
        ast::CompilationDb db(db_dir);
        yml::Tree workspace;
        yml::NodeRef wsroot = workspace.rootref();
        SourceFile sf;

        m_writer.begin_files();
        for(const char* source_file : collection)
        {
            ast::Index idx;
            ast::TranslationUnit unit(idx, source_file, db);
            sf.clear();
            sf.init_source_file(idx, unit);

            sf.extract(m_gens_all.data(), m_gens_all.size());
            sf.gencode(m_gens_all.data(), m_gens_all.size(), workspace);
            m_writer.write(sf);
        }
        m_writer.end_files();
    }


    template <class SourceFileNameCollection>
    void print_output_filenames(SourceFileNameCollection c$$ collection)
    {
        Writer::set_type workspace;
        for(const char* source_file : collection)
        {
            m_writer.extract_filenames(to_csubstr(source_file), &workspace);
        }
        for(auto c$$ name : workspace)
        {
            printf("%s\n", name.c_str());
        }
    }

private:

    void _load_config_file(const char* file_name);

    template< class GeneratorT >
    void _loadgen(c4::yml::NodeRef const& n, std::vector<GeneratorT> *gens)
    {
        gens->emplace_back();
        GeneratorT &g = gens->back();
        g.load(n);
        m_gens_all.push_back(&g);
    }
};

} // namespace regen
} // namespace c4

#include <c4/c4_pop.hpp>

#endif /* _C4_REGEN_HPP_ */
