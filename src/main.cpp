#include <c4/regen/regen.hpp>
#include <cppast/libclang_parser.hpp>
#include <cppast/visitor.hpp>

#include <iostream>

void print_indent(std::string prefix, std::string name)
{
    std::cout << prefix << ' ';
    for(size_t i = 0; name.size(); ++i)
    {
        std::cout << ' ';
    }
    std::cout << "  - ";
}

void print_ast(const cppast::cpp_file& file)
{
    using namespace cppast;

    std::string prefix;
    // visit each entity in the file
    visit(file, [&](const cpp_entity& e, visitor_info info) {
        if(info.event == visitor_info::container_entity_exit)
        {
            // exiting an old container
            prefix.pop_back();
        }
        else if (info.event == visitor_info::container_entity_enter)
        // entering a new container
        {
            std::cout << prefix << "'" << e.name() << "' - " << to_string(e.kind()) << '\n';
            prefix += "\t";
        }
        else // if (info.event == visitor_info::leaf_entity) // a non-container entity
        {
            std::cout << prefix << "'" << e.name() << "' - " << to_string(e.kind()) << '\n';
            auto const& cmt = e.comment();
            if(cmt)
            {
                print_indent(prefix, e.name());
                std::cout << "comment: " << cmt.value();
            }
            auto const& attrs = e.attributes();
            if( ! attrs.empty())
            {
                for(auto const& a : attrs)
                {
                    print_indent(prefix, e.name());
                    std::cout << "attr: " << a.name() << "\n";
                }
            }
        }
    });
}

/// another comment
struct MyStruct
{
    int member;
    std::string mem2;
};

int main(int argc, const char *argv[])
{
    if(argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <compile_commands.json-dir> <file> [<more files...>]\n";
        return 1;
    }

    cppast::libclang_compilation_database db(argv[1]);

    for(size_t i = 2; i < argc; ++i)
    {
        std::cout << "processing file: " << argv[i] << "\n";
        cppast::cpp_entity_index idx;
        cppast::libclang_compile_config cfg(db, argv[i]);
        cppast::simple_file_parser<cppast::libclang_parser> parser(type_safe::ref(idx));
        auto cpp_file = parser.parse(argv[i], cfg);
        print_ast(cpp_file.value());
    }

    return 0;
}
