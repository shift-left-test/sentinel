#include <stdio.h>

#include <gcc-plugin.h>
#include <tree.h>
#include <plugin-version.h>
#include <print-tree.h>
#include <tree-iterator.h>
#include <tree-pretty-print.h>
#include <dumpfile.h>

FILE *mutants_file;
int plugin_is_GPL_compatible;

#define NIY do_niy(pp, node, flags)

// bitwise
#define BOR "BOR"
// shift
#define SOR "SOR"
// arithmetic
#define AOR "AOR"
// logical
#define LCR "LCR"
// relational
#define ROR "ROR"
// statement
#define SBR "SBR"
// unary
#define UOR "UOR"
// unary operator insertion
#define UOI "UOI"
// UNKNOWN
#define UNK "UNKNOWN"

int dump_generic_node(void *pp, tree node, int spc, int flags, bool is_stmt);

void mutation_target_stmt(const char *mutant_type, tree node)
{
    tree_code code = TREE_CODE(node);
    expanded_location xloc = expand_location(EXPR_LOCATION(node));
    if (xloc.file)
    {
        fprintf(stdout, "%s (%d) [%s]", mutant_type, code, xloc.file);
        source_range r = EXPR_LOCATION_RANGE(node);
        if (r.m_start && r.m_finish)
        {
            expanded_location xloc_start = expand_location(r.m_start);
            expanded_location xloc_finish = expand_location(r.m_finish);
            fprintf(stdout, "    [%d:%d] ~ [%d:%d]", xloc_start.line, xloc_start.column, xloc_finish.line, xloc_finish.column);
        }
        fprintf(stdout, "\n");
    }
}

void mutation_target_expr(const char *mutant_type, tree node)
{
    if (TREE_VISITED(node))
        return;
    
    tree_code code = TREE_CODE(node);
    const char *op = op_symbol_code(code);
    expanded_location xloc = expand_location(EXPR_LOCATION(node));
    source_range r = EXPR_LOCATION_RANGE(node);
    expanded_location xloc_start = expand_location(r.m_start);
    expanded_location xloc_finish = expand_location(r.m_finish);

    if (xloc.file)
    {
        // file, line, column, mutant_type, operator, code_name
        fprintf(mutants_file,
            "%s, \"%s\", "
            "\"%s\", %d, %d, "
            "%s\n", 
            mutant_type, op,
            xloc.file, xloc.line, xloc.column,
            get_tree_code_name(code));
    }
}

void do_niy(void *pp, const_tree node, int flags)
{
    int i, len;

    //pp_string (pp, "<<< Unknown tree: ");
    //pp_string (pp, get_tree_code_name (TREE_CODE (node)));

    if (EXPR_P(node))
    {
        len = TREE_OPERAND_LENGTH(node);
        for (i = 0; i < len; ++i)
        {
            //newline_and_indent (pp, 2);
            dump_generic_node(pp, TREE_OPERAND(node, i), 2, flags, false);
        }
    }

    //pp_string (pp, " >>>");
}

void pp_tree_identifier(void *pp, tree id)
{
    //fprintf(stderr, "Identifier: %s\n", IDENTIFIER_POINTER(id));
}

void dump_location(void *buffer, location_t loc)
{
    //expanded_location xloc = expand_location(loc);
    //fprintf(stderr, "[%s:%d:%d]\n", xloc.file ? xloc.file : "", xloc.line, xloc.column);
    /*
    source_range r = EXPR_LOCATION_RANGE(node);
    if (r.m_start)
    {
        xloc = expand_location(r.m_start);
        fprintf(stderr, " start: %s:%d:%d\n", xloc.file, xloc.line, xloc.column);
    }
    if (r.m_finish)
    {
        xloc = expand_location(r.m_finish);
        fprintf(stderr, " finish: %s:%d:%d\n", xloc.file, xloc.line, xloc.column);
    }
    */
}

void dump_fancy_name(void *pp, tree name)
{
    fprintf(stderr, "dump_fancy_name\n");
}

void dump_decl_name(void *pp, tree node, int flags)
{
    if (DECL_NAME(node))
    {
        if ((flags & TDF_ASMNAME) && DECL_ASSEMBLER_NAME_SET_P(node))
            pp_tree_identifier(pp, DECL_ASSEMBLER_NAME(node));
        /* For DECL_NAMELESS names look for embedded uids in the
	 names and sanitize them for TDF_NOUID.  */
        else if ((flags & TDF_NOUID) && DECL_NAMELESS(node))
            dump_fancy_name(pp, DECL_NAME(node));
        else
            pp_tree_identifier(pp, DECL_NAME(node));
    }
    char uid_sep = (flags & TDF_GIMPLE) ? '_' : '.';
    if ((flags & TDF_UID) || DECL_NAME(node) == NULL_TREE)
    {
        if (TREE_CODE(node) == LABEL_DECL && LABEL_DECL_UID(node) != -1)
        {
        }
        else if (TREE_CODE(node) == DEBUG_EXPR_DECL)
        {
            if (flags & TDF_NOUID)
            {
            }
            else
            {
                // DEBUG_TEMP_UID(node)
            }
        }
        else
        {
            char c = TREE_CODE(node) == CONST_DECL ? 'C' : 'D';
            if (flags & TDF_NOUID)
            {
            }
            else
            {
                // DECL_UID(node)
            }
        }
    }
    if ((flags & TDF_ALIAS) && DECL_PT_UID(node) != DECL_UID(node))
    {
        if (flags & TDF_NOUID)
        {
        }
        else
        {
            // DECL_PT_UID(node)
        }
    }
}

void dump_function_name(void *pp, tree node, int flags)
{
    if (CONVERT_EXPR_P(node))
        node = TREE_OPERAND(node, 0);
    if (DECL_NAME(node) && (flags & TDF_ASMNAME) == 0)
    {
        //pp_string (pp, lang_hooks.decl_printable_name (node, 1));
    }
    else
    {
        dump_decl_name(pp, node, flags);
    }
}

void dump_function_declaration(void *pp, tree node, int spc, int flags)
{
    bool wrote_arg = false;
    tree arg;

    //pp_space (pp);
    //pp_left_paren (pp);

    /* Print the argument types.  */
    arg = TYPE_ARG_TYPES(node);
    while (arg && arg != void_list_node && arg != error_mark_node)
    {
        if (wrote_arg)
        {
            //pp_comma(pp);
            //pp_space(pp);
        }
        wrote_arg = true;
        dump_generic_node(pp, TREE_VALUE(arg), spc, flags, false);
        arg = TREE_CHAIN(arg);
    }

    /* Drop the trailing void_type_node if we had any previous argument.  */
    if (arg == void_list_node && !wrote_arg)
    {
        //pp_string(pp, "void");
    }
    /* Properly dump vararg function types.  */
    else if (!arg && wrote_arg)
    {
        //pp_string(pp, ", ...");
    }
    /* Avoid printing any arg for unprototyped functions.  */

    //pp_right_paren(pp);
}

void dump_array_domain(void *pp, tree domain, int spc, int flags)
{
    if (domain)
    {
        tree min = TYPE_MIN_VALUE(domain);
        tree max = TYPE_MAX_VALUE(domain);

        if (min && max && integer_zerop(min) && tree_fits_shwi_p(max))
        {
            //pp_wide_integer(pp, tree_to_shwi(max) + 1);
        }
        else
        {
            if (min)
                dump_generic_node(pp, min, spc, flags, false);
            //pp_colon(pp);
            if (max)
                dump_generic_node(pp, max, spc, flags, false);
        }
    }
    else
    {
        //pp_string(pp, "<unknown>");
    }
}

void dump_omp_clauses(void *, tree, int, int)
{
    fprintf(stderr, "dump_omp_clauses\n");
}

void dump_omp_clause(void *pp, tree clause, int spc, int flags)
{
    fprintf(stderr, "dump_omp_clause\n");
}

void dump_block_node(void *pp, tree block, int spc, int flags)
{
    tree t;

    //pp_printf(pp, "BLOCK #%d ", BLOCK_NUMBER(block));

    if (flags & TDF_ADDRESS)
    {
        //pp_printf(pp, "[%p] ", (void *)block);
    }

    if (BLOCK_ABSTRACT(block))
    {
        //pp_string(pp, "[abstract] ");
    }

    if (TREE_ASM_WRITTEN(block))
    {
        //pp_string(pp, "[written] ");
    }

    if (flags & TDF_SLIM)
        return;

    if (BLOCK_SOURCE_LOCATION(block))
    {
        //dump_location(pp, BLOCK_SOURCE_LOCATION(block));
    }

    if (BLOCK_SUPERCONTEXT(block))
    {
        //pp_string(pp, "SUPERCONTEXT: ");
        dump_generic_node(pp, BLOCK_SUPERCONTEXT(block), 0, flags | TDF_SLIM, false);
    }

    if (BLOCK_SUBBLOCKS(block))
    {
        //pp_string(pp, "SUBBLOCKS: ");
        for (t = BLOCK_SUBBLOCKS(block); t; t = BLOCK_CHAIN(t))
        {
            dump_generic_node(pp, t, 0, flags | TDF_SLIM, false);
            //pp_space(pp);
        }
    }

    if (BLOCK_CHAIN(block))
    {
        //pp_string(pp, "SIBLINGS: ");
        for (t = BLOCK_CHAIN(block); t; t = BLOCK_CHAIN(t))
        {
            dump_generic_node(pp, t, 0, flags | TDF_SLIM, false);
            //pp_space(pp);
        }
    }

    if (BLOCK_VARS(block))
    {
        //pp_string(pp, "VARS: ");
        for (t = BLOCK_VARS(block); t; t = TREE_CHAIN(t))
        {
            dump_generic_node(pp, t, 0, flags, false);
            //pp_space(pp);
        }
    }

    if (vec_safe_length(BLOCK_NONLOCALIZED_VARS(block)) > 0)
    {
        unsigned i;
        vec<tree, va_gc> *nlv = BLOCK_NONLOCALIZED_VARS(block);

        //pp_string(pp, "NONLOCALIZED_VARS: ");
        FOR_EACH_VEC_ELT(*nlv, i, t)
        {
            dump_generic_node(pp, t, 0, flags, false);
            //pp_space(pp);
        }
    }

    if (BLOCK_ABSTRACT_ORIGIN(block))
    {
        //pp_string(pp, "ABSTRACT_ORIGIN: ");
        dump_generic_node(pp, BLOCK_ABSTRACT_ORIGIN(block), 0, flags | TDF_SLIM, false);
    }

    if (BLOCK_FRAGMENT_ORIGIN(block))
    {
        //pp_string(pp, "FRAGMENT_ORIGIN: ");
        dump_generic_node(pp, BLOCK_FRAGMENT_ORIGIN(block), 0, flags | TDF_SLIM, false);
    }

    if (BLOCK_FRAGMENT_CHAIN(block))
    {
        //pp_string(pp, "FRAGMENT_CHAIN: ");
        for (t = BLOCK_FRAGMENT_CHAIN(block); t; t = BLOCK_FRAGMENT_CHAIN(t))
        {
            dump_generic_node(pp, t, 0, flags | TDF_SLIM, false);
            //pp_space(pp);
        }
    }
}

void print_declaration(void *pp, tree t, int spc, int flags)
{
    //INDENT(spc);

    if (TREE_CODE(t) == NAMELIST_DECL)
    {
        //pp_string(pp, "namelist ");
        dump_decl_name(pp, t, flags);
        return;
    }

    if (TREE_CODE(t) == TYPE_DECL)
    {
        //pp_string(pp, "typedef ");
    }

    if (CODE_CONTAINS_STRUCT(TREE_CODE(t), TS_DECL_WRTL) && DECL_REGISTER(t))
    {
        //pp_string(pp, "register ");
    }

    if (TREE_PUBLIC(t) && DECL_EXTERNAL(t))
    {
        //pp_string(pp, "extern ");
    }
    else if (TREE_STATIC(t))
    {
        //pp_string(pp, "static ");
    }

    /* Print the type and name.  */
    if (TREE_TYPE(t) && TREE_CODE(TREE_TYPE(t)) == ARRAY_TYPE)
    {
        tree tmp;

        /* Print array's type.  */
        tmp = TREE_TYPE(t);
        while (TREE_CODE(TREE_TYPE(tmp)) == ARRAY_TYPE)
            tmp = TREE_TYPE(tmp);
        dump_generic_node(pp, TREE_TYPE(tmp), spc, flags, false);

        /* Print variable's name.  */
        dump_generic_node(pp, t, spc, flags, false);

        /* Print the dimensions.  */
        tmp = TREE_TYPE(t);
        while (TREE_CODE(tmp) == ARRAY_TYPE)
        {
            dump_array_domain(pp, TYPE_DOMAIN(tmp), spc, flags);
            tmp = TREE_TYPE(tmp);
        }
    }
    else if (TREE_CODE(t) == FUNCTION_DECL)
    {
        dump_generic_node(pp, TREE_TYPE(TREE_TYPE(t)), spc, flags, false);
        dump_decl_name(pp, t, flags);
        dump_function_declaration(pp, TREE_TYPE(t), spc, flags);
    }
    else
    {
        /* Print type declaration.  */
        dump_generic_node(pp, TREE_TYPE(t), spc, flags, false);

        /* Print variable's name.  */
        dump_generic_node(pp, t, spc, flags, false);
    }

    if (VAR_P(t) && DECL_HARD_REGISTER(t))
    {
        //pp_string(pp, " __asm__ ");
        dump_generic_node(pp, DECL_ASSEMBLER_NAME(t), spc, flags, false);
    }

    /* The initial value of a function serves to determine whether the function
     is declared or defined.  So the following does not apply to function
     nodes.  */
    if (TREE_CODE(t) != FUNCTION_DECL)
    {
        /* Print the initial value.  */
        if (DECL_INITIAL(t))
        {
            dump_generic_node(pp, DECL_INITIAL(t), spc, flags, false);
        }
    }

    if (VAR_P(t) && DECL_HAS_VALUE_EXPR_P(t))
    {
        //pp_string(pp, " [value-expr: ");
        dump_generic_node(pp, DECL_VALUE_EXPR(t), spc, flags, false);
    }
}

void print_struct_decl(void *pp, const_tree node, int spc, int flags)
{
    /* Print the name of the structure.  */
    if (TYPE_NAME(node))
    {
        //INDENT(spc);
        if (TREE_CODE(node) == RECORD_TYPE)
        {
            //pp_string(pp, "struct ");
        }
        else if ((TREE_CODE(node) == UNION_TYPE || TREE_CODE(node) == QUAL_UNION_TYPE))
        {
            //pp_string(pp, "union ");
        }

        dump_generic_node(pp, TYPE_NAME(node), spc, 0, false);
    }

    /* Print the contents of the structure.  */
    //pp_newline(pp);
    //INDENT(spc);
    //pp_left_brace(pp);
    //pp_newline(pp);

    /* Print the fields of the structure.  */
    {
        tree tmp;
        tmp = TYPE_FIELDS(node);
        while (tmp)
        {
            /* Avoid to print recursively the structure.  */
            /* FIXME : Not implemented correctly...,
	   what about the case when we have a cycle in the contain graph? ...
	   Maybe this could be solved by looking at the scope in which the
	   structure was declared.  */
            if (TREE_TYPE(tmp) != node && (TREE_CODE(TREE_TYPE(tmp)) != POINTER_TYPE || TREE_TYPE(TREE_TYPE(tmp)) != node))
            {
                print_declaration(pp, tmp, spc + 2, flags);
                //pp_newline(pp);
            }
            tmp = DECL_CHAIN(tmp);
        }
    }
    //INDENT(spc);
    //pp_right_brace(pp);
}

void print_call_name(void *pp, tree node, int flags)
{
    tree op0 = node;

    if (TREE_CODE(op0) == NON_LVALUE_EXPR)
        op0 = TREE_OPERAND(op0, 0);

again:
    switch (TREE_CODE(op0))
    {
    case VAR_DECL:
    case PARM_DECL:
    case FUNCTION_DECL:
        dump_function_name(pp, op0, flags);
        break;

    case ADDR_EXPR:
    case INDIRECT_REF:
    CASE_CONVERT:
        op0 = TREE_OPERAND(op0, 0);
        goto again;

    case COND_EXPR:
        //pp_left_paren(pp);
        dump_generic_node(pp, TREE_OPERAND(op0, 0), 0, flags, false);
        //pp_string(pp, ") ? ");
        dump_generic_node(pp, TREE_OPERAND(op0, 1), 0, flags, false);
        //pp_string(pp, " : ");
        dump_generic_node(pp, TREE_OPERAND(op0, 2), 0, flags, false);
        break;

    case ARRAY_REF:
        if (TREE_CODE(TREE_OPERAND(op0, 0)) == VAR_DECL)
            dump_function_name(pp, TREE_OPERAND(op0, 0), flags);
        else
            dump_generic_node(pp, op0, 0, flags, false);
        break;

    case MEM_REF:
        if (integer_zerop(TREE_OPERAND(op0, 1)))
        {
            op0 = TREE_OPERAND(op0, 0);
            goto again;
        }
        /* Fallthru.  */
    case COMPONENT_REF:
    case SSA_NAME:
    case OBJ_TYPE_REF:
        dump_generic_node(pp, op0, 0, flags, false);
        break;

    default:
        NIY;
    }
}

int dump_generic_node(void *pp, tree node, int spc, int flags, bool is_stmt)
{
    tree type;
    tree op0, op1;
    const char *str;
    bool is_expr;

    if (node == NULL_TREE)
        return spc;

    if (DECL_P(node) && DECL_IS_BUILTIN(node))
        return spc;

    enum tree_code code = TREE_CODE(node);;
    enum tree_code_class tclass = TREE_CODE_CLASS(TREE_CODE(node));

    is_expr = EXPR_P(node);

    if (!is_expr || EXPR_HAS_LOCATION(node)) {
        for(int i = 0; i <spc; i++)
            fprintf(stderr, " ");
        fprintf(stderr, "<%s>\n", get_tree_code_name(code));
    }
    
    //if (is_stmt && (flags & TDF_STMTADDR))
    //{
    //}

    //if ((flags & TDF_LINENO) && EXPR_HAS_LOCATION(node))
    //    dump_location(pp, EXPR_LOCATION(node));

    switch (code)
    {
    case ERROR_MARK:
        //pp_string(pp, "<<< error >>>");
        break;

    case IDENTIFIER_NODE:
        pp_tree_identifier(pp, node);
        break;

    case TREE_LIST:
        while (node && node != error_mark_node)
        {
            if (TREE_PURPOSE(node))
            {
                dump_generic_node(pp, TREE_PURPOSE(node), spc, flags, false);
            }
            dump_generic_node(pp, TREE_VALUE(node), spc, flags, false);
            node = TREE_CHAIN(node);
            if (node && TREE_CODE(node) == TREE_LIST)
            {
                //pp_comma(pp);
                //pp_space(pp);
            }
        }
        break;

    case TREE_BINFO:
        dump_generic_node(pp, BINFO_TYPE(node), spc, flags, false);
        break;

    case TREE_VEC:
    {
        size_t i;
        if (TREE_VEC_LENGTH(node) > 0)
        {
            size_t len = TREE_VEC_LENGTH(node);
            for (i = 0; i < len - 1; i++)
            {
                dump_generic_node(pp, TREE_VEC_ELT(node, i), spc, flags, false);
            }
            dump_generic_node(pp, TREE_VEC_ELT(node, len - 1), spc, flags, false);
        }
    }
    break;

    case VOID_TYPE:
    case POINTER_BOUNDS_TYPE:
    case INTEGER_TYPE:
    case REAL_TYPE:
    case FIXED_POINT_TYPE:
    case COMPLEX_TYPE:
    case VECTOR_TYPE:
    case ENUMERAL_TYPE:
    case BOOLEAN_TYPE:
    {
        unsigned int quals = TYPE_QUALS(node);

        if (quals & TYPE_QUAL_ATOMIC)
        {
            //pp_string(pp, "atomic ");
        }
        if (quals & TYPE_QUAL_CONST)
        {
            //pp_string(pp, "const ");
        }
        else if (quals & TYPE_QUAL_VOLATILE)
        {
            //pp_string(pp, "volatile ");
        }
        else if (quals & TYPE_QUAL_RESTRICT)
        {
            //pp_string(pp, "restrict ");
        }

        if (!ADDR_SPACE_GENERIC_P(TYPE_ADDR_SPACE(node)))
        {
            //pp_string(pp, "<address-space-");
            //pp_decimal_int(pp, TYPE_ADDR_SPACE(node));
            //pp_string(pp, "> ");
        }

        if (tclass == tcc_declaration)
        {
            if (DECL_NAME(node))
            {
                dump_decl_name(pp, node, flags);
            }
            else
            {
                //pp_string(pp, "<unnamed type decl>");
            }
        }
        else if (tclass == tcc_type)
        {
            if (TYPE_NAME(node))
            {
                if (TREE_CODE(TYPE_NAME(node)) == IDENTIFIER_NODE)
                {
                    pp_tree_identifier(pp, TYPE_NAME(node));
                }
                else if (TREE_CODE(TYPE_NAME(node)) == TYPE_DECL && DECL_NAME(TYPE_NAME(node)))
                {
                    dump_decl_name(pp, TYPE_NAME(node), flags);
                }
                else
                {
                    //pp_string(pp, "<unnamed type>");
                }
            }
            else if (TREE_CODE(node) == VECTOR_TYPE)
            {
                //pp_string(pp, "vector");
                //pp_wide_integer(pp, TYPE_VECTOR_SUBPARTS(node));
                dump_generic_node(pp, TREE_TYPE(node), spc, flags, false);
            }
            else if (TREE_CODE(node) == INTEGER_TYPE)
            {
                if (TYPE_PRECISION(node) == CHAR_TYPE_SIZE)
                {
                    //pp_string(pp, (TYPE_UNSIGNED(node) ? "unsigned char" : "signed char"));
                }
                else if (TYPE_PRECISION(node) == SHORT_TYPE_SIZE)
                {
                    //pp_string(pp, (TYPE_UNSIGNED(node) ? "unsigned short" : "signed short"));
                }
                else if (TYPE_PRECISION(node) == INT_TYPE_SIZE)
                {
                    //pp_string(pp, (TYPE_UNSIGNED(node) ? "unsigned int" : "signed int"));
                }
                else if (TYPE_PRECISION(node) == LONG_TYPE_SIZE)
                {
                    //pp_string(pp, (TYPE_UNSIGNED(node) ? "unsigned long" : "signed long"));
                }
                else if (TYPE_PRECISION(node) == LONG_LONG_TYPE_SIZE)
                {
                    // pp_string(pp, (TYPE_UNSIGNED(node) ? "unsigned long long" : "signed long long"));
                }
                else if (TYPE_PRECISION(node) >= CHAR_TYPE_SIZE && pow2p_hwi(TYPE_PRECISION(node)))
                {
                    //pp_string(pp, (TYPE_UNSIGNED(node) ? "uint" : "int"));
                    //pp_decimal_int(pp, TYPE_PRECISION(node));
                    //pp_string(pp, "_t");
                }
                else
                {
                    //pp_string(pp, (TYPE_UNSIGNED(node) ? "<unnamed-unsigned:" : "<unnamed-signed:"));
                    //pp_decimal_int(pp, TYPE_PRECISION(node));
                    //pp_greater(pp);
                }
            }
            else if (TREE_CODE(node) == COMPLEX_TYPE)
            {
                //pp_string(pp, "__complex__ ");
                dump_generic_node(pp, TREE_TYPE(node), spc, flags, false);
            }
            else if (TREE_CODE(node) == REAL_TYPE)
            {
                //pp_string(pp, "<float:");
                //pp_decimal_int(pp, TYPE_PRECISION(node));
                //pp_greater(pp);
            }
            else if (TREE_CODE(node) == FIXED_POINT_TYPE)
            {
                //pp_string(pp, "<fixed-point-");
                //pp_string(pp, TYPE_SATURATING(node) ? "sat:" : "nonsat:");
                //pp_decimal_int(pp, TYPE_PRECISION(node));
                //pp_greater(pp);
            }
            else if (TREE_CODE(node) == VOID_TYPE)
            {
                //pp_string(pp, "void");
            }
            else
            {
                //pp_string(pp, "<unnamed type>");
            }
        }
        break;
    }

    case POINTER_TYPE:
    case REFERENCE_TYPE:
        str = (TREE_CODE(node) == POINTER_TYPE ? "*" : "&");

        if (TREE_TYPE(node) == NULL)
        {
            //pp_string(pp, str);
            //pp_string(pp, "<null type>");
        }
        else if (TREE_CODE(TREE_TYPE(node)) == FUNCTION_TYPE)
        {
            tree fnode = TREE_TYPE(node);

            dump_generic_node(pp, TREE_TYPE(fnode), spc, flags, false);
            //pp_string(pp, str);
            if (TYPE_NAME(node) && DECL_NAME(TYPE_NAME(node)))
            {
                dump_decl_name(pp, TYPE_NAME(node), flags);
            }
            else if (flags & TDF_NOUID)
            {
            }
            else
            {
                // TYPE_UID(node)
            }
            dump_function_declaration(pp, fnode, spc, flags);
        }
        else
        {
            unsigned int quals = TYPE_QUALS(node);

            dump_generic_node(pp, TREE_TYPE(node), spc, flags, false);
            //pp_string(pp, str);

            if (quals & TYPE_QUAL_CONST)
            {
                //pp_string(pp, " const");
            }
            if (quals & TYPE_QUAL_VOLATILE)
            {
                //pp_string(pp, " volatile");
            }
            if (quals & TYPE_QUAL_RESTRICT)
            {
                //pp_string(pp, " restrict");
            }

            if (!ADDR_SPACE_GENERIC_P(TYPE_ADDR_SPACE(node)))
            {
                //pp_string(pp, " <address-space-");
                //pp_decimal_int(pp, TYPE_ADDR_SPACE(node));
                //pp_greater(pp);
            }

            if (TYPE_REF_CAN_ALIAS_ALL(node))
            {
                //pp_string(pp, " {ref-all}");
            }
        }
        break;

    case OFFSET_TYPE:
        NIY;
        break;

    case MEM_REF:
    {
        if (flags & TDF_GIMPLE)
        {
            //pp_string(pp, "__MEM <");
            dump_generic_node(pp, TREE_TYPE(node), spc, flags | TDF_SLIM, false);
            if (TYPE_ALIGN(TREE_TYPE(node)) != TYPE_ALIGN(TYPE_MAIN_VARIANT(TREE_TYPE(node))))
            {
                //pp_string(pp, ", ");
                //pp_decimal_int(pp, TYPE_ALIGN(TREE_TYPE(node)));
            }
            //pp_greater(pp);
            if (TREE_TYPE(TREE_OPERAND(node, 0)) != TREE_TYPE(TREE_OPERAND(node, 1)))
            {
                //pp_left_paren(pp);
                dump_generic_node(pp, TREE_TYPE(TREE_OPERAND(node, 1)), spc, flags | TDF_SLIM, false);
                //pp_right_paren(pp);
            }
            dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags | TDF_SLIM, false);
            if (!integer_zerop(TREE_OPERAND(node, 1)))
            {
                //pp_string(pp, " + ");
                dump_generic_node(pp, TREE_OPERAND(node, 1), spc, flags | TDF_SLIM, false);
            }
        }
        else if (integer_zerop(TREE_OPERAND(node, 1))
                 /* Dump the types of INTEGER_CSTs explicitly, for we can't
	       infer them and MEM_ATTR caching will share MEM_REFs
	       with differently-typed op0s.  */
                 && TREE_CODE(TREE_OPERAND(node, 0)) != INTEGER_CST
                 /* Released SSA_NAMES have no TREE_TYPE.  */
                 && TREE_TYPE(TREE_OPERAND(node, 0)) != NULL_TREE
                 /* Same pointer types, but ignoring POINTER_TYPE vs.
	       REFERENCE_TYPE.  */
                 && (TREE_TYPE(TREE_TYPE(TREE_OPERAND(node, 0))) == TREE_TYPE(TREE_TYPE(TREE_OPERAND(node, 1))))
                 //&& (TYPE_MODE(TREE_TYPE(TREE_OPERAND(node, 0))) == TYPE_MODE(TREE_TYPE(TREE_OPERAND(node, 1))))
                 && (TYPE_REF_CAN_ALIAS_ALL(TREE_TYPE(TREE_OPERAND(node, 0))) == TYPE_REF_CAN_ALIAS_ALL(TREE_TYPE(TREE_OPERAND(node, 1))))
                 /* Same value types ignoring qualifiers.  */
                 && (TYPE_MAIN_VARIANT(TREE_TYPE(node)) == TYPE_MAIN_VARIANT(TREE_TYPE(TREE_TYPE(TREE_OPERAND(node, 1))))) && (!(flags & TDF_ALIAS) || MR_DEPENDENCE_CLIQUE(node) == 0))
        {
            if (TREE_CODE(TREE_OPERAND(node, 0)) != ADDR_EXPR)
            {
                dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
            }
            else
            {
                dump_generic_node(pp, TREE_OPERAND(TREE_OPERAND(node, 0), 0), spc, flags, false);
            }
        }
        else
        {
            tree ptype;

            //pp_string(pp, "MEM[");
            ptype = TYPE_MAIN_VARIANT(TREE_TYPE(TREE_OPERAND(node, 1)));
            dump_generic_node(pp, ptype, spc, flags | TDF_SLIM, false);
            dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
            if (!integer_zerop(TREE_OPERAND(node, 1)))
            {
                //pp_string(pp, " + ");
                dump_generic_node(pp, TREE_OPERAND(node, 1), spc, flags, false);
            }
            if ((flags & TDF_ALIAS) && MR_DEPENDENCE_CLIQUE(node) != 0)
            {
                //pp_string(pp, " clique ");
                //pp_unsigned_wide_integer(pp, MR_DEPENDENCE_CLIQUE(node));
                //pp_string(pp, " base ");
                //pp_unsigned_wide_integer(pp, MR_DEPENDENCE_BASE(node));
            }
            //pp_right_bracket(pp);
        }
        break;
    }

    case TARGET_MEM_REF:
    {
        tree tmp;

        if (TREE_CODE(TMR_BASE(node)) == ADDR_EXPR)
        {
            dump_generic_node(pp, TREE_OPERAND(TMR_BASE(node), 0), spc, flags, false);
        }
        else
        {
            dump_generic_node(pp, TMR_BASE(node), spc, flags, false);
        }
        tmp = TMR_INDEX2(node);
        if (tmp)
        {
            dump_generic_node(pp, tmp, spc, flags, false);
        }
        tmp = TMR_INDEX(node);
        if (tmp)
        {
            dump_generic_node(pp, tmp, spc, flags, false);
        }
        tmp = TMR_STEP(node);
        if (tmp)
        {
            dump_generic_node(pp, tmp, spc, flags, false);
        }
        tmp = TMR_OFFSET(node);
        if (tmp)
        {
            dump_generic_node(pp, tmp, spc, flags, false);
        }
    }
    break;

    case ARRAY_TYPE:
    {
        tree tmp;

        /* Print the innermost component type.  */
        for (tmp = TREE_TYPE(node); TREE_CODE(tmp) == ARRAY_TYPE;
             tmp = TREE_TYPE(tmp))
            ;
        dump_generic_node(pp, tmp, spc, flags, false);

        /* Print the dimensions.  */
        for (tmp = node; TREE_CODE(tmp) == ARRAY_TYPE; tmp = TREE_TYPE(tmp))
        {
            dump_array_domain(pp, TYPE_DOMAIN(tmp), spc, flags);
        }
        break;
    }

    case RECORD_TYPE:
    case UNION_TYPE:
    case QUAL_UNION_TYPE:
    {
        unsigned int quals = TYPE_QUALS(node);

        if (quals & TYPE_QUAL_ATOMIC)
        {
            //pp_string(pp, "atomic ");
        }
        if (quals & TYPE_QUAL_CONST)
        {
            //pp_string(pp, "const ");
        }
        if (quals & TYPE_QUAL_VOLATILE)
        {
            //pp_string(pp, "volatile ");
        }

        /* Print the name of the structure.  */
        if (TREE_CODE(node) == RECORD_TYPE)
        {
            //pp_string(pp, "struct ");
        }
        else if (TREE_CODE(node) == UNION_TYPE)
        {
            //pp_string(pp, "union ");
        }

        if (TYPE_NAME(node))
        {
            dump_generic_node(pp, TYPE_NAME(node), spc, flags, false);
        }
        else if (!(flags & TDF_SLIM))
        {
            /* FIXME: If we eliminate the 'else' above and attempt
	     to show the fields for named types, we may get stuck
	     following a cycle of pointers to structs.  The alleged
	     self-reference check in print_struct_decl will not detect
	     cycles involving more than one pointer or struct type.  */
            print_struct_decl(pp, node, spc, flags);
        }
        break;
    }

    case LANG_TYPE:
        NIY;
        break;

    case INTEGER_CST:
        if (flags & TDF_GIMPLE && (POINTER_TYPE_P(TREE_TYPE(node)) || (TYPE_PRECISION(TREE_TYPE(node)) < TYPE_PRECISION(integer_type_node)) || exact_log2(TYPE_PRECISION(TREE_TYPE(node))) == -1))
        {
            //pp_string(pp, "_Literal (");
            dump_generic_node(pp, TREE_TYPE(node), spc, flags, false);
            //pp_string(pp, ") ");
        }
        if (TREE_CODE(TREE_TYPE(node)) == POINTER_TYPE && !(flags & TDF_GIMPLE))
        {
            /* In the case of a pointer, one may want to divide by the
	     size of the pointed-to type.  Unfortunately, this not
	     straightforward.  The C front-end maps expressions

	     (int *) 5
	     int *p; (p + 5)

	     in such a way that the two INTEGER_CST nodes for "5" have
	     different values but identical types.  In the latter
	     case, the 5 is multiplied by sizeof (int) in c-common.c
	     (pointer_int_sum) to convert it to a byte address, and
	     yet the type of the node is left unchanged.  Argh.  What
	     is consistent though is that the number value corresponds
	     to bytes (UNITS) offset.

             NB: Neither of the following divisors can be trivially
             used to recover the original literal:

             TREE_INT_CST_LOW (TYPE_SIZE_UNIT (TREE_TYPE (node)))
	     TYPE_PRECISION (TREE_TYPE (TREE_TYPE (node)))  */
            //pp_wide_integer(pp, TREE_INT_CST_LOW(node));
            //pp_string(pp, "B"); /* pseudo-unit */
        }
        else if (tree_fits_shwi_p(node))
        {
            //pp_wide_integer(pp, tree_to_shwi(node));
        }
        else if (tree_fits_uhwi_p(node))
        {
            //pp_unsigned_wide_integer(pp, tree_to_uhwi(node));
        }
        else
        {
            wide_int val = node;

            if (wi::neg_p(val, TYPE_SIGN(TREE_TYPE(node))))
            {
                //pp_minus(pp);
                val = -val;
            }
            //print_hex(val, pp_buffer(pp)->digit_buffer);
            //pp_string(pp, pp_buffer(pp)->digit_buffer);
        }
        if ((flags & TDF_GIMPLE) && !(POINTER_TYPE_P(TREE_TYPE(node)) || (TYPE_PRECISION(TREE_TYPE(node)) < TYPE_PRECISION(integer_type_node)) || exact_log2(TYPE_PRECISION(TREE_TYPE(node))) == -1))
        {
            if (TYPE_UNSIGNED(TREE_TYPE(node)))
            {
                //pp_character(pp, 'u');
            }
            if (TYPE_PRECISION(TREE_TYPE(node)) == TYPE_PRECISION(unsigned_type_node))
            {
                ;
            }
            else if (TYPE_PRECISION(TREE_TYPE(node)) == TYPE_PRECISION(long_unsigned_type_node))
            {
                //pp_character(pp, 'l');
            }
            else if (TYPE_PRECISION(TREE_TYPE(node)) == TYPE_PRECISION(long_long_unsigned_type_node))
            {
                //pp_string(pp, "ll");
            }
        }
        if (TREE_OVERFLOW(node))
        {
            //pp_string(pp, "(OVF)");
        }
        break;

    case REAL_CST:
        /* Code copied from print_node.  */
        {
            REAL_VALUE_TYPE d;
            if (TREE_OVERFLOW(node))
            {
                //pp_string(pp, " overflow");
            }

            d = TREE_REAL_CST(node);
            if (REAL_VALUE_ISINF(d))
            {
                //pp_string(pp, REAL_VALUE_NEGATIVE(d) ? " -Inf" : " Inf");
            }
            else if (REAL_VALUE_ISNAN(d))
            {
                //pp_string(pp, " Nan");
            }
            else
            {
                //char string[100];
                //real_to_decimal(string, &d, sizeof(string), 0, 1);
                //pp_string(pp, string);
            }
            break;
        }

    case FIXED_CST:
    {
        //char string[100];
        //fixed_to_decimal(string, TREE_FIXED_CST_PTR(node), sizeof(string));
        //pp_string(pp, string);
        break;
    }

    case COMPLEX_CST:
        dump_generic_node(pp, TREE_REALPART(node), spc, flags, false);
        dump_generic_node(pp, TREE_IMAGPART(node), spc, flags, false);
        break;

    case STRING_CST:
        //pretty_print_string(pp, TREE_STRING_POINTER(node));
        break;

    case VECTOR_CST:
    {
        unsigned i;
        for (i = 0; i < VECTOR_CST_NELTS(node); ++i)
        {
            dump_generic_node(pp, VECTOR_CST_ELT(node, i), spc, flags, false);
        }
    }
    break;

    case FUNCTION_TYPE:
    case METHOD_TYPE:
        dump_generic_node(pp, TREE_TYPE(node), spc, flags, false);
        if (TREE_CODE(node) == METHOD_TYPE)
        {
            if (TYPE_METHOD_BASETYPE(node))
            {
                dump_decl_name(pp, TYPE_NAME(TYPE_METHOD_BASETYPE(node)), flags);
            }
            else
            {
                //pp_string(pp, "<null method basetype>");
            }
        }
        if (TYPE_NAME(node) && DECL_NAME(TYPE_NAME(node)))
            dump_decl_name(pp, TYPE_NAME(node), flags);
        else if (flags & TDF_NOUID)
        {
        }
        else
        {
            // TYPE_UID(node)
        }
        dump_function_declaration(pp, node, spc, flags);
        break;

    case FUNCTION_DECL:
        dump_decl_name(pp, node, flags);
        // dump function body
        dump_generic_node(pp, DECL_SAVED_TREE(node), spc, flags, false);
        break;
    case CONST_DECL:
        dump_decl_name(pp, node, flags);
        break;

    case LABEL_DECL:
        if (DECL_NAME(node))
        {
            dump_decl_name(pp, node, flags);
        }
        else if (LABEL_DECL_UID(node) != -1)
        {
            if (flags & TDF_GIMPLE)
            {
                // (int)LABEL_DECL_UID(node)
            }
            else
            {
            }
        }
        else
        {
            if (flags & TDF_NOUID)
            {
                //pp_string(pp, "<D.xxxx>");
            }
            else
            {
                if (flags & TDF_GIMPLE)
                {
                    // DECL_UID(node)
                }
                else
                {
                }
            }
        }
        break;

    case TYPE_DECL:
        if (DECL_IS_BUILTIN(node))
        {
            /* Don't print the declaration of built-in types.  */
            break;
        }
        if (DECL_NAME(node))
        {
            dump_decl_name(pp, node, flags);
        }
        else if (TYPE_NAME(TREE_TYPE(node)) != node)
        {
            if ((TREE_CODE(TREE_TYPE(node)) == RECORD_TYPE || TREE_CODE(TREE_TYPE(node)) == UNION_TYPE) && TYPE_METHODS(TREE_TYPE(node)))
            {
                /* The type is a c++ class: all structures have at least 4 methods.  */
                //pp_string(pp, "class ");
                dump_generic_node(pp, TREE_TYPE(node), spc, flags, false);
            }
            else
            {
                //pp_string(pp, (TREE_CODE(TREE_TYPE(node)) == UNION_TYPE ? "union" : "struct "));
                dump_generic_node(pp, TREE_TYPE(node), spc, flags, false);
            }
        }
        else
        {
            //pp_string(pp, "<anon>");
        }
        break;

    case VAR_DECL:
    case PARM_DECL:
    case FIELD_DECL:
    case DEBUG_EXPR_DECL:
    case NAMESPACE_DECL:
    case NAMELIST_DECL:
        dump_decl_name(pp, node, flags);
        break;

    case RESULT_DECL:
    {
        //pp_string(pp, "<retval>");
    }
    break;

    case COMPONENT_REF:
        op0 = TREE_OPERAND(node, 0);
        str = ".";
        if (op0 && (TREE_CODE(op0) == INDIRECT_REF || (TREE_CODE(op0) == MEM_REF && TREE_CODE(TREE_OPERAND(op0, 0)) != ADDR_EXPR && integer_zerop(TREE_OPERAND(op0, 1))
                                                       /* Dump the types of INTEGER_CSTs explicitly, for we
		     can't infer them and MEM_ATTR caching will share
		     MEM_REFs with differently-typed op0s.  */
                                                       && TREE_CODE(TREE_OPERAND(op0, 0)) != INTEGER_CST
                                                       /* Released SSA_NAMES have no TREE_TYPE.  */
                                                       && TREE_TYPE(TREE_OPERAND(op0, 0)) != NULL_TREE
                                                       /* Same pointer types, but ignoring POINTER_TYPE vs.
		     REFERENCE_TYPE.  */
                                                       && (TREE_TYPE(TREE_TYPE(TREE_OPERAND(op0, 0))) == TREE_TYPE(TREE_TYPE(TREE_OPERAND(op0, 1))))
                                                       //&& (TYPE_MODE(TREE_TYPE(TREE_OPERAND(op0, 0))) == TYPE_MODE(TREE_TYPE(TREE_OPERAND(op0, 1))))
                                                       && (TYPE_REF_CAN_ALIAS_ALL(TREE_TYPE(TREE_OPERAND(op0, 0))) == TYPE_REF_CAN_ALIAS_ALL(TREE_TYPE(TREE_OPERAND(op0, 1))))
                                                       /* Same value types ignoring qualifiers.  */
                                                       && (TYPE_MAIN_VARIANT(TREE_TYPE(op0)) == TYPE_MAIN_VARIANT(TREE_TYPE(TREE_TYPE(TREE_OPERAND(op0, 1))))) && MR_DEPENDENCE_CLIQUE(op0) == 0)))
        {
            op0 = TREE_OPERAND(op0, 0);
            str = "->";
        }
        if (op_prio(op0) < op_prio(node))
        {
            //pp_left_paren(pp);
        }
        dump_generic_node(pp, op0, spc, flags, false);
        if (op_prio(op0) < op_prio(node))
        {
            //pp_right_paren(pp);
        }
        //pp_string(pp, str);
        dump_generic_node(pp, TREE_OPERAND(node, 1), spc, flags, false);
        op0 = component_ref_field_offset(node);
        if (op0 && TREE_CODE(op0) != INTEGER_CST)
        {
            //pp_string(pp, "{off: ");
            dump_generic_node(pp, op0, spc, flags, false);
            //pp_right_brace(pp);
        }
        break;

    case BIT_FIELD_REF:
        //pp_string(pp, "BIT_FIELD_REF <");
        dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
        //pp_string(pp, ", ");
        dump_generic_node(pp, TREE_OPERAND(node, 1), spc, flags, false);
        //pp_string(pp, ", ");
        dump_generic_node(pp, TREE_OPERAND(node, 2), spc, flags, false);
        //pp_greater(pp);
        break;

    case BIT_INSERT_EXPR:
        //pp_string(pp, "BIT_INSERT_EXPR <");
        dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
        //pp_string(pp, ", ");
        dump_generic_node(pp, TREE_OPERAND(node, 1), spc, flags, false);
        //pp_string(pp, ", ");
        dump_generic_node(pp, TREE_OPERAND(node, 2), spc, flags, false);
        //pp_string(pp, " (");
        if (INTEGRAL_TYPE_P(TREE_TYPE(TREE_OPERAND(node, 1))))
        {
            //pp_decimal_int(pp, TYPE_PRECISION(TREE_TYPE(TREE_OPERAND(node, 1))));
        }
        else
        {
            dump_generic_node(pp, TYPE_SIZE(TREE_TYPE(TREE_OPERAND(node, 1))), spc, flags, false);
        }
        //pp_string(pp, " bits)>");
        break;

    case ARRAY_REF:
    case ARRAY_RANGE_REF:
        op0 = TREE_OPERAND(node, 0);
        if (op_prio(op0) < op_prio(node))
        {
            //pp_left_paren(pp);
        }
        dump_generic_node(pp, op0, spc, flags, false);
        if (op_prio(op0) < op_prio(node))
        {
            //pp_right_paren(pp);
        }
        dump_generic_node(pp, TREE_OPERAND(node, 1), spc, flags, false);
        if (TREE_CODE(node) == ARRAY_RANGE_REF)
        {
            //pp_string(pp, " ...");
        }

        op0 = array_ref_low_bound(node);
        op1 = array_ref_element_size(node);

        if (!integer_zerop(op0) || TREE_OPERAND(node, 2) || TREE_OPERAND(node, 3))
        {
            //pp_string(pp, "{lb: ");
            dump_generic_node(pp, op0, spc, flags, false);
            //pp_string(pp, " sz: ");
            dump_generic_node(pp, op1, spc, flags, false);
            //pp_right_brace(pp);
        }
        break;

    case CONSTRUCTOR:
    {
        unsigned HOST_WIDE_INT ix;
        tree field, val;
        bool is_struct_init = false;
        bool is_array_init = false;
        widest_int curidx;
        if (TREE_CLOBBER_P(node))
        {
            //pp_string(pp, "CLOBBER");
        }
        else if (TREE_CODE(TREE_TYPE(node)) == RECORD_TYPE || TREE_CODE(TREE_TYPE(node)) == UNION_TYPE)
        {
            is_struct_init = true;
        }
        else if (TREE_CODE(TREE_TYPE(node)) == ARRAY_TYPE && TYPE_DOMAIN(TREE_TYPE(node)) && TYPE_MIN_VALUE(TYPE_DOMAIN(TREE_TYPE(node))) && TREE_CODE(TYPE_MIN_VALUE(TYPE_DOMAIN(TREE_TYPE(node)))) == INTEGER_CST)
        {
            tree minv = TYPE_MIN_VALUE(TYPE_DOMAIN(TREE_TYPE(node)));
            is_array_init = true;
            curidx = wi::to_widest(minv);
        }
        FOR_EACH_CONSTRUCTOR_ELT(CONSTRUCTOR_ELTS(node), ix, field, val)
        {
            if (field)
            {
                if (is_struct_init)
                {
                    //pp_dot(pp);
                    dump_generic_node(pp, field, spc, flags, false);
                    //pp_equal(pp);
                }
                else if (is_array_init && (TREE_CODE(field) != INTEGER_CST || curidx != wi::to_widest(field)))
                {
                    if (TREE_CODE(field) == RANGE_EXPR)
                    {
                        dump_generic_node(pp, TREE_OPERAND(field, 0), spc, flags, false);
                        //pp_string(pp, " ... ");
                        dump_generic_node(pp, TREE_OPERAND(field, 1), spc, flags, false);
                        if (TREE_CODE(TREE_OPERAND(field, 1)) == INTEGER_CST)
                        {
                            curidx = wi::to_widest(TREE_OPERAND(field, 1));
                        }
                    }
                    else
                    {
                        dump_generic_node(pp, field, spc, flags, false);
                    }
                    if (TREE_CODE(field) == INTEGER_CST)
                    {
                        curidx = wi::to_widest(field);
                    }
                    //pp_string(pp, "]=");
                }
            }
            if (is_array_init)
            {
                curidx += 1;
            }
            if (val && TREE_CODE(val) == ADDR_EXPR)
            {
                if (TREE_CODE(TREE_OPERAND(val, 0)) == FUNCTION_DECL)
                {
                    val = TREE_OPERAND(val, 0);
                }
            }
            if (val && TREE_CODE(val) == FUNCTION_DECL)
            {
                dump_decl_name(pp, val, flags);
            }
            else
            {
                dump_generic_node(pp, val, spc, flags, false);
            }
            if (ix != CONSTRUCTOR_NELTS(node) - 1)
            {
                //pp_comma(pp);
                //pp_space(pp);
            }
        }
    }
    break;

    case COMPOUND_EXPR:
    {
        tree *tp;
        if (flags & TDF_SLIM)
        {
            //pp_string(pp, "<COMPOUND_EXPR>");
            break;
        }

        dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, !(flags & TDF_SLIM));

        for (tp = &TREE_OPERAND(node, 1);
             TREE_CODE(*tp) == COMPOUND_EXPR;
             tp = &TREE_OPERAND(*tp, 1))
        {
            dump_generic_node(pp, TREE_OPERAND(*tp, 0), spc, flags, !(flags & TDF_SLIM));
        }

        dump_generic_node(pp, *tp, spc, flags, !(flags & TDF_SLIM));
    }
    break;

    case STATEMENT_LIST:
    {
        tree_stmt_iterator si;
        if (flags & TDF_SLIM)
        {
            //pp_string(pp, "<STATEMENT_LIST>");
            break;
        }

        for (si = tsi_start(node); !tsi_end_p(si); tsi_next(&si))
        {
            // TODO: mutation_target_stmt(SBR, tsi_stmt(si));
            dump_generic_node(pp, tsi_stmt(si), spc, flags, true);
        }
    }
    break;

    case MODIFY_EXPR:
    case INIT_EXPR:
    {
        dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
        dump_generic_node(pp, TREE_OPERAND(node, 1), spc, flags, false);
    }
    break;

    case TARGET_EXPR:
        //pp_string(pp, "TARGET_EXPR <");
        dump_generic_node(pp, TARGET_EXPR_SLOT(node), spc, flags, false);
        //pp_comma(pp);
        //pp_space(pp);
        dump_generic_node(pp, TARGET_EXPR_INITIAL(node), spc, flags, false);
        //pp_greater(pp);
        break;

    case DECL_EXPR:
        print_declaration(pp, DECL_EXPR_DECL(node), spc, flags);
        is_stmt = false;
        break;

    case COND_EXPR:
        if (TREE_TYPE(node) == NULL || TREE_TYPE(node) == void_type_node)
        {
            //pp_string(pp, "if (");
            dump_generic_node(pp, COND_EXPR_COND(node), spc, flags, false);
            //pp_right_paren(pp);
            /* The lowered cond_exprs should always be printed in full.  */
            if (COND_EXPR_THEN(node) && (IS_EMPTY_STMT(COND_EXPR_THEN(node)) || TREE_CODE(COND_EXPR_THEN(node)) == GOTO_EXPR) && COND_EXPR_ELSE(node) && (IS_EMPTY_STMT(COND_EXPR_ELSE(node)) || TREE_CODE(COND_EXPR_ELSE(node)) == GOTO_EXPR))
            {
                //pp_space(pp);
                dump_generic_node(pp, COND_EXPR_THEN(node), 0, flags, true);
                if (!IS_EMPTY_STMT(COND_EXPR_ELSE(node)))
                {
                    //pp_string(pp, " else ");
                    dump_generic_node(pp, COND_EXPR_ELSE(node), 0, flags, true);
                }
            }
            else if (!(flags & TDF_SLIM))
            {
                /* Output COND_EXPR_THEN.  */
                if (COND_EXPR_THEN(node))
                {
                    dump_generic_node(pp, COND_EXPR_THEN(node), spc + 4, flags, true);
                }

                /* Output COND_EXPR_ELSE.  */
                if (COND_EXPR_ELSE(node) && !IS_EMPTY_STMT(COND_EXPR_ELSE(node)))
                {
                    dump_generic_node(pp, COND_EXPR_ELSE(node), spc + 4, flags, true);
                }
            }
            is_expr = false;
        }
        else
        {
            // a ? b : c
            dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
            dump_generic_node(pp, TREE_OPERAND(node, 1), spc, flags, false);
            dump_generic_node(pp, TREE_OPERAND(node, 2), spc, flags, false);
        }
        break;

    case BIND_EXPR:
    {
        if (!(flags & TDF_SLIM))
        {
            if (BIND_EXPR_VARS(node))
            {
                for (op0 = BIND_EXPR_VARS(node); op0; op0 = DECL_CHAIN(op0))
                {
                    print_declaration(pp, op0, spc + 2, flags);
                }
            }
            dump_generic_node(pp, BIND_EXPR_BODY(node), spc + 2, flags, true);
        }
        is_expr = false;
    }
    break;

    case CALL_EXPR:
        if (CALL_EXPR_FN(node) != NULL_TREE)
        {
            print_call_name(pp, CALL_EXPR_FN(node), flags);
        }
        else
        {
            //pp_string(pp, internal_fn_name(CALL_EXPR_IFN(node)));
        }

        /* Print parameters.  */
        {
            tree arg;
            call_expr_arg_iterator iter;
            FOR_EACH_CALL_EXPR_ARG(arg, iter, node)
            {
                dump_generic_node(pp, arg, spc, flags, false);
                if (more_call_expr_args_p(&iter))
                {
                    //pp_comma(pp);
                    //pp_space(pp);
                }
            }
        }
        if (CALL_EXPR_VA_ARG_PACK(node))
        {
            if (call_expr_nargs(node) > 0)
            {
                //pp_comma(pp);
                //pp_space(pp);
            }
            //pp_string(pp, "__builtin_va_arg_pack ()");
        }

        op1 = CALL_EXPR_STATIC_CHAIN(node);
        if (op1)
        {
            //pp_string(pp, " [static-chain: ");
            dump_generic_node(pp, op1, spc, flags, false);
            //pp_right_bracket(pp);
        }

        if (CALL_EXPR_RETURN_SLOT_OPT(node))
        {
            //pp_string(pp, " [return slot optimization]");
        }
        if (CALL_EXPR_TAILCALL(node))
        {
            //pp_string(pp, " [tail call]");
        }
        break;

    case WITH_CLEANUP_EXPR:
        NIY;
        break;

    case CLEANUP_POINT_EXPR:
        //pp_string(pp, "<<cleanup_point ");
        dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
        //pp_string(pp, ">>");
        break;

    case PLACEHOLDER_EXPR:
        //pp_string(pp, "<PLACEHOLDER_EXPR ");
        dump_generic_node(pp, TREE_TYPE(node), spc, flags, false);
        //pp_greater(pp);
        break;

        /* Binary arithmetic and logic expressions.  */
    case WIDEN_SUM_EXPR:
    case WIDEN_MULT_EXPR:
    case MULT_EXPR:
    case MULT_HIGHPART_EXPR:
    case PLUS_EXPR:
    case POINTER_PLUS_EXPR:
    case MINUS_EXPR:
    case TRUNC_DIV_EXPR:
    case CEIL_DIV_EXPR:
    case FLOOR_DIV_EXPR:
    case ROUND_DIV_EXPR:
    case TRUNC_MOD_EXPR:
    case CEIL_MOD_EXPR:
    case FLOOR_MOD_EXPR:
    case ROUND_MOD_EXPR:
    case RDIV_EXPR:
    case EXACT_DIV_EXPR:
    {
        mutation_target_expr(AOR, node);
        op0 = TREE_OPERAND(node, 0);
        op1 = TREE_OPERAND(node, 1);

        dump_generic_node(pp, op0, spc, flags, false);
        dump_generic_node(pp, op1, spc, flags, false);
    }
    break;

    case LSHIFT_EXPR:
    case RSHIFT_EXPR:
    case LROTATE_EXPR:
    case RROTATE_EXPR:
    case WIDEN_LSHIFT_EXPR:
    {
        mutation_target_expr(SOR, node);
        op0 = TREE_OPERAND(node, 0);
        op1 = TREE_OPERAND(node, 1);

        dump_generic_node(pp, op0, spc, flags, false);
        dump_generic_node(pp, op1, spc, flags, false);
    }
    break;

    case BIT_IOR_EXPR:
    case BIT_XOR_EXPR:
    case BIT_AND_EXPR:
    case BIT_NOT_EXPR:
    {
        mutation_target_expr(BOR, node);
        op0 = TREE_OPERAND(node, 0);
        op1 = TREE_OPERAND(node, 1);

        dump_generic_node(pp, op0, spc, flags, false);
        dump_generic_node(pp, op1, spc, flags, false);
    }
    break;

    case TRUTH_ANDIF_EXPR:
    case TRUTH_ORIF_EXPR:
    case TRUTH_AND_EXPR:
    case TRUTH_OR_EXPR:
    case TRUTH_XOR_EXPR:
    {
        mutation_target_expr(LCR, node);
        op0 = TREE_OPERAND(node, 0);
        op1 = TREE_OPERAND(node, 1);

        dump_generic_node(pp, op0, spc, flags, false);
        dump_generic_node(pp, op1, spc, flags, false);
    }
    break;

    case LT_EXPR:
    case LE_EXPR:
    case GT_EXPR:
    case GE_EXPR:
    case EQ_EXPR:
    case NE_EXPR:
    {
        mutation_target_expr(ROR, node);
        op0 = TREE_OPERAND(node, 0);
        op1 = TREE_OPERAND(node, 1);

        dump_generic_node(pp, op0, spc, flags, false);
        dump_generic_node(pp, op1, spc, flags, false);
    }
    break;

    case UNLT_EXPR:
    case UNLE_EXPR:
    case UNGT_EXPR:
    case UNGE_EXPR:
    case UNEQ_EXPR:
    case LTGT_EXPR:
    case ORDERED_EXPR:
    case UNORDERED_EXPR:
    {
        mutation_target_expr(ROR, node);
        op0 = TREE_OPERAND(node, 0);
        op1 = TREE_OPERAND(node, 1);

        dump_generic_node(pp, op0, spc, flags, false);
        dump_generic_node(pp, op1, spc, flags, false);
    }
    break;

        /* Unary arithmetic and logic expressions.  */
    case ADDR_EXPR:
    case INDIRECT_REF:
    {
        mutation_target_expr(UNK, node);

        if (TREE_CODE(node) == ADDR_EXPR && (TREE_CODE(TREE_OPERAND(node, 0)) == STRING_CST || TREE_CODE(TREE_OPERAND(node, 0)) == FUNCTION_DECL))
            ; /* Do not output '&' for strings and function pointers.  */

        if (op_prio(TREE_OPERAND(node, 0)) < op_prio(node))
        {
            dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
        }
        else
            dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
    }
    break;

    case TRUTH_NOT_EXPR:
    case NEGATE_EXPR:
    case PREDECREMENT_EXPR:
    case PREINCREMENT_EXPR:
    {
        mutation_target_expr(UOR, node);

        if (TREE_CODE(node) == ADDR_EXPR && (TREE_CODE(TREE_OPERAND(node, 0)) == STRING_CST || TREE_CODE(TREE_OPERAND(node, 0)) == FUNCTION_DECL))
            ; /* Do not output '&' for strings and function pointers.  */

        dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
    }
    break;

    case POSTDECREMENT_EXPR:
    case POSTINCREMENT_EXPR:
    {
        mutation_target_expr(UOR, node);

        dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
    }
    break;

    case MIN_EXPR:
    case MAX_EXPR:
    {
        dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
        dump_generic_node(pp, TREE_OPERAND(node, 1), spc, flags, false);
    }
    break;

    case ABS_EXPR:
    {
        dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
    }
    break;

    case RANGE_EXPR:
        NIY;
        break;

    case ADDR_SPACE_CONVERT_EXPR:
    case FIXED_CONVERT_EXPR:
    case FIX_TRUNC_EXPR:
    case FLOAT_EXPR:
    CASE_CONVERT:
        type = TREE_TYPE(node);
        op0 = TREE_OPERAND(node, 0);
        if (type != TREE_TYPE(op0))
        {
            //pp_left_paren(pp);
            dump_generic_node(pp, type, spc, flags, false);
            //pp_string(pp, ") ");
        }
        if (op_prio(op0) < op_prio(node))
        {
            //pp_left_paren(pp);
        }
        dump_generic_node(pp, op0, spc, flags, false);
        if (op_prio(op0) < op_prio(node))
        {
            //pp_right_paren(pp);
        }
        break;

    case VIEW_CONVERT_EXPR:
        //pp_string(pp, "VIEW_CONVERT_EXPR<");
        dump_generic_node(pp, TREE_TYPE(node), spc, flags, false);
        //pp_string(pp, ">(");
        dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
        //pp_right_paren(pp);
        break;

    case PAREN_EXPR:
    case NON_LVALUE_EXPR:
    case SAVE_EXPR:
    case CONJ_EXPR:
    case VA_ARG_EXPR:
        dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
        break;

    case COMPLEX_EXPR:
        //pp_string(pp, "COMPLEX_EXPR <");
        dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
        //pp_string(pp, ", ");
        dump_generic_node(pp, TREE_OPERAND(node, 1), spc, flags, false);
        //pp_greater(pp);
        break;

    case REALPART_EXPR:
        if (flags & TDF_GIMPLE)
        {
            //pp_string(pp, "__real ");
            dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
        }
        else
        {
            //pp_string(pp, "REALPART_EXPR <");
            dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
            //pp_greater(pp);
        }
        break;

    case IMAGPART_EXPR:
        if (flags & TDF_GIMPLE)
        {
            //pp_string(pp, "__imag ");
            dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
        }
        else
        {
            //pp_string(pp, "IMAGPART_EXPR <");
            dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
            //pp_greater(pp);
        }
        break;

    case TRY_FINALLY_EXPR:
    case TRY_CATCH_EXPR:
        dump_generic_node(pp, TREE_OPERAND(node, 0), spc + 4, flags, true);
        dump_generic_node(pp, TREE_OPERAND(node, 1), spc + 4, flags, true);
        is_expr = false;
        break;

    case CATCH_EXPR:
        dump_generic_node(pp, CATCH_TYPES(node), spc + 2, flags, false);
        dump_generic_node(pp, CATCH_BODY(node), spc + 4, flags, true);
        is_expr = false;
        break;

    case EH_FILTER_EXPR:
        dump_generic_node(pp, EH_FILTER_TYPES(node), spc + 2, flags, false);
        dump_generic_node(pp, EH_FILTER_FAILURE(node), spc + 4, flags, true);
        is_expr = false;
        break;

    case LABEL_EXPR:
        op0 = TREE_OPERAND(node, 0);
        /* If this is for break or continue, don't bother printing it.  */
        if (DECL_NAME(op0))
        {
            const char *name = IDENTIFIER_POINTER(DECL_NAME(op0));
            if (strcmp(name, "break") == 0 || strcmp(name, "continue") == 0)
                break;
        }
        dump_generic_node(pp, op0, spc, flags, false);
        //pp_colon(pp);
        if (DECL_NONLOCAL(op0))
        {
            //pp_string(pp, " [non-local]");
        }
        break;

    case LOOP_EXPR:
        if (!(flags & TDF_SLIM))
        {
            dump_generic_node(pp, LOOP_EXPR_BODY(node), spc + 4, flags, true);
        }
        is_expr = false;
        break;

    case PREDICT_EXPR:
        //pp_string(pp, "// predicted ");
        if (PREDICT_EXPR_OUTCOME(node))
        {
            //pp_string(pp, "likely by ");
        }
        else
        {
            //pp_string(pp, "unlikely by ");
        }
        //pp_string(pp, predictor_name(PREDICT_EXPR_PREDICTOR(node)));
        //pp_string(pp, " predictor.");
        break;

    case ANNOTATE_EXPR:
        //pp_string(pp, "ANNOTATE_EXPR <");
        dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
        switch ((enum annot_expr_kind)TREE_INT_CST_LOW(TREE_OPERAND(node, 1)))
        {
        case annot_expr_ivdep_kind:
            //pp_string(pp, ", ivdep");
            break;
        case annot_expr_no_vector_kind:
            //pp_string(pp, ", no-vector");
            break;
        case annot_expr_vector_kind:
            //pp_string(pp, ", vector");
            break;
        default:
            gcc_unreachable();
        }
        //pp_greater(pp);
        break;

    case RETURN_EXPR:
        op0 = TREE_OPERAND(node, 0);
        if (op0)
        {
            if (TREE_CODE(op0) == MODIFY_EXPR)
                dump_generic_node(pp, TREE_OPERAND(op0, 1), spc, flags, false);
            else
                dump_generic_node(pp, op0, spc, flags, false);
        }
        break;

    case EXIT_EXPR:
        //pp_string(pp, "if (");
        dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
        //pp_string(pp, ") break");
        break;

    case SWITCH_EXPR:
        //pp_string(pp, "switch (");
        dump_generic_node(pp, SWITCH_COND(node), spc, flags, false);
        if (!(flags & TDF_SLIM))
        {
            if (SWITCH_BODY(node))
            {
                dump_generic_node(pp, SWITCH_BODY(node), spc + 4, flags, true);
            }
            else
            {
                tree vec = SWITCH_LABELS(node);
                size_t i, n = TREE_VEC_LENGTH(vec);
                for (i = 0; i < n; ++i)
                {
                    tree elt = TREE_VEC_ELT(vec, i);
                    if (elt)
                    {
                        dump_generic_node(pp, elt, spc + 4, flags, false);
                        //pp_string(pp, " goto ");
                        dump_generic_node(pp, CASE_LABEL(elt), spc + 4, flags, true);
                    }
                    else
                    {
                        //pp_string(pp, "case ???: goto ???;");
                    }
                }
            }
        }
        is_expr = false;
        break;

    case GOTO_EXPR:
        op0 = GOTO_DESTINATION(node);
        if (TREE_CODE(op0) != SSA_NAME && DECL_P(op0) && DECL_NAME(op0))
        {
            const char *name = IDENTIFIER_POINTER(DECL_NAME(op0));
            if (strcmp(name, "break") == 0 || strcmp(name, "continue") == 0)
            {
                //pp_string(pp, name);
                break;
            }
        }
        //pp_string(pp, "goto ");
        dump_generic_node(pp, op0, spc, flags, false);
        break;

    case ASM_EXPR:
        //pp_string(pp, "__asm__");
        if (ASM_VOLATILE_P(node))
        {
            //pp_string(pp, " __volatile__");
        }
        dump_generic_node(pp, ASM_STRING(node), spc, flags, false);
        dump_generic_node(pp, ASM_OUTPUTS(node), spc, flags, false);
        dump_generic_node(pp, ASM_INPUTS(node), spc, flags, false);
        if (ASM_CLOBBERS(node))
        {
            dump_generic_node(pp, ASM_CLOBBERS(node), spc, flags, false);
        }
        break;

    case CASE_LABEL_EXPR:
        if (CASE_LOW(node) && CASE_HIGH(node))
        {
            //pp_string(pp, "case ");
            dump_generic_node(pp, CASE_LOW(node), spc, flags, false);
            //pp_string(pp, " ... ");
            dump_generic_node(pp, CASE_HIGH(node), spc, flags, false);
        }
        else if (CASE_LOW(node))
        {
            //pp_string(pp, "case ");
            dump_generic_node(pp, CASE_LOW(node), spc, flags, false);
        }
        else
        {
            //pp_string(pp, "default");
        }
        break;

    case OBJ_TYPE_REF:
        //pp_string(pp, "OBJ_TYPE_REF(");
        dump_generic_node(pp, OBJ_TYPE_REF_EXPR(node), spc, flags, false);

        if (!(flags & TDF_SLIM) && virtual_method_call_p(node))
        {
            //pp_string(pp, "(");
            dump_generic_node(pp, obj_type_ref_class(node), spc, flags, false);
            //pp_string(pp, ")");
        }
        dump_generic_node(pp, OBJ_TYPE_REF_OBJECT(node), spc, flags, false);
        //pp_arrow(pp);
        dump_generic_node(pp, OBJ_TYPE_REF_TOKEN(node), spc, flags, false);
        break;

    case SSA_NAME:
        if (SSA_NAME_IDENTIFIER(node))
        {
            if ((flags & TDF_NOUID) && SSA_NAME_VAR(node) && DECL_NAMELESS(SSA_NAME_VAR(node)))
                dump_fancy_name(pp, SSA_NAME_IDENTIFIER(node));
            else if (!(flags & TDF_GIMPLE) || SSA_NAME_VAR(node))
                dump_generic_node(pp, SSA_NAME_IDENTIFIER(node), spc, flags, false);
        }
        //pp_underscore(pp);
        //pp_decimal_int(pp, SSA_NAME_VERSION(node));
        if (SSA_NAME_IS_DEFAULT_DEF(node))
        {
            //pp_string(pp, "(D)");
        }
        if (SSA_NAME_OCCURS_IN_ABNORMAL_PHI(node))
        {
            //pp_string(pp, "(ab)");
        }
        break;

    case WITH_SIZE_EXPR:
        //pp_string(pp, "WITH_SIZE_EXPR <");
        dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
        //pp_string(pp, ", ");
        dump_generic_node(pp, TREE_OPERAND(node, 1), spc, flags, false);
        //pp_greater(pp);
        break;

    case ASSERT_EXPR:
        //pp_string(pp, "ASSERT_EXPR <");
        dump_generic_node(pp, ASSERT_EXPR_VAR(node), spc, flags, false);
        //pp_string(pp, ", ");
        dump_generic_node(pp, ASSERT_EXPR_COND(node), spc, flags, false);
        //pp_greater(pp);
        break;

    case SCEV_KNOWN:
        //pp_string(pp, "scev_known");
        break;

    case SCEV_NOT_KNOWN:
        //pp_string(pp, "scev_not_known");
        break;

    case POLYNOMIAL_CHREC:
        //pp_left_brace(pp);
        dump_generic_node(pp, CHREC_LEFT(node), spc, flags, false);
        //pp_string(pp, ", +, ");
        dump_generic_node(pp, CHREC_RIGHT(node), spc, flags, false);
        //pp_string(pp, "}_");
        dump_generic_node(pp, CHREC_VAR(node), spc, flags, false);
        is_stmt = false;
        break;

    case REALIGN_LOAD_EXPR:
    case VEC_COND_EXPR:
    case VEC_PERM_EXPR:
    case DOT_PROD_EXPR:
    case WIDEN_MULT_PLUS_EXPR:
    case WIDEN_MULT_MINUS_EXPR:
    case FMA_EXPR:
        dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
        dump_generic_node(pp, TREE_OPERAND(node, 1), spc, flags, false);
        dump_generic_node(pp, TREE_OPERAND(node, 2), spc, flags, false);
        break;

    case OACC_PARALLEL:
        //pp_string(pp, "#pragma acc parallel");
        goto dump_omp_clauses_body;

    case OACC_KERNELS:
        //pp_string(pp, "#pragma acc kernels");
        goto dump_omp_clauses_body;

    case OACC_DATA:
        //pp_string(pp, "#pragma acc data");
        dump_omp_clauses(pp, OACC_DATA_CLAUSES(node), spc, flags);
        goto dump_omp_body;

    case OACC_HOST_DATA:
        //pp_string(pp, "#pragma acc host_data");
        dump_omp_clauses(pp, OACC_HOST_DATA_CLAUSES(node), spc, flags);
        goto dump_omp_body;

    case OACC_DECLARE:
        //pp_string(pp, "#pragma acc declare");
        dump_omp_clauses(pp, OACC_DECLARE_CLAUSES(node), spc, flags);
        break;

    case OACC_UPDATE:
        //pp_string(pp, "#pragma acc update");
        dump_omp_clauses(pp, OACC_UPDATE_CLAUSES(node), spc, flags);
        break;

    case OACC_ENTER_DATA:
        //pp_string(pp, "#pragma acc enter data");
        dump_omp_clauses(pp, OACC_ENTER_DATA_CLAUSES(node), spc, flags);
        break;

    case OACC_EXIT_DATA:
        //pp_string(pp, "#pragma acc exit data");
        dump_omp_clauses(pp, OACC_EXIT_DATA_CLAUSES(node), spc, flags);
        break;

    case OACC_CACHE:
        //pp_string(pp, "#pragma acc cache");
        dump_omp_clauses(pp, OACC_CACHE_CLAUSES(node), spc, flags);
        break;

    case OMP_PARALLEL:
        //pp_string(pp, "#pragma omp parallel");
        dump_omp_clauses(pp, OMP_PARALLEL_CLAUSES(node), spc, flags);
        goto dump_omp_body;

    dump_omp_clauses_body:
        dump_omp_clauses(pp, OMP_CLAUSES(node), spc, flags);
        goto dump_omp_body;

    dump_omp_body:
        if (!(flags & TDF_SLIM) && OMP_BODY(node))
        {
            dump_generic_node(pp, OMP_BODY(node), spc + 4, flags, false);
        }
        is_expr = false;
        break;

    case OMP_TASK:
        //pp_string(pp, "#pragma omp task");
        dump_omp_clauses(pp, OMP_TASK_CLAUSES(node), spc, flags);
        goto dump_omp_body;

    case OMP_FOR:
        //pp_string(pp, "#pragma omp for");
        goto dump_omp_loop;

    case OMP_SIMD:
        //pp_string(pp, "#pragma omp simd");
        goto dump_omp_loop;

    case CILK_SIMD:
        //pp_string(pp, "#pragma simd");
        goto dump_omp_loop;

    case CILK_FOR:
        /* This label points one line after dumping the clauses.
	 For _Cilk_for the clauses are dumped after the _Cilk_for (...)
	 parameters are printed out.  */
        goto dump_omp_loop_cilk_for;

    case OMP_DISTRIBUTE:
        //pp_string(pp, "#pragma omp distribute");
        goto dump_omp_loop;

    case OMP_TASKLOOP:
        //pp_string(pp, "#pragma omp taskloop");
        goto dump_omp_loop;

    case OACC_LOOP:
        //pp_string(pp, "#pragma acc loop");
        goto dump_omp_loop;

    case OMP_TEAMS:
        //pp_string(pp, "#pragma omp teams");
        dump_omp_clauses(pp, OMP_TEAMS_CLAUSES(node), spc, flags);
        goto dump_omp_body;

    case OMP_TARGET_DATA:
        //pp_string(pp, "#pragma omp target data");
        dump_omp_clauses(pp, OMP_TARGET_DATA_CLAUSES(node), spc, flags);
        goto dump_omp_body;

    case OMP_TARGET_ENTER_DATA:
        //pp_string(pp, "#pragma omp target enter data");
        dump_omp_clauses(pp, OMP_TARGET_ENTER_DATA_CLAUSES(node), spc, flags);
        is_expr = false;
        break;

    case OMP_TARGET_EXIT_DATA:
        //pp_string(pp, "#pragma omp target exit data");
        dump_omp_clauses(pp, OMP_TARGET_EXIT_DATA_CLAUSES(node), spc, flags);
        is_expr = false;
        break;

    case OMP_TARGET:
        //pp_string(pp, "#pragma omp target");
        dump_omp_clauses(pp, OMP_TARGET_CLAUSES(node), spc, flags);
        goto dump_omp_body;

    case OMP_TARGET_UPDATE:
        //pp_string(pp, "#pragma omp target update");
        dump_omp_clauses(pp, OMP_TARGET_UPDATE_CLAUSES(node), spc, flags);
        is_expr = false;
        break;

    dump_omp_loop:
        dump_omp_clauses(pp, OMP_FOR_CLAUSES(node), spc, flags);

    dump_omp_loop_cilk_for:
        if (!(flags & TDF_SLIM))
        {
            int i;

            if (OMP_FOR_PRE_BODY(node))
            {
                spc += 4;
                dump_generic_node(pp, OMP_FOR_PRE_BODY(node), spc, flags, false);
            }
            if (OMP_FOR_INIT(node))
            {
                spc -= 2;
                for (i = 0; i < TREE_VEC_LENGTH(OMP_FOR_INIT(node)); i++)
                {
                    spc += 2;
                    if (TREE_CODE(node) == CILK_FOR)
                    {
                        //pp_string(pp, "_Cilk_for (");
                    }
                    else
                    {
                        //pp_string(pp, "for (");
                    }
                    dump_generic_node(pp, TREE_VEC_ELT(OMP_FOR_INIT(node), i), spc, flags, false);
                    //pp_string(pp, "; ");
                    dump_generic_node(pp, TREE_VEC_ELT(OMP_FOR_COND(node), i), spc, flags, false);
                    //pp_string(pp, "; ");
                    dump_generic_node(pp, TREE_VEC_ELT(OMP_FOR_INCR(node), i), spc, flags, false);
                    //pp_right_paren(pp);
                }
                if (TREE_CODE(node) == CILK_FOR)
                {
                    dump_omp_clauses(pp, OMP_FOR_CLAUSES(node), spc, flags);
                }
            }
            if (OMP_FOR_BODY(node))
            {
                dump_generic_node(pp, OMP_FOR_BODY(node), spc + 4, flags, false);
            }
            if (OMP_FOR_INIT(node))
            {
                spc -= 2 * TREE_VEC_LENGTH(OMP_FOR_INIT(node)) - 2;
            }
            if (OMP_FOR_PRE_BODY(node))
            {
                spc -= 4;
            }
        }
        is_expr = false;
        break;

    case OMP_SECTIONS:
        //pp_string(pp, "#pragma omp sections");
        dump_omp_clauses(pp, OMP_SECTIONS_CLAUSES(node), spc, flags);
        goto dump_omp_body;

    case OMP_SECTION:
    case OMP_MASTER:
    case OMP_TASKGROUP:
        goto dump_omp_body;

    case OMP_ORDERED:
        //pp_string(pp, "#pragma omp ordered");
        dump_omp_clauses(pp, OMP_ORDERED_CLAUSES(node), spc, flags);
        goto dump_omp_body;

    case OMP_CRITICAL:
        //pp_string(pp, "#pragma omp critical");
        if (OMP_CRITICAL_NAME(node))
        {
            //pp_space(pp);
            //pp_left_paren(pp);
            dump_generic_node(pp, OMP_CRITICAL_NAME(node), spc, flags, false);
            //pp_right_paren(pp);
        }
        dump_omp_clauses(pp, OMP_CRITICAL_CLAUSES(node), spc, flags);
        goto dump_omp_body;

    case OMP_ATOMIC:
        //pp_string(pp, "#pragma omp atomic");
        if (OMP_ATOMIC_SEQ_CST(node))
        {
            //pp_string(pp, " seq_cst");
        }
        dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
        dump_generic_node(pp, TREE_OPERAND(node, 1), spc, flags, false);
        break;

    case OMP_ATOMIC_READ:
        //pp_string(pp, "#pragma omp atomic read");
        if (OMP_ATOMIC_SEQ_CST(node))
        {
            //pp_string(pp, " seq_cst");
        }
        dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
        break;

    case OMP_ATOMIC_CAPTURE_OLD:
    case OMP_ATOMIC_CAPTURE_NEW:
        //pp_string(pp, "#pragma omp atomic capture");
        if (OMP_ATOMIC_SEQ_CST(node))
        {
            //pp_string(pp, " seq_cst");
        }
        dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
        dump_generic_node(pp, TREE_OPERAND(node, 1), spc, flags, false);
        break;

    case OMP_SINGLE:
        //pp_string(pp, "#pragma omp single");
        dump_omp_clauses(pp, OMP_SINGLE_CLAUSES(node), spc, flags);
        goto dump_omp_body;

    case OMP_CLAUSE:
        dump_omp_clause(pp, node, spc, flags);
        is_expr = false;
        break;

    case TRANSACTION_EXPR:
        if (TRANSACTION_EXPR_OUTER(node))
        {
            //pp_string(pp, "__transaction_atomic [[outer]]");
        }
        else if (TRANSACTION_EXPR_RELAXED(node))
        {
            //pp_string(pp, "__transaction_relaxed");
        }
        else
        {
            //pp_string(pp, "__transaction_atomic");
        }
        if (!(flags & TDF_SLIM) && TRANSACTION_EXPR_BODY(node))
        {
            dump_generic_node(pp, TRANSACTION_EXPR_BODY(node), spc + 2, flags, false);
        }
        is_expr = false;
        break;

    case REDUC_MAX_EXPR:
    case REDUC_MIN_EXPR:
    case REDUC_PLUS_EXPR:
        dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
        break;

    case VEC_WIDEN_MULT_HI_EXPR:
    case VEC_WIDEN_MULT_LO_EXPR:
    case VEC_WIDEN_MULT_EVEN_EXPR:
    case VEC_WIDEN_MULT_ODD_EXPR:
    case VEC_WIDEN_LSHIFT_HI_EXPR:
    case VEC_WIDEN_LSHIFT_LO_EXPR:
        for (str = get_tree_code_name(code); *str; str++)
        {
            //pp_character(pp, TOUPPER(*str));
        }
        //pp_string(pp, " < ");
        dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
        //pp_string(pp, ", ");
        dump_generic_node(pp, TREE_OPERAND(node, 1), spc, flags, false);
        //pp_string(pp, " > ");
        break;

    case VEC_UNPACK_HI_EXPR:
    case VEC_UNPACK_LO_EXPR:
    case VEC_UNPACK_FLOAT_HI_EXPR:
    case VEC_UNPACK_FLOAT_LO_EXPR:
        dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
        break;

    case VEC_PACK_TRUNC_EXPR:
    case VEC_PACK_SAT_EXPR:
    case VEC_PACK_FIX_TRUNC_EXPR:
        dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
        dump_generic_node(pp, TREE_OPERAND(node, 1), spc, flags, false);
        break;

    case BLOCK:
        dump_block_node(pp, node, spc, flags);
        break;

    case CILK_SPAWN_STMT:
        //pp_string(pp, "_Cilk_spawn ");
        dump_generic_node(pp, TREE_OPERAND(node, 0), spc, flags, false);
        break;

    case CILK_SYNC_STMT:
        //pp_string(pp, "_Cilk_sync");
        break;

    default:
        NIY;
    }

    if (is_stmt && is_expr)
    {
        //pp_semicolon(pp);
    }

    return spc;
}

static void pre_genericize_callback(void *event_data, void *user_data)
{
    tree t = (tree)event_data;
    while (t)
    {
        dump_generic_node((void *)NULL, t, 0, TDF_VERBOSE | TDF_LINENO, false);
        t = TREE_CHAIN(t);
    }
}

int plugin_init(struct plugin_name_args *plugin_info, struct plugin_gcc_version *version)
{
    register_callback(plugin_info->base_name, PLUGIN_PRE_GENERICIZE, pre_genericize_callback, NULL);

    mutants_file = fopen("mutants.csv", "w");

    return 0;
}