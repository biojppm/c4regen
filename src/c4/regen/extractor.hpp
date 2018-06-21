#ifndef _c4_REGEN_EXTRACTOR_HPP_
#define _c4_REGEN_EXTRACTOR_HPP_

#include <string>

#include <c4/yml/node.hpp>
#include "c4/ast/ast.hpp"

#include <c4/c4_push.hpp>

namespace c4 {
namespace regen {

struct SourceFile;

typedef enum {
    EXTR_ALL, ///< extract all possible entities
    EXTR_TAGGED_MACRO, ///< extract only entities tagged with a macro
    EXTR_TAGGED_MACRO_ANNOTATED, ///< extract only entities with an annotation within a tag macro
} ExtractorType_e;

/** Examines the AST and extracts useful entities.
 *
 * YAML config examples:
 * for EXTR_ALL:
 *
 * @begincode
 * extract: all
 * @endcode
 *
 * for EXTR_TAGGED_MACRO:
 *
 * @begincode
 * extract:
 *   macro: C4_ENUM
 * @endcode
 *
 * for EXTR_TAGGED_MACRO_ANNOTATED:
 *
 * @begincode
 * extract:
 *   macro: C4_PROPERTY
 *   entry: gui
 * @endcode
 */
struct Extractor
{
    ExtractorType_e m_type{EXTR_ALL};
    std::string m_macro;
    std::string m_entry;
    std::vector<CXCursorKind> m_cursor_kinds;

    void load(c4::yml::NodeRef n);

    void set_kinds(std::initializer_list<CXCursorKind> il);
    bool kind_matches(CXCursorKind k) const;

    bool has_true_annotation(csubstr annot, csubstr entry) const;

    struct Data
    {
        c4::ast::Cursor cursor, tag;
        bool extracted, has_tag;
    };
    Extractor::Data extract(SourceFile c$$ sf, c4::ast::Cursor c) const;
    
};


} // namespace regen
} // namespace c4

#include <c4/c4_pop.hpp>

#endif /* _c4_REGEN_EXTRACTOR_HPP_ */
