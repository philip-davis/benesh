%{
    #include <stdio.h>
    #include <string.h>
    #include "util.h"
    #include "xc_parse.h"
    #define YY_SKIP_YYWRAP
    #define YY_NO_INPUT
    #define YY_DECL int yylex(XC_STYPE *lvalp, XC_LTYPE *xc_lloc)
    #define YY_USER_INIT { \
        xc_ipush(&g_indent_levels, 0); \
        BEGIN(start); \
    }

%}
    extern int yywrap() { return 1; };

    static const unsigned int TAB_WIDTH = 4;
    static struct xc_int_stack *g_indent_levels = NULL;
    static int g_current_indent = 0;
    static int g_fake_symbol = 0;

    int yycolumn = 1;
    void set_yycolumn(XC_LTYPE *xc_lloc, int val) {
        yycolumn = val;
        xc_lloc->first_column = yycolumn;
        xc_lloc->last_column = yycolumn + yyleng - 1;
    }

    #define YY_USER_ACTION { \
        xc_lloc->first_line = xc_lloc->last_line = yylineno; \
        set_yycolumn(xc_lloc, yycolumn); \
        yycolumn += yyleng; \
    }


D   [0-9]
id  [A-Za-z_][A-Za-z0-9_]*

    /*
        The following indentation code is based on:
            https://github.com/lucasb-eyer/flex-bison-indentation
        See LICENSE.beyer for details.
    */

%option yylineno

%x start
%x indent
%s normal
%s obj

%%
<start>. {
    set_yycolumn(xc_lloc, yycolumn - 1);
    yyless(0);
    BEGIN(indent);
}

<start>\n {
    yyless(0);
    BEGIN(indent);
}

<indent>" " {
    g_current_indent++;
}
<indent>\t {
    g_current_indent = (g_current_indent + TAB_WIDTH) & ~(TAB_WIDTH - 1);
}
<indent>\n {
    g_current_indent = 0;
}
<indent><<EOF>> {
    if(xc_itop(g_indent_levels) != 0) {
        xc_ipop(&g_indent_levels);
        if(xc_itop(g_indent_levels)) {
            int i;
            unput('\n');
            for(i = 0; i < xc_itop(g_indent_levels); i++) {
                unput(' ');
            }
        } else {
            BEGIN(normal);
        }

        return(OUTDENT);

    } else {
        BEGIN(normal);
    }
}
<indent>. {
    if(!g_fake_symbol) {
        unput(*yytext);
    }
    set_yycolumn(xc_lloc, yycolumn - 1);
    if(g_current_indent > xc_itop(g_indent_levels)) {
        xc_ipush(&g_indent_levels, g_current_indent);
        BEGIN(normal);

        return(INDENT);

    } else if(g_current_indent < xc_itop(g_indent_levels)) {
        xc_ipop(&g_indent_levels);
        if(g_current_indent != xc_itop(g_indent_levels)) {
            int i;
            for(i = 0; i < g_current_indent; i++) {
                unput(' ');
            }
            unput('\n');
            unput('.');
            g_fake_symbol = 1;
            for(i = 0; i < xc_itop(g_indent_levels); i++) {
                unput(' ');
            }
            unput('\n');
        } else {
            BEGIN(normal);
        }

        return(OUTDENT);

    } else {
        // avoid returning an NEWLINE for fake outdent lines
        if(g_fake_symbol) {
            g_fake_symbol = 0;
        } else {
            BEGIN(normal);
        }
    }
}

    /*  End of lucasb-eyer code */

<normal>\n  {

    g_current_indent = 0;
    BEGIN(indent);
    return(NEWLINE);
}
<normal>:= {

    return(DASG);
}
<normal>\[\[ {
    return(OEXPR);
}
<normal>\]\] {
    BEGIN(obj); // handle objparts that are just integers
    return(CLEXPR);
}
<normal>[:;<>\{\}\(\),\*\[\]=%\.@+] {

    return(yytext[0]);
}
<normal>interface {

    return(INTF);
}
<normal>domain {

    return(DOMAIN);
}
<normal>in {

    return(IN);
}
<normal>out {

    return(OUT);
}
<normal>component {

    return(COMP);
}
<normal>{id}    {
    lvalp->str = malloc(strlen(yytext)+1);
    strcpy(lvalp->str, yytext);
    BEGIN(obj); // handle objparts that are just integers
    return(ID);
}

<normal>"-" {

    return(NEG);
}

<normal>({D}*\.{D}+|{D}+\.{D}*) {

    lvalp->real = atof(yytext);
    return(RCONST);
}
<normal>{D}+ {

    lvalp->val = atol(yytext);
    return(ICONST);
}
    /*
        Without the below, the lexer will tokenize an object like
            a.b.0
        as
            ID '.' ID RCONST

        We want this to instead tokenize to
            ID '.' ID '.' ICONST
        Which requires contextual awareness
    */
<obj>\n  {

    g_current_indent = 0;
    BEGIN(indent);
    return(NEWLINE);
}
<obj>" " {
    BEGIN(normal);
}
<obj>\[\[ {

    BEGIN(normal);
    return(OEXPR);
}
<obj>\]\] {
    BEGIN(normal);
    return(CLEXPR);
}
<obj>[:=@<>\(\)\[\],%{+] {

    BEGIN(normal);
    return(yytext[0]);
}
<obj>[.}] {
    return(yytext[0]);
}
<obj>{id}    {

    lvalp->str = malloc(strlen(yytext)+1);
    strcpy(lvalp->str, yytext);
    return(ID);
}

<obj>"-" {

    BEGIN(normal);
    return(NEG);
}

<obj>{D}+ {

    lvalp->val = atol(yytext);
    return(ICONST);
}

[ \t]+          /* do nothing */

%%
