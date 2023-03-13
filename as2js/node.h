// Copyright (c) 2005-2023  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/as2js
// contact@m2osw.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
#pragma once

// self
//
#include    <as2js/position.h>


// C++
//
#include    <bitset>
#include    <functional>
#include    <limits>
#include    <map>
#include    <memory>
#include    <vector>


namespace as2js
{



// NOTE: The attributes (Attrs) are defined in the second pass
//       whenever we transform the identifiers in actual attribute
//       flags. While creating the tree, the attributes are always
//       set to 0.

// node related depth parameter
typedef ssize_t             depth_t;

static depth_t const        MATCH_NOT_FOUND = 0;
static depth_t const        MATCH_HIGHEST_DEPTH = 1;
static depth_t const        MATCH_LOWEST_DEPTH = std::numeric_limits<int>::max() / 2;

// the node type is often referenced as a token
enum class node_t
{
    NODE_EOF                    = -1,       // when reading after the end of the file
    NODE_UNKNOWN                = 0,        // node still uninitialized

    // here are all the punctuation as themselves
    // (i.e. '<', '>', '=', '+', '-', etc.)
    NODE_ADD                    = '+',      // 0x2B
    NODE_ASSIGNMENT             = '=',      // 0x3D
    NODE_BITWISE_AND            = '&',      // 0x26
    NODE_BITWISE_NOT            = '~',      // 0x7E
    NODE_BITWISE_OR             = '|',      // 0x7C
    NODE_BITWISE_XOR            = '^',      // 0x5E
    NODE_CLOSE_CURVLY_BRACKET   = '}',      // 0x7D
    NODE_CLOSE_PARENTHESIS      = ')',      // 0x29
    NODE_CLOSE_SQUARE_BRACKET   = ']',      // 0x5D
    NODE_COLON                  = ':',      // 0x3A
    NODE_COMMA                  = ',',      // 0x2C
    NODE_CONDITIONAL            = '?',      // 0x3F
    NODE_DIVIDE                 = '/',      // 0x2F
    NODE_GREATER                = '>',      // 0x3E
    NODE_LESS                   = '<',      // 0x3C
    NODE_LOGICAL_NOT            = '!',      // 0x21
    NODE_MODULO                 = '%',      // 0x25
    NODE_MULTIPLY               = '*',      // 0x2A
    NODE_OPEN_CURVLY_BRACKET    = '{',      // 0x7B
    NODE_OPEN_PARENTHESIS       = '(',      // 0x28
    NODE_OPEN_SQUARE_BRACKET    = '[',      // 0x5B
    NODE_MEMBER                 = '.',      // 0x2E
    NODE_SEMICOLON              = ';',      // 0x3B
    NODE_SUBTRACT               = '-',      // 0x2D

    // The following are composed tokens or based on non-ASCII characters
    // (operators, keywords, strings, numbers...)
    NODE_other = 1000,

    NODE_ABSTRACT,
    NODE_ALMOST_EQUAL,
    NODE_ARRAY,
    NODE_ARRAY_LITERAL,
    NODE_ARROW,
    NODE_AS,
    NODE_ASSIGNMENT_ADD,
    NODE_ASSIGNMENT_BITWISE_AND,
    NODE_ASSIGNMENT_BITWISE_OR,
    NODE_ASSIGNMENT_BITWISE_XOR,
    NODE_ASSIGNMENT_COALESCE,
    NODE_ASSIGNMENT_DIVIDE,
    NODE_ASSIGNMENT_LOGICAL_AND,
    NODE_ASSIGNMENT_LOGICAL_OR,
    NODE_ASSIGNMENT_LOGICAL_XOR,
    NODE_ASSIGNMENT_MAXIMUM,
    NODE_ASSIGNMENT_MINIMUM,
    NODE_ASSIGNMENT_MODULO,
    NODE_ASSIGNMENT_MULTIPLY,
    NODE_ASSIGNMENT_POWER,
    NODE_ASSIGNMENT_ROTATE_LEFT,
    NODE_ASSIGNMENT_ROTATE_RIGHT,
    NODE_ASSIGNMENT_SHIFT_LEFT,
    NODE_ASSIGNMENT_SHIFT_RIGHT,
    NODE_ASSIGNMENT_SHIFT_RIGHT_UNSIGNED,
    NODE_ASSIGNMENT_SUBTRACT,
    NODE_ASYNC,
    NODE_ATTRIBUTES,
    NODE_AUTO,
    NODE_AWAIT,
    NODE_BOOLEAN,
    NODE_BREAK,
    NODE_BYTE,
    NODE_CALL,
    NODE_CASE,
    NODE_CATCH,
    NODE_CHAR,
    NODE_CLASS,
    NODE_COALESCE,
    NODE_COMPARE,
    NODE_CONST,
    NODE_CONTINUE,
    NODE_DEBUGGER,
    NODE_DECREMENT,
    NODE_DEFAULT,
    NODE_DELETE,
    NODE_DIRECTIVE_LIST,
    NODE_DO,
    NODE_DOUBLE,
    NODE_ELSE,
    NODE_EMPTY,
    NODE_ENSURE,
    NODE_ENUM,
    NODE_EQUAL,
    NODE_EXCLUDE,
    NODE_EXTENDS,
    NODE_EXTERN,
    NODE_EXPORT,
    NODE_FALSE,
    NODE_FINAL,
    NODE_FINALLY,
    NODE_FLOAT,             // "float" keyword
    NODE_FLOATING_POINT,    // a literal float (i.e. 3.14159)
    NODE_FOR,
    NODE_FUNCTION,
    NODE_GOTO,
    NODE_GREATER_EQUAL,
    NODE_IDENTIFIER,
    NODE_IDENTITY,
    NODE_IF,
    NODE_IF_FALSE,
    NODE_IF_TRUE,
    NODE_IMPLEMENTS,
    NODE_IMPORT,
    NODE_IN,
    NODE_INCLUDE,
    NODE_INCREMENT,
    NODE_INLINE,
    NODE_INSTANCEOF,
    NODE_INTEGER,           // a literal integer (i.e. 123)
    NODE_INTERFACE,
    NODE_INVARIANT,
    NODE_IS,
    NODE_LABEL,
    NODE_LESS_EQUAL,
    NODE_LIST,
    NODE_LOGICAL_AND,
    NODE_LOGICAL_OR,
    NODE_LOGICAL_XOR,
    NODE_LONG,
    NODE_MATCH,
    NODE_MAXIMUM,
    NODE_MINIMUM,
    NODE_NAME,
    NODE_NAMESPACE,
    NODE_NATIVE,
    NODE_NEGATE,
    NODE_NEW,
    NODE_NOT_EQUAL,
    NODE_NOT_MATCH,
    NODE_NULL,
    NODE_OBJECT_LITERAL,
    NODE_OPTIONAL_MEMBER,
    NODE_PACKAGE,
    NODE_PARAM,
    NODE_PARAMETERS,
    NODE_PARAM_MATCH,
    NODE_POST_DECREMENT,
    NODE_POST_INCREMENT,
    NODE_POWER,
    NODE_PRIVATE,
    NODE_PROGRAM,
    NODE_PROTECTED,
    NODE_PUBLIC,
    NODE_RANGE,
    NODE_REGULAR_EXPRESSION,
    NODE_REQUIRE,
    NODE_REST,
    NODE_RETURN,
    NODE_ROOT,
    NODE_ROTATE_LEFT,
    NODE_ROTATE_RIGHT,
    NODE_SCOPE,
    NODE_SET,
    NODE_SHIFT_LEFT,
    NODE_SHIFT_RIGHT,
    NODE_SHIFT_RIGHT_UNSIGNED,
    NODE_SHORT,
    NODE_SMART_MATCH,
    NODE_STATIC,
    NODE_STRICTLY_EQUAL,
    NODE_STRICTLY_NOT_EQUAL,
    NODE_STRING,
    NODE_SUPER,
    NODE_SWITCH,
    NODE_SYNCHRONIZED,
    NODE_TEMPLATE,
    NODE_TEMPLATE_HEAD,
    NODE_TEMPLATE_MIDDLE,
    NODE_TEMPLATE_TAIL,
    NODE_THEN,
    NODE_THIS,
    NODE_THROW,
    NODE_THROWS,
    NODE_TRANSIENT,
    NODE_TRUE,
    NODE_TRY,
    NODE_TYPE,
    NODE_TYPEOF,
    NODE_UNDEFINED,
    NODE_USE,
    NODE_VAR,
    NODE_VARIABLE,
    NODE_VAR_ATTRIBUTES,
    NODE_VIDENTIFIER,
    NODE_VOID,
    NODE_VOLATILE,
    NODE_WHILE,
    NODE_WITH,
    NODE_YIELD,

    NODE_max     // mark the limit
};

// some nodes use flags, all of which are managed in one bitset
//
// (Note that our Nodes are smart and make use of the function named
// verify_flag() to make sure that this specific node can
// indeed be given such flag)
//
enum class flag_t
{
    // NODE_CATCH
    NODE_CATCH_FLAG_TYPED,

    // NODE_DIRECTIVE_LIST
    NODE_DIRECTIVE_LIST_FLAG_NEW_VARIABLES,

    // NODE_ENUM
    NODE_ENUM_FLAG_CLASS,
    NODE_ENUM_FLAG_INUSE,

    // NODE_FOR
    NODE_FOR_FLAG_CONST,
    NODE_FOR_FLAG_FOREACH,
    NODE_FOR_FLAG_IN,

    // NODE_FUNCTION
    NODE_FUNCTION_FLAG_GETTER,
    NODE_FUNCTION_FLAG_SETTER,
    NODE_FUNCTION_FLAG_OUT,
    NODE_FUNCTION_FLAG_VOID,
    NODE_FUNCTION_FLAG_NEVER,
    NODE_FUNCTION_FLAG_NOPARAMS,
    NODE_FUNCTION_FLAG_OPERATOR,

    // NODE_IDENTIFIER, NODE_VIDENTIFIER, NODE_STRING
    NODE_IDENTIFIER_FLAG_WITH,
    NODE_IDENTIFIER_FLAG_TYPED,
    NODE_IDENTIFIER_FLAG_OPERATOR,

    // NODE_IMPORT
    NODE_IMPORT_FLAG_IMPLEMENTS,

    // NODE_PACKAGE
    NODE_PACKAGE_FLAG_FOUND_LABELS,
    NODE_PACKAGE_FLAG_REFERENCED,

    // NODE_PARAM
    NODE_PARAM_FLAG_CONST,
    NODE_PARAM_FLAG_IN,
    NODE_PARAM_FLAG_OUT,
    NODE_PARAM_FLAG_NAMED,
    NODE_PARAM_FLAG_REST,
    NODE_PARAM_FLAG_UNCHECKED,
    NODE_PARAM_FLAG_UNPROTOTYPED,
    NODE_PARAM_FLAG_REFERENCED,         // referenced from a parameter or a variable
    NODE_PARAM_FLAG_PARAMREF,           // referenced from another parameter
    NODE_PARAM_FLAG_CATCH,              // a parameter defined in a catch()

    // NODE_PARAM_MATCH
    NODE_PARAM_MATCH_FLAG_UNPROTOTYPED,

    // NODE_SWITCH
    NODE_SWITCH_FLAG_DEFAULT,           // we found a 'default:' label in that switch

    // NODE_TYPE
    NODE_TYPE_FLAG_MODULO,              // modulo numeric type declaration

    // NODE_VARIABLE, NODE_VAR_ATTRIBUTES
    NODE_VARIABLE_FLAG_CONST,
    NODE_VARIABLE_FLAG_FINAL,
    NODE_VARIABLE_FLAG_LOCAL,
    NODE_VARIABLE_FLAG_MEMBER,
    NODE_VARIABLE_FLAG_ATTRIBUTES,
    NODE_VARIABLE_FLAG_ENUM,            // there is a NODE_SET and it somehow needs to be copied
    NODE_VARIABLE_FLAG_COMPILED,        // Expression() was called on the NODE_SET
    NODE_VARIABLE_FLAG_INUSE,           // this variable was referenced
    NODE_VARIABLE_FLAG_ATTRS,           // currently being read for attributes (to avoid loops)
    NODE_VARIABLE_FLAG_DEFINED,         // was already parsed
    NODE_VARIABLE_FLAG_DEFINING,        // currently defining, cannot read
    NODE_VARIABLE_FLAG_TOADD,           // to be added in the directive list
    NODE_VARIABLE_FLAG_TEMPORARY,       // when creating assembly, a temporary we can save on the stack

    NODE_FLAG_max
};

typedef std::bitset<static_cast<size_t>(flag_t::NODE_FLAG_max)>     flag_set_t;

// some nodes use flags, all of which are managed in one bitset
//
// (Note that our Nodes are smart and make use of the function named
// verify_flag() to make sure that this specific node can
// indeed be given such flag)
enum class attribute_t
{
    // member visibility
    NODE_ATTR_PUBLIC,
    NODE_ATTR_PRIVATE,
    NODE_ATTR_PROTECTED,
    NODE_ATTR_INTERNAL,
    NODE_ATTR_TRANSIENT, // variables only, skip when serializing a class
    NODE_ATTR_VOLATILE, // variable only

    // function member type
    NODE_ATTR_STATIC,
    NODE_ATTR_ABSTRACT,
    NODE_ATTR_VIRTUAL,
    NODE_ATTR_ARRAY,
    NODE_ATTR_INLINE,

    // function contract
    NODE_ATTR_REQUIRE_ELSE,
    NODE_ATTR_ENSURE_THEN,

    // function/variable is defined in your system (execution env.)
    // you won't find a body for these functions; the variables
    // will likely be read-only
    NODE_ATTR_NATIVE,

    // function/variable is still defined, but should not be used
    // (using generates a "foo deprecated" warning or equivalent)
    NODE_ATTR_DEPRECATED,
    NODE_ATTR_UNSAFE, // i.e. eval()

    // functions/variables are accessible externally
    NODE_ATTR_EXTERN,

    // TODO: add a way to mark functions/variables as browser specific
    //       so we can easily tell the user that it should not be used
    //       or with caution (i.e. #ifdef browser-name ...)

    // operator overload (function member)
    // Contructor -> another way to construct this type of objects
    NODE_ATTR_CONSTRUCTOR,

    // function & member constrains
    // CONST is not currently available as an attribute (see flags instead)
    //NODE_ATTR_CONST,
    NODE_ATTR_FINAL,
    NODE_ATTR_ENUMERABLE,

    // conditional compilation
    NODE_ATTR_TRUE,
    NODE_ATTR_FALSE,
    NODE_ATTR_UNUSED,                   // if definition is used, error!

    // class attribute (whether a class can be enlarged at run time)
    NODE_ATTR_DYNAMIC,

    // switch attributes
    NODE_ATTR_FOREACH,
    NODE_ATTR_NOBREAK,
    NODE_ATTR_AUTOBREAK,

    // type attribute, to mark all the nodes within a type expression
    NODE_ATTR_TYPE,

    // The following is to make sure we never define the attributes more
    // than once. In itself it is not an attribute.
    NODE_ATTR_DEFINED,

    // max used to know the number of entries and define our bitset
    NODE_ATTR_max
};

typedef std::bitset<static_cast<int>(attribute_t::NODE_ATTR_max)>     attribute_set_t;

//enum class link_t : uint32_t
//{
//    LINK_INSTANCE = 0,
//    LINK_TYPE,
//    LINK_ATTRIBUTES,    // this is the list of identifiers
//    LINK_GOTO_EXIT,
//    LINK_GOTO_ENTER,
//    LINK_max
//};

enum class compare_mode_t
{
    COMPARE_STRICT,     // ===
    COMPARE_LOOSE,      // ==
    COMPARE_SMART       // ~~
};


// Note: the std::enable_shared_from_this<> has no virtual destructor
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"

class node
    : public std::enable_shared_from_this<node>
{
public:
    typedef std::shared_ptr<node>               pointer_t;
    typedef std::weak_ptr<node>                 weak_pointer_t;
    typedef std::map<std::string, weak_pointer_t>
                                                map_of_weak_pointers_t;
    typedef std::vector<pointer_t>              vector_of_pointers_t;
    typedef std::vector<weak_pointer_t>         vector_of_weak_pointers_t;

    typedef std::function<bool(pointer_t)>      node_filter_t;

                                node(node_t type);
    virtual                     ~node(); // virtual because of shared pointers

    /** \brief Do not allow direct copies of nodes.
     *
     * It is not safe to just copy a node because a node is part of a
     * tree (parent, child, siblings...) and a copy would not work.
     */
                                node(node const &) = delete;

    /** \brief Do not allow direct copies of nodes.
     *
     * It is not safe to just copy a node because a node is part of a
     * tree (parent, child, siblings...) and a copy would not work.
     */
    node &                      operator = (node const &) = delete;

    node_t                      get_type() const;
    char const *                get_type_name() const;
    static char const *         type_to_string(node_t type);
    void                        set_type_node(node::pointer_t node);
    pointer_t                   get_type_node() const;

    bool                        is_number() const;
    bool                        is_nan() const;
    bool                        is_integer() const;
    bool                        is_floating_point() const;
    bool                        is_boolean() const;
    bool                        is_true() const;
    bool                        is_false() const;
    bool                        is_string() const;
    bool                        is_undefined() const;
    bool                        is_null() const;
    bool                        is_identifier() const;
    bool                        is_literal() const;

    // basic conversions
    void                        to_unknown();
    bool                        to_as();
    node_t                      to_boolean_type_only() const;
    bool                        to_boolean();
    bool                        to_call();
    bool                        to_identifier();
    bool                        to_integer();
    bool                        to_floating_point();
    bool                        to_label();
    bool                        to_number();
    bool                        to_operator(node::pointer_t id);
    bool                        to_string();
    void                        to_videntifier();
    void                        to_var_attributes();

    void                        set_boolean(bool value);
    void                        set_integer(integer const & value);
    void                        set_floating_point(floating_point const & value);
    void                        set_string(std::string const & value);

    bool                        get_boolean() const;
    integer                     get_integer() const;
    floating_point              get_floating_point() const;
    std::string const &         get_string() const;

    static compare_t            compare(node::pointer_t const lhs, node::pointer_t const rhs, compare_mode_t const mode);

    pointer_t                   clone_basic_node() const;
    pointer_t                   create_replacement(node_t type) const;

    // check flags
    bool                        get_flag(flag_t f) const;
    void                        set_flag(flag_t f, bool v);
    bool                        compare_all_flags(flag_set_t const& s) const;

    // check attributes
    void                        set_attribute_node(pointer_t n);
    pointer_t                   get_attribute_node() const;
    bool                        get_attribute(attribute_t const a) const;
    void                        set_attribute(attribute_t const a, bool const v);
    void                        set_attribute_tree(attribute_t const a, bool const v);
    bool                        compare_all_attributes(attribute_set_t const& s) const;
    static char const *         attribute_to_string(attribute_t const a);

    // various nodes are assigned an "instance" (direct link to actual declaration)
    void                        set_instance(pointer_t n);
    pointer_t                   get_instance() const;

    // switch operator: switch(...) with(<operator>)
    node_t                      get_switch_operator() const;
    void                        set_switch_operator(node_t op);

    // goto / label
    void                        set_goto_enter(pointer_t n);
    void                        set_goto_exit(pointer_t n);
    pointer_t                   get_goto_enter() const;
    pointer_t                   get_goto_exit() const;

    // handle function parameters (reorder and depth)
    void                        set_param_size(size_t size);
    std::size_t                 get_param_size() const;
    depth_t                     get_param_depth(std::size_t j) const;
    void                        set_param_depth(std::size_t j, depth_t depth);
    std::size_t                 get_param_index(std::size_t idx) const; // returns 'j'
    void                        set_param_index(std::size_t idx, size_t j);

    void                        set_position(position const & pos);
    position const &            get_position() const;

    bool                        has_side_effects() const;

    bool                        is_locked() const;
    void                        lock();
    void                        unlock();

    std::size_t                 get_offset() const;

    void                        set_parent(pointer_t parent = pointer_t(), int index = -1);
    pointer_t                   get_parent() const;

    std::size_t                 get_children_size() const;
    void                        replace_with(pointer_t node);
    void                        delete_child(int index);
    void                        append_child(pointer_t child);
    void                        insert_child(int index, pointer_t child);
    void                        set_child(int index, pointer_t child);
    pointer_t                   get_child(int index) const;
    pointer_t                   find_first_child(node_t type) const;
    pointer_t                   find_next_child(pointer_t start, node_t type) const;
    pointer_t                   find_descendent(node_t type, node_filter_t filter) const;
    void                        clean_tree();

    void                        add_variable(pointer_t variable);
    std::size_t                 get_variable_size() const;
    pointer_t                   get_variable(int index) const;

    void                        add_label(pointer_t label);
    pointer_t                   find_label(std::string const & name) const;

    static char const *         operator_to_string(node_t op);
    static node_t               string_to_operator(std::string const & str);

    void                        display(std::ostream& out, int indent, char c) const;

private:
    typedef std::vector<int32_t>    param_depth_t;
    typedef std::vector<uint32_t>   param_index_t;

    // verify different parameters
    void                        verify_flag(flag_t f) const;
    void                        verify_attribute(attribute_t const f) const;
    bool                        verify_exclusive_attributes(attribute_t f) const;
    void                        modifying() const;

    // output a node to out (on your end, use the << operator)
    void                        display_data(std::ostream & out) const;

    // define the node type
    node_t                      f_type = node_t::NODE_UNKNOWN;
    weak_pointer_t              f_type_node = weak_pointer_t();
    flag_set_t                  f_flags = flag_set_t();
    pointer_t                   f_attribute_node = pointer_t();
    attribute_set_t             f_attributes = attribute_set_t();
    node_t                      f_switch_operator = node_t::NODE_UNKNOWN;

    // whether this node is currently locked
    std::int32_t                f_lock = 0;

    // location where the node was found (filename, line #, etc.)
    position                    f_position = position();

    // data of this node
    integer                     f_int = integer();
    floating_point              f_float = floating_point();
    std::string                 f_str = std::string();

    // function parameters
    param_depth_t               f_param_depth = param_depth_t();
    param_index_t               f_param_index = param_index_t();

    // parent children node tree handling
    weak_pointer_t              f_parent = weak_pointer_t();
    std::int32_t                f_offset = 0;   // offset (index) in parent array of children -- set by compiler, should probably be removed...
    vector_of_pointers_t        f_children = vector_of_pointers_t();
    weak_pointer_t              f_instance = weak_pointer_t();

    // goto nodes
    weak_pointer_t              f_goto_enter = weak_pointer_t();
    weak_pointer_t              f_goto_exit = weak_pointer_t();

    // other connections between nodes
    vector_of_weak_pointers_t   f_variables = vector_of_weak_pointers_t();
    map_of_weak_pointers_t      f_labels = map_of_weak_pointers_t();
};

#pragma GCC diagnostic pop


std::ostream & operator << (std::ostream & out, node const & n);



// Stack based locking of nodes
class node_lock
{
public:
                node_lock(node::pointer_t n);
                ~node_lock();

    // premature unlock
    void        unlock();

private:
    node::pointer_t f_node = node::pointer_t();
};


} // namespace as2js
// vim: ts=4 sw=4 et
