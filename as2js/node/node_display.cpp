// Copyright (c) 2005-2022  Made to Order Software Corp.  All Rights Reserved
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

// self
//
#include    "as2js/node.h"



// snapdev
//
#include    <snapdev/safe_stream.h>


// libutf8
//
#include    <libutf8/iterator.h>


// C++
//
#include    <iomanip>


// last include
//
#include    <snapdev/poison.h>



/** \file
 * \brief Handle the display of a node.
 *
 * In order to debug the compiler, it is extremely practical to have
 * a way to display it in a console. The functions defined here are
 * used for that purpose.
 *
 * The display is pretty complicated because nodes can only have a
 * certain set of flags and attributes and calling the corresponding
 * functions to retrieve these flags and attributes throw if the
 * node type is wrong. For that reason we have a large amount of
 * very specialized code.
 *
 * The function gets 100% coverage from the node test so we are
 * confident that it is 99.9% correct.
 *
 * The output definition let you use a node with the standard output
 * functions of C++ as in:
 *
 * \code
 *      std::cout << my_node << std::endl;
 * \endcode
 *
 * \sa display()
 */


namespace as2js
{


/**********************************************************************/
/**********************************************************************/
/***  NODE DISPLAY  ***************************************************/
/**********************************************************************/
/**********************************************************************/


/** \brief Display a node.
 *
 * This function prints a node in the \p out stream.
 *
 * The function is smart enough to recognize the different type of nodes
 * and thus know what is saved in them and knows how to display all of
 * that information.
 *
 * This is only to display a node in a technical way. It does not attempt
 * to display things in JavaScript or any other language.
 *
 * \param[in] out  The output stream where the node is displayed.
 */
void node::display_data(std::ostream & out) const
{
    // save the output stream flags
    //
    snapdev::safe_stream stream_flags(out);

    auto display_str = [&out](std::string const & str)
    {
        out << ": '";
        for(libutf8::utf8_iterator it(str); it != str.end(); ++it)
        {
            char32_t const wc(*it);
            if(wc < 0x20)
            {
                // show controls as ^<letter>
                out << '^' << static_cast<char>(wc + '@');
            }
            else if(wc < 0x7f)
            {
                if(wc == '\'')
                {
                    out << "\\'";
                }
                else
                {
                    out << static_cast<char>(wc);
                }
            }
            else if(wc < 0x100)
            {
                out << "\\x" << std::hex << static_cast<int>(wc) << std::dec;
            }
            else if(wc < 0x10000)
            {
                out << "\\u" << std::hex << std::setfill('0') << std::setw(4) << static_cast<int>(wc) << std::dec;
            }
            else
            {
                out << "\\U" << std::hex << std::setfill('0') << std::setw(6) << static_cast<int>(wc) << std::dec;
            }
        }
        out << "'";
    };

    // WARNING: somehow g++ views the node_t type as a node type and thus
    //          it recursively calls this function until the stack is full
    //          unless we cast the value to something else
    //
    if(f_type == node_t::NODE_EOF)
    {
        out << std::setw(4) << std::setfill(' ') << static_cast<int>(f_type)
            << std::setfill('\0') << ": " << get_type_name();
    }
    else
    {
        out << std::setw(4) << std::setfill('0') << static_cast<int>(f_type)
            << std::setfill('\0') << ": " << get_type_name();
    }
    if(static_cast<int>(f_type) > ' '
    && static_cast<int>(f_type) < 0x7F)
    {
        out << " = '" << static_cast<char>(f_type) << "'";
    }

    switch(f_type)
    {
    case node_t::NODE_BREAK:
    case node_t::NODE_CONTINUE:
    case node_t::NODE_GOTO:
    case node_t::NODE_INTERFACE:
    case node_t::NODE_LABEL:
    case node_t::NODE_NAMESPACE:
    case node_t::NODE_REGULAR_EXPRESSION:
        display_str(f_str);
        break;

    case node_t::NODE_CATCH:
        out << ":";
        if(f_flags[static_cast<size_t>(flag_t::NODE_CATCH_FLAG_TYPED)])
        {
            out << " TYPED";
        }
        break;

    case node_t::NODE_DIRECTIVE_LIST:
        out << ":";
        if(f_flags[static_cast<size_t>(flag_t::NODE_DIRECTIVE_LIST_FLAG_NEW_VARIABLES)])
        {
            out << " NEW-VARIABLES";
        }
        break;

    case node_t::NODE_ENUM:
        display_str(f_str);
        if(f_flags[static_cast<size_t>(flag_t::NODE_ENUM_FLAG_CLASS)])
        {
            out << " CLASS";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_ENUM_FLAG_INUSE)])
        {
            out << " INUSE";
        }
        break;

    case node_t::NODE_FOR:
        out << ":";
        if(f_flags[static_cast<size_t>(flag_t::NODE_FOR_FLAG_CONST)])
        {
            out << " CONST";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_FOR_FLAG_FOREACH)])
        {
            out << " FOREACH";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_FOR_FLAG_IN)])
        {
            out << " IN";
        }
        break;

    case node_t::NODE_CLASS:
    case node_t::NODE_IDENTIFIER:
    case node_t::NODE_STRING:
    case node_t::NODE_TEMPLATE:
    case node_t::NODE_TEMPLATE_HEAD:
    case node_t::NODE_TEMPLATE_MIDDLE:
    case node_t::NODE_TEMPLATE_TAIL:
    case node_t::NODE_VIDENTIFIER:
        display_str(f_str);
        if(f_flags[static_cast<size_t>(flag_t::NODE_IDENTIFIER_FLAG_WITH)])
        {
            out << " WITH";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_IDENTIFIER_FLAG_TYPED)])
        {
            out << " TYPED";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_IDENTIFIER_FLAG_OPERATOR)]) // [V]IDENTIFIERS-only
        {
            out << " OPERATOR";
        }
        break;

    case node_t::NODE_IMPORT:
        display_str(f_str);
        if(f_flags[static_cast<size_t>(flag_t::NODE_IMPORT_FLAG_IMPLEMENTS)])
        {
            out << " IMPLEMENTS";
        }
        break;

    case node_t::NODE_PACKAGE:
        display_str(f_str);
        if(f_flags[static_cast<size_t>(flag_t::NODE_PACKAGE_FLAG_FOUND_LABELS)])
        {
            out << " FOUND-LABELS";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_PACKAGE_FLAG_REFERENCED)])
        {
            out << " REFERENCED";
        }
        break;

    case node_t::NODE_INTEGER:
        out << ": " << f_int.get() << ", 0x" << std::hex << std::setw(16) << std::setfill('0') << f_int.get() << std::dec << std::setw(0) << std::setfill('\0');
        break;

    case node_t::NODE_FLOATING_POINT:
        out << ": " << f_float.get();
        break;

    case node_t::NODE_CALL:
        out << ":";
        if(f_flags[static_cast<size_t>(flag_t::NODE_FUNCTION_FLAG_OPERATOR)])
        {
            out << " OPERATOR";
        }
        break;

    case node_t::NODE_FUNCTION:
        display_str(f_str);
        if(f_flags[static_cast<size_t>(flag_t::NODE_FUNCTION_FLAG_GETTER)])
        {
            out << " GETTER";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_FUNCTION_FLAG_SETTER)])
        {
            out << " SETTER";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_FUNCTION_FLAG_OUT)])
        {
            out << " OUT";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_FUNCTION_FLAG_VOID)])
        {
            out << " VOID";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_FUNCTION_FLAG_NEVER)])
        {
            out << " NEVER";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_FUNCTION_FLAG_NOPARAMS)])
        {
            out << " NOPARAMS";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_FUNCTION_FLAG_OPERATOR)])
        {
            out << " OPERATOR";
        }
        break;

    case node_t::NODE_PARAM:
        display_str(f_str);
        out << ":";
        if(f_flags[static_cast<size_t>(flag_t::NODE_PARAM_FLAG_CONST)])
        {
            out << " CONST";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_PARAM_FLAG_IN)])
        {
            out << " IN";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_PARAM_FLAG_OUT)])
        {
            out << " OUT";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_PARAM_FLAG_NAMED)])
        {
            out << " NAMED";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_PARAM_FLAG_REST)])
        {
            out << " REST";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_PARAM_FLAG_UNCHECKED)])
        {
            out << " UNCHECKED";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_PARAM_FLAG_UNPROTOTYPED)])
        {
            out << " UNPROTOTYPED";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_PARAM_FLAG_REFERENCED)])
        {
            out << " REFERENCED";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_PARAM_FLAG_PARAMREF)])
        {
            out << " PARAMREF";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_PARAM_FLAG_CATCH)])
        {
            out << " CATCH";
        }
        break;

    case node_t::NODE_PARAM_MATCH:
        out << ":";
        if(f_flags[static_cast<size_t>(flag_t::NODE_PARAM_MATCH_FLAG_UNPROTOTYPED)])
        {
            out << " UNPROTOTYPED";
        }
        break;

    case node_t::NODE_SWITCH:
        out << ":";
        if(f_flags[static_cast<size_t>(flag_t::NODE_SWITCH_FLAG_DEFAULT)])
        {
            out << " DEFAULT";
        }
        break;

    case node_t::NODE_TYPE:
        out << ":";
        if(f_flags[static_cast<size_t>(flag_t::NODE_TYPE_FLAG_MODULO)])
        {
            out << " MODULO";
        }
        break;

    case node_t::NODE_VARIABLE:
    case node_t::NODE_VAR_ATTRIBUTES:
        display_str(f_str);
        if(f_flags[static_cast<size_t>(flag_t::NODE_VARIABLE_FLAG_CONST)])
        {
            out << " CONST";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_VARIABLE_FLAG_FINAL)])
        {
            out << " FINAL";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_VARIABLE_FLAG_LOCAL)])
        {
            out << " LOCAL";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_VARIABLE_FLAG_MEMBER)])
        {
            out << " MEMBER";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_VARIABLE_FLAG_ATTRIBUTES)])
        {
            out << " ATTRIBUTES";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_VARIABLE_FLAG_ENUM)])
        {
            out << " ENUM";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_VARIABLE_FLAG_COMPILED)])
        {
            out << " COMPILED";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_VARIABLE_FLAG_INUSE)])
        {
            out << " INUSE";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_VARIABLE_FLAG_ATTRS)])
        {
            out << " ATTRS";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_VARIABLE_FLAG_DEFINED)])
        {
            out << " DEFINED";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_VARIABLE_FLAG_DEFINING)])
        {
            out << " DEFINING";
        }
        if(f_flags[static_cast<size_t>(flag_t::NODE_VARIABLE_FLAG_TOADD)])
        {
            out << " TOADD";
        }
        break;

    default:
        break;

    }
}


/** \brief Display a node tree.
 *
 * This function displays this node, its children, its children's children,
 * etc. until all the nodes in the tree were displayed.
 *
 * Note that the function knows about the node links, variables, and labels
 * which also get displayed.
 *
 * Because the tree cannot generate loops (the set_parent() function
 * prevents such), we do not have anything that would break the
 * recursivity of the function.
 *
 * The character used to start the string (\p c) changes depending on what
 * we are showing to the user. That way we know whether it is the root (.),
 * a child (-), a variable (=), or a label (:).
 *
 * \param[in,out] out  The output stream.
 * \param[in] indent  The current indentation. We start with 2.
 * \param[in] c  A character to start each line of output.
 */
void node::display(std::ostream& out, int indent, char c) const
{
    // save the output stream flags
    //
    snapdev::safe_stream stream_flags(out);

    // this pointer and indentation
    //
    out << this << ": " << std::setfill('0') << std::setw(2) << indent << std::setfill(' ') << c << std::setw(indent) << "";

    // display node data (integer, string, float, etc.)
    //
    display_data(out);

    // display information about the links
    {
        pointer_t n(f_instance.lock());
        if(n)
        {
            out << " Instance: " << n.get();
        }
    }
    {
        pointer_t n(f_type_node.lock());
        if(n)
        {
            out << " Type node: " << n.get();
        }
    }
    {
        if(f_attribute_node)
        {
            out << " Attribute node: " << f_attribute_node.get();
        }
    }
    {
        pointer_t n(f_goto_exit.lock());
        if(n)
        {
            out << " Goto Exit: " << n.get();
        }
    }
    {
        pointer_t n(f_goto_enter.lock());
        if(n)
        {
            out << " Goto Enter: " << n.get();
        }
    }

    // display the different attributes if any
    //
    struct display_attributes
    {
        display_attributes(std::ostream & out, attribute_set_t attrs)
            : f_out(out)
            , f_attributes(attrs)
        {
            display_attribute(attribute_t::NODE_ATTR_PUBLIC);
            display_attribute(attribute_t::NODE_ATTR_PRIVATE);
            display_attribute(attribute_t::NODE_ATTR_PROTECTED);
            display_attribute(attribute_t::NODE_ATTR_INTERNAL);
            display_attribute(attribute_t::NODE_ATTR_TRANSIENT);
            display_attribute(attribute_t::NODE_ATTR_VOLATILE);

            display_attribute(attribute_t::NODE_ATTR_STATIC);
            display_attribute(attribute_t::NODE_ATTR_ABSTRACT);
            display_attribute(attribute_t::NODE_ATTR_VIRTUAL);
            display_attribute(attribute_t::NODE_ATTR_ARRAY);
            display_attribute(attribute_t::NODE_ATTR_INLINE);

            display_attribute(attribute_t::NODE_ATTR_REQUIRE_ELSE);
            display_attribute(attribute_t::NODE_ATTR_ENSURE_THEN);

            display_attribute(attribute_t::NODE_ATTR_NATIVE);

            display_attribute(attribute_t::NODE_ATTR_DEPRECATED);
            display_attribute(attribute_t::NODE_ATTR_UNSAFE);

            display_attribute(attribute_t::NODE_ATTR_CONSTRUCTOR);

            //display_attribute(attribute_t::NODE_ATTR_CONST); -- this is a flag, not needed here
            display_attribute(attribute_t::NODE_ATTR_FINAL);
            display_attribute(attribute_t::NODE_ATTR_ENUMERABLE);

            display_attribute(attribute_t::NODE_ATTR_TRUE);
            display_attribute(attribute_t::NODE_ATTR_FALSE);
            display_attribute(attribute_t::NODE_ATTR_UNUSED);

            display_attribute(attribute_t::NODE_ATTR_DYNAMIC);

            display_attribute(attribute_t::NODE_ATTR_FOREACH);
            display_attribute(attribute_t::NODE_ATTR_NOBREAK);
            display_attribute(attribute_t::NODE_ATTR_AUTOBREAK);

            display_attribute(attribute_t::NODE_ATTR_TYPE);

            display_attribute(attribute_t::NODE_ATTR_DEFINED);
        }

        void display_attribute(attribute_t a)
        {
            if(f_attributes[static_cast<size_t>(a)])
            {
                if(f_first)
                {
                    f_first = false;
                    f_out << " attrs:";
                }
                f_out << " " << node::attribute_to_string(a);
            }
        }

        std::ostream &              f_out;
        bool                        f_first = true;
        attribute_set_t             f_attributes;
    } display_attr(out, f_attributes);

    // end the line with our position
    //
    out << " (" << f_position << ")";

    if(f_lock > 0)
    {
        out << " Locked: " << static_cast<int32_t>(f_lock);
    }

    out << std::endl;

    // now print children
    //
    for(size_t idx(0); idx < f_children.size(); ++idx)
    {
        f_children[idx]->display(out, indent + 1, '-');
    }

    // now print variables
    //
    for(size_t idx(0); idx < f_variables.size(); ++idx)
    {
        pointer_t variable(f_variables[idx].lock());
        if(variable)
        {
            variable->display(out, indent + 1, '=');
        }
    }

    // now print labels
    //
    for(auto const & it : f_labels)
    {
        pointer_t label(it.second.lock());
        if(label)
        {
            label->display(out, indent + 1, ':');
        }
    }
}


/** \brief Send a node to the specified output stream.
 *
 * This function prints a node to the output stream. The printing is very
 * technical and mainly used to debug the node tree while parsing,
 * compiling, optimizing, and generating the final output.
 *
 * \param[in,out] out  The output stream.
 * \param[in] node  The node to print in the output stream.
 *
 * \return A reference to the output stream.
 */
std::ostream & operator << (std::ostream & out, node const & node)
{
    node.display(out, 2, '.');
    return out;
}


}
// namespace as2js

// vim: ts=4 sw=4 et
