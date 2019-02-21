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

    std::vector<SourceFile> m_src_files;
    bool                    m_save_src_files;

public:

    Regen() : m_save_src_files(false) {}

    Regen(const char* config_file) : Regen()
    {
        load_config(config_file);
    }

    bool empty() const { return m_gens_all.empty(); }

    void load_config(const char* file_name);

    void save_src_files(bool yes) { m_save_src_files = yes; }

public:

    template <class SourceFileNameCollection>
    void gencode(SourceFileNameCollection c$$ collection, const char* db_dir=nullptr, const char* const* flags=nullptr, size_t num_flags=0)
    {
        ast::CompilationDb db(db_dir);
        ast::Index idx;
        ast::TranslationUnit unit;
        yml::Tree workspace;

        SourceFile buf;
        if(m_save_src_files)
        {
            size_t fsz = 0;
            for(const char* filename : collection)
            {
                fsz++;
            }
            m_src_files.resize(fsz);
        }
        
        m_writer.begin_files();
        size_t ifile = 0;
        for(const char* filename : collection)
        {
            if( ! m_save_src_files)
            {
                buf.clear();
                idx.clear();
            }

            SourceFile &sf = m_save_src_files ? m_src_files[ifile++] : buf;

            if(db_dir)
            {
                unit.reset(idx, filename, db);
            }
            else
            {
                unit.reset(idx, filename, flags, num_flags);
            }
            sf.init_source_file(idx, unit);
            sf.extract(m_gens_all.data(), m_gens_all.size());
            sf.gencode(m_gens_all.data(), m_gens_all.size(), workspace);
            
            m_writer.write(sf);
        }
        m_writer.end_files();
    }

    template<class SourceFileNameCollection>
    void print_output_filenames(SourceFileNameCollection c$$ collection)
    {
        Writer::set_type workspace;
        for(const char* source_file : collection)
        {
            m_writer.insert_filenames(to_csubstr(source_file), &workspace);
        }
        for(auto c$$ name : workspace)
        {
            printf("%s\n", name.c_str());
        }
    }

private:

    template<class GeneratorT>
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
