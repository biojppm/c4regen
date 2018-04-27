#include <c4/regen/regen.hpp>
#include <iostream>

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <compile_commands.json-dir> <file> [<more files...>]\n";
        return 1;
    }

    using namespace c4::ast;

    CompilationDb db(argv[1]);
    for(size_t i = 2; i < argc; ++i)
    {
        Index idx(clang_createIndex(0, 0));
        TranslationUnit unit(idx, db, argv[i]);
        unit.visit_children([](Cursor c, Cursor parent, void*){
            std::cout << "Cursor '" << c.spelling() << "' of kind '" << c.kind_spelling() << "'\n";
            return CXChildVisit_Recurse;
        });
    }

    return 0;
}
