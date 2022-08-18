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
#include    "optimizer_tables.h"

#include    "as2js/exceptions.h"

// Low level matching tables
#include    "optimizer_matches.ci"
#include    "optimizer_values.ci"

// Optimization tables 
#include    "optimizer_optimize.ci"

// Actual optimization entries
#include    "optimizer_additive.ci"
#include    "optimizer_assignments.ci"
#include    "optimizer_bitwise.ci"
#include    "optimizer_compare.ci"
#include    "optimizer_conditional.ci"
#include    "optimizer_equality.ci"
#include    "optimizer_logical.ci"
#include    "optimizer_match.ci"
#include    "optimizer_multiplicative.ci"
#include    "optimizer_relational.ci"
#include    "optimizer_statements.ci"


// last include
//
#include    <snapdev/poison.h>



namespace as2js
{

/** \brief The optimizer sub-namespace.
 *
 * This namespace is used to define all the optimizer internal
 * tables, functions, and classes.
 *
 * We have a separate namespace because the number of tables
 * in the optimizer is really large and could clash with other
 * parts of the libraries.
 */
namespace optimizer_details
{



/** \brief Hide all optimizer tables implementation details.
 *
 * This unnamed namespace is used to further hide all the optimizer
 * details.
 */
namespace
{

/** \brief Table holding all the optimization tables.
 *
 * We have one additional level for no technical reason other
 * than it makes it a bit cleaner to definie one table per
 * category of optimization and conglomerate them in one
 * larger table here.
 *
 * The table size is known and defined below. It is not
 * otherwise terminated with a null entry or anything like
 * that.
 *
 * The table is only defined locally and used in the optimize_tree()
 * function below.
 */
optimization_tables_t const g_optimizer_tables[] =
{
    {
        POINTER_AND_COUNT(g_optimizer_additive_table)
    },
    {
        POINTER_AND_COUNT(g_optimizer_assignments_table)
    },
    {
        POINTER_AND_COUNT(g_optimizer_bitwise_table)
    },
    {
        POINTER_AND_COUNT(g_optimizer_compare_table)
    },
    {
        POINTER_AND_COUNT(g_optimizer_conditional_table)
    },
    {
        POINTER_AND_COUNT(g_optimizer_equality_table)
    },
    {
        POINTER_AND_COUNT(g_optimizer_logical_table)
    },
// regex is not well supported before 4.9.0
#if (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)
    {
        POINTER_AND_COUNT(g_optimizer_match_table)
    },
#endif
    {
        POINTER_AND_COUNT(g_optimizer_multiplicative_table)
    },
    {
        POINTER_AND_COUNT(g_optimizer_relational_table)
    },
    {
        POINTER_AND_COUNT(g_optimizer_statements_table)
    }
};


/** \brief The size of the table of tables.
 *
 * This variable holds the number of tables defined in the
 * g_optimizer_tables. It is always larger than zero.
 */
size_t g_optimizer_tables_count = sizeof(g_optimizer_tables) / sizeof(g_optimizer_tables[0]);


/** \brief Attempt to apply one optimization against this node.
 *
 * This function applies the optimization entry defined in \p entry to
 * the specified node tree. If the node tree matches that entry, then
 * the function proceeds and optimizes the node tree and returns true.
 *
 * Note that the root node (the input node) may itself be changed.
 *
 * \internal
 *
 * \param[in] n  The tree of nodes being optimized.
 * \param[in] entry  The entry definining one optimization.
 *
 * \return If this optimization was applied, true, otherwise false.
 */
bool apply_optimization(node::pointer_t & n, optimization_entry_t const * entry)
{
    if((entry->f_flags & OPTIMIZATION_ENTRY_FLAG_UNSAFE_MATH) != 0)
    {
        // TODO: test whether the Unsafe Math option is turned on, if not
        //       skip this optimization
    }

    node::vector_of_pointers_t node_array;
    if(match_tree(node_array, n, entry->f_match, entry->f_match_count, 0))
    {
//#if defined(_DEBUG) || defined(DEBUG)
//        std::cout << "Optimize with: " << entry->f_name << "\n";
//#endif
        node::pointer_t parent(n->get_parent());
        if(!parent)
        {
            // if you create your own tree of node, it is possible to
            // reach this statement... otherwise, the top should always
            // have a NODE_PROGRAM which cannot be optimized
            throw internal_error("INTERNAL ERROR: somehow the optimizer is optimizing a node without a parent.");
        }
        std::size_t index(n->get_offset());

        apply_functions(node_array, entry->f_optimize, entry->f_optimize_count);

        // in case the node pointer changed (which is nearly always)
        n = parent->get_child(index);
        return true;
    }

    return false;
}


}
// noname namespace


/** \brief Optimize a tree of nodes as much as possible.
 *
 * This function checks the specified node against all the available
 * optimizations defined in the optimizer.
 *
 * \todo
 * Look into losing the recusirve aspect of this function (so the
 * entire tree of nodes gets checked.)
 *
 * \param[in] n  The node being checked.
 *
 * \return true if any optimization was applied
 */
bool optimize_tree(node::pointer_t n)
{
    bool result(false);

    // accept empty nodes, just ignore them
    if(!n || n->get_type() == node::node_t::NODE_UNKNOWN)
    {
        return result;
    }

    // we need to optimize the child most nodes first
    size_t const max_children(n->get_children_size());
    for(size_t idx(0); idx < max_children; ++idx)
    {
        // Note: although the child at index 'idx' may change
        //       the number of children in 'node' cannot change
        if(optimize_tree(n->get_child(idx))) // recursive
        {
            result = true;
        }
    }

    for(;;)
    {
        bool repeat(false);
        for(size_t i(0); i < g_optimizer_tables_count; ++i)
        {
            optimization_table_t const *table(g_optimizer_tables[i].f_table);
            size_t const table_max(g_optimizer_tables[i].f_table_count);
            for(size_t j(0); j < table_max; ++j)
            {
                optimization_entry_t const *entry(table[j].f_entry);
                size_t const entry_max(table[j].f_entry_count);
                for(size_t k(0); k < entry_max; ++k)
                {
                    if(apply_optimization(n, entry + k))
                    {
                        repeat = true;

                        // at least one optimization was applied
                        result = true;

                        // TBD: would it be faster to immediately
                        //      repeat from the start?
                    }
                }
            }
        }

        // anything was optimized?
        if(!repeat)
        {
            // we are done
            break;
        }
    }

    return result;
}



} // namespace optimizer_details
} // namespace as2js
// vim: ts=4 sw=4 et
