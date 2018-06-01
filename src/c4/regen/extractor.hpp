#ifndef _c4_REGEN_EXTRACTOR_HPP_
#define _c4_REGEN_EXTRACTOR_HPP_

#include <string>

#include <c4/yml/node.hpp>
#include "c4/ast/ast.hpp"

namespace c4 {
namespace regen {

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
 *   attr: gui
 * @endcode
 */
struct Extractor
{
    ExtractorType_e m_type{EXTR_ALL};
    std::string m_tag;
    std::string m_attr;

    void load(c4::yml::NodeRef n);
    size_t extract(CXCursorKind kind, c4::ast::TranslationUnit const& tu, std::vector<ast::Entity> *out) const;

    struct Data
    {
        c4::ast::Cursor cursor, tag;
        bool extracted, has_tag;
        inline operator bool() const { return extracted; }
    };
    Extractor::Data extract(c4::ast::Index &idx, c4::ast::Cursor c) const;
};


} // namespace regen
} // namespace c4

#endif /* _c4_REGEN_EXTRACTOR_HPP_ */
