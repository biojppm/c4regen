#include "c4/ast/ast.hpp"

namespace c4 {
namespace ast {

thread_local std::vector< const char* > CompilationDb::s_cmd;
thread_local std::vector< std::string > CompilationDb::s_cmd_buf;

} // namespace ast
} // namespace c4
