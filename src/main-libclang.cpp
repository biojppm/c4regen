#include <c4/regen/regen.hpp>
#include <iostream>

int main(int argc, char* argv[])
{
    if(argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <cfg-yml> <compile_commands.json-dir> <file> [<more files...>]\n";
        return 1;
    }

    using namespace c4::ast;
    using namespace c4::regen;

    Regen rg(argv[1]);
    CompilationDb db(argv[2]);
    for(size_t i = 3; i < argc; ++i)
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
