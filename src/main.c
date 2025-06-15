#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "dym.h"
#include "bigfile.h"

/*
MIT License

Copyright (c) 2025 Aidar Shigapov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// Old C style code just cuz I like it


#define IO_FEATURE_ENABLE true

#define LMACHINE_IMPLEMENTATION
#define LMACHINE_BASIC_UTILS
#define LMACHINE_CACHED_BOOLEANS
#define LMACHINE_STDIO
#define LMACHINE_STRICT
#include <lmachine.h>
#define STB_C_LEXER_IMPLEMENTATION
#include <stb_c_lexer.h>

#define INVALID_VARIABLE_INDEX SIZE_MAX

char* lasm_strdup(const char* str) {
    size_t len;
    char* result;

    if (!str) return NULL;
    
    len = strlen(str);
    result = malloc(len + 1);
    if (!result) return NULL;
    
    result[len] = '\0';
    memcpy(result, str, len);

    return result;
}

typedef struct {
    const char* quote;
    const char* input_filepath;
    bool no_print_resuk;
} config_t;

typedef struct {
    const char* name;
    lm_node_t* node;

} kv_constant_t;

typedef struct {
    kv_constant_t* data;
    size_t size;
    size_t capacity;
    
} constants_t;

typedef struct {
    char* name;
    lm_node_t* node;

} kv_index_t;
typedef struct {
    kv_index_t* data;
    size_t size;
    size_t capacity;

} indecies_t;

typedef struct {
    constants_t constants;
    lm_node_t* entry_point;

} l_vm_state_t;

typedef struct {
    const char* filepath;
    indecies_t indecies;
    stb_lexer lex;
} parser_ctx_t;

void destroy_parse_context(parser_ctx_t* ctx) {
    size_t i;

    if (!ctx) return;

    for (i = 0; i < ctx->indecies.size; ++i) {
        if (ctx->indecies.data[i].name) free((void*)ctx->indecies.data[i].name);
        lm_destroy_node(ctx->indecies.data[i].node);
    }

    dym_free(&ctx->indecies);
}
void destroy_l_vm_state(l_vm_state_t* state) {
    size_t i;

    if (!state) return;

    for (i = 0; i < state->constants.size; ++i) {
        if (state->constants.data[i].name) free((void*)state->constants.data[i].name);
        lm_destroy_node(state->constants.data[i].node);
    }

    dym_free(&state->constants);
}

void print_usage(const char* selfpath) {
    printf("Usage: %s [OPTIONS] [FILE]\n"
           "Options:\n"
           "  -q/--quote TEXT           Specify quote text to process\n"
           "  -npr/--no-print-result    Disable printing result of entry point evaluation\n"
           "  --help                    Show this help message\n"
           "\n"
           "You can either specify a quote with -q or an input file, but not both.\n",
        selfpath);
}

void process_arguments(config_t* config, int argc, char** argv) {
    char* selfpath;
    if (argc < 2) {
        fprintf(stderr, "No arguments provided. Use `--help` for usage information.\n");
        exit(EXIT_FAILURE);
    }
    
    selfpath = *argv;
    --argc; // Skip selfpath
    ++argv;

    config->quote = NULL;
    config->input_filepath = NULL;
    config->no_print_resuk = false;

    while (argc) {
        if (strcmp(*argv, "--help") == 0) {
            print_usage(selfpath);
            exit(EXIT_SUCCESS);
        }
        else if (strcmp(*argv, "-q") == 0 || strcmp(*argv, "--quote") == 0) {
            if (argc < 2) {
                fprintf(stderr, "Missing argument for `-q` option.\nProvide the quote text after `-q` flag.\n");
                exit(EXIT_FAILURE);
            }
            if (config->quote) {
                fprintf(stderr, "Quote already specified.\nYou can only provide one quote with `-q` option.\n");
                exit(EXIT_FAILURE);
            }

            --argc; // skip `-q`
            ++argv;

            config->quote = *argv;
        } else if (strcmp(*argv, "-npr") == 0 || strcmp(*argv, "--no-print-result") == 0) {
            config->no_print_resuk = true;

        } else {
            if (config->input_filepath) {
                fprintf(stderr, "Multiple input files specified.\nProvide only one input file.\n");
                exit(EXIT_FAILURE);
            }

            config->input_filepath = *argv;
        }

        --argc;
        ++argv;
    }

    if (config->quote && config->input_filepath) {
        fprintf(stderr, "Cannot specify both quote (-q) and file input.\nUse either quote mode or file input mode, but not both.\n");
        exit(EXIT_FAILURE);
    }
    if (!config->quote && !config->input_filepath) {
        fprintf(stderr, "No input specified.\nProvide either a quote with `-q` option or an input file.\n");
        exit(EXIT_FAILURE);
    }
}

lm_node_t* get_index_node_no_copy(parser_ctx_t* ctx, const char* name) {
    bool success = false;
    size_t i;
    kv_index_t new_index = {0};

    
    for (i = 0; i < ctx->indecies.size; ++i) {
        if (strcmp(ctx->indecies.data[i].name, name) == 0) {
            return ctx->indecies.data[i].node;
        }
    }

    new_index.name = lasm_strdup(name);
    if (!new_index.name) goto cleanup;
    new_index.node = lm_mk_var(i);
    if (!new_index.node) goto cleanup;

    success = true;
cleanup:
    if (success) {
        dym_push(&ctx->indecies, new_index);
        return new_index.node;
    } else {
        fprintf(stderr, "Buy more RAM\n");

        if (new_index.name) free(new_index.name);
        if (new_index.node) lm_destroy_node(new_index.node);
        return NULL;
    }
    
}
lm_node_t* get_index_node(parser_ctx_t* ctx, const char* name) {
    return lm_copy_node(get_index_node_no_copy(ctx, name));

}
size_t get_index(parser_ctx_t* ctx, const char* name) {
    lm_node_t* index_node = NULL;

    index_node = get_index_node_no_copy(ctx, name);
    if (!index_node) INVALID_VARIABLE_INDEX;
    
    return index_node->as.variable;

}

lm_node_t* get_constant(const l_vm_state_t* state, const char* name) {
    size_t i;
    const char* cur_name;

    for (i = 0; i < state->constants.size; ++i) {
        if (strcmp(state->constants.data[i].name, name) == 0) {
            return lm_copy_node(state->constants.data[i].node);
        }
    }

    return NULL;
}

int stb_c_lexer_peek_token(stb_lexer *lexer, stb_lex_token *tok) {
    stb_lexer lexer_last_state;
    int result;
    if (!lexer) return 1;
    if (!tok) return 1;

    lexer_last_state = *lexer;
    result = stb_c_lexer_get_token(lexer, tok);
    *lexer = lexer_last_state;
    return result;
}

bool parse_lambda_expr(parser_ctx_t* ctx, lm_node_t** expr, const l_vm_state_t* state);
bool parse_expression(parser_ctx_t* ctx, lm_node_t** expr, const l_vm_state_t* state);

bool parse_constant(parser_ctx_t* ctx, l_vm_state_t* state) {
    bool success = false;
    char* constant_name = NULL;
    lm_node_t *expr = NULL;
    kv_constant_t constant;
    stb_lex_token token;
    stb_lex_location pos;

    { // Get name of the constant
        stb_c_lexer_get_token(&ctx->lex, &token);
        if (token.token != CLEX_id) {
            stb_c_lexer_get_location(&ctx->lex, NULL, &pos);

            fprintf(stderr, "%s:%d:%d Expected constant name\n", ctx->filepath, pos.line_number, pos.line_offset);
            goto cleanup;
        }
    }

    { // Allocate new constant name
        constant_name = lasm_strdup(token.string);
        if (constant_name == NULL) {
            stb_c_lexer_get_location(&ctx->lex, NULL, &pos);

            fprintf(stderr, "%s:%d:%d Buy more RAM, dude\n",
                    ctx->filepath, pos.line_number, pos.line_offset);
            goto cleanup;
        }
    }
    
    { // Expect '='
        stb_c_lexer_get_token(&ctx->lex, &token);
        if (token.token != '=') {
            stb_c_lexer_get_location(&ctx->lex, NULL, &pos);

            fprintf(stderr, "%s:%d:%d Expected `=`\n", ctx->filepath, pos.line_number, pos.line_offset);

            goto cleanup;
        }
    }
    
    { // Parse expression
        if (!parse_expression(ctx, &expr, state)) goto cleanup;
    }
    
    { // Process constant
        if (strcmp(constant_name, "main") == 0) { // Entry point
            if (state->entry_point) {
                stb_c_lexer_get_location(&ctx->lex, NULL, &pos);
                
                fprintf(stderr, "%s:%d:%d Entry point double defined\n", ctx->filepath, pos.line_number, pos.line_offset);
                
                goto cleanup;
            }
            
            state->entry_point = expr;
            expr = NULL;    // Avoid double free
            
        } else {
            constant.name = constant_name;
            constant.node = lm_evaluate(expr); // its constant you know
            
            dym_push(&state->constants, constant);
            
            constant_name = NULL;   // Avoid double free
            expr = NULL;            // Avoid double free
        }
    }
    
    success = true;
cleanup:
    if (constant_name) free(constant_name);
    if (expr) lm_destroy_node(expr);
    return success;
}

bool parse_lambda_expr(parser_ctx_t* ctx, lm_node_t** expr, const l_vm_state_t* state) {
    size_t param_index = 0;
    stb_lex_token token;
    stb_lex_location pos;
    lm_node_t *abs_list_head = NULL, *abs_list_tail = NULL;
    bool success = false;

    { // Parse till '.'
      //  In original lambda calculus you should provide only 1 binding name and body after
      //  But here, you can do this `\ a b c. (c b a)`
        while (true) {
            if (!stb_c_lexer_get_token(&ctx->lex, &token)) {
                stb_c_lexer_get_location(&ctx->lex, NULL, &pos);

                fprintf(stderr, "%s:%d:%d EOF in lambda expression\n", ctx->filepath, pos.line_number, pos.line_offset);

                goto cleanup;
            }
            
            if (token.token == '.') {
                break;

            } else if (token.token == CLEX_id) {
                param_index = get_index(ctx, token.string);
                if (param_index == INVALID_VARIABLE_INDEX) goto cleanup;                

                if (abs_list_tail) {
                    abs_list_tail->as.abstraction.body = lm_mk_abs(param_index, NULL);
                    abs_list_tail = abs_list_tail->as.abstraction.body;
                } else {
                    abs_list_tail = lm_mk_abs(param_index, NULL);
                    abs_list_head = abs_list_tail;
                }

            } else {
                stb_c_lexer_get_location(&ctx->lex, NULL, &pos);

                fprintf(stderr, "%s:%d:%d Expected `.` or binding name\n", ctx->filepath, pos.line_number, pos.line_offset);

                goto cleanup;
            }
        }
    }

    { // Validation
        if (!abs_list_head) {
            stb_c_lexer_get_location(&ctx->lex, NULL, &pos);

            fprintf(stderr, "%s:%d:%d Expected at least 1 binding name\n", ctx->filepath, pos.line_number, pos.line_offset);

            goto cleanup;
        }

        if (!parse_expression(ctx, &abs_list_tail->as.abstraction.body, state)) {
            goto cleanup;
        }
    }

    *expr = abs_list_head;
    abs_list_head = NULL;
    success = true;
cleanup:
    if (abs_list_head) lm_destroy_node(abs_list_head); // abs_list_tail - part of linked list
    
    return success;
}

bool parse_expression(parser_ctx_t* ctx, lm_node_t** expr, const l_vm_state_t* state) {
    stb_lex_token token;
    stb_lex_location pos;
    enum lm_node_primitive_t primitive_op;
    lm_node_t *temp_expr = NULL, *temp_expr2;
    int temp_char;

    if (!stb_c_lexer_get_token(&ctx->lex, &token)) {
        stb_c_lexer_get_location(&ctx->lex, NULL, &pos);

        fprintf(stderr, "%s:%d:%d Unexpected EOF in expression: %ld\n",ctx->filepath, pos.line_number, pos.line_offset, token.token);

        return false;
    }
    
    switch (token.token) {
        case CLEX_int: {
            *expr = lm_mk_value(token.int_number);
            break;
        }
        case '<':
        case '>':
        case '=':
        case '/':
        case '%':
        case '*':
        case '-':
        case '+': {
            switch (token.token) {
                case '<': primitive_op = LM_NODE_PRIMITIVE_LESS; break;
                case '>': primitive_op = LM_NODE_PRIMITIVE_MORE; break;
                case '=': primitive_op = LM_NODE_PRIMITIVE_EQ; break;
                case '/': primitive_op = LM_NODE_PRIMITIVE_DIV; break;
                case '%': primitive_op = LM_NODE_PRIMITIVE_MOD; break;
                case '*': primitive_op = LM_NODE_PRIMITIVE_MUL; break;
                case '-': primitive_op = LM_NODE_PRIMITIVE_SUB; break;
                case '+': primitive_op = LM_NODE_PRIMITIVE_ADD; break;
                default: return false;
            }
            *expr = lm_mk_primitive(primitive_op, NULL, NULL);
            return parse_expression(ctx, &(*expr)->as.primitive.args[0], state) && parse_expression(ctx, &(*expr)->as.primitive.args[1], state);
        }
        case '\\': {
            return parse_lambda_expr(ctx, expr, state);
        }
        case '[': // Strict application. Maybe add as option in command line
        case '(': { // Parse application. In common lambda calculus application takes only 2 arguments, but here its varios
            // ((x x) x) -> (x x x)
            // Syntax sugar
            *expr = NULL;
            temp_char = token.token == '[' ? ']' : ')';

            while (true) {
                stb_c_lexer_peek_token(&ctx->lex, &token);
                if (token.token == temp_char) {
                    stb_c_lexer_get_token(&ctx->lex, &token);
                    break;
                }

                if (!parse_expression(ctx, &temp_expr, state)) return false;
                
                if (*expr) {
                    *expr = 
                        temp_char == ']' ? 
                            lm_mk_strict_app(*expr, temp_expr) :
                            lm_mk_app(*expr, temp_expr);
                } else {
                    *expr = temp_expr;
                }
                temp_expr = NULL;

            }

            return *expr;
        }
        
        case CLEX_id: {
            if (strcmp(token.string, "λ") == 0) return parse_lambda_expr(ctx, expr, state);
            if (IO_FEATURE_ENABLE && strcmp(token.string, "put_char") == 0) {
                if (!parse_expression(ctx, &temp_expr, state)) return false;
                if (!parse_expression(ctx, &temp_expr2, state)) return false;
                *expr = lm_mk_primitive(LM_NODE_PRIMITIVE_PRINT_CHAR, temp_expr, temp_expr2);
                temp_expr = NULL;
                temp_expr2 = NULL;
                return *expr;
            }
            if (IO_FEATURE_ENABLE && strcmp(token.string, "get_char") == 0) {
                *expr = lm_mk_primitive(LM_NODE_PRIMITIVE_GET_CHAR, NULL, NULL);
                return *expr;
            }
            if (strcmp(token.string, "true") == 0) {
                *expr = lm_mk_boolean(1);
                return *expr;
            }
            if (strcmp(token.string, "false") == 0) {
                *expr = lm_mk_boolean(0);
                return *expr;
            }

            temp_expr = get_constant(state, token.string);

            if (temp_expr) {
                *expr = temp_expr;
            } else {
                *expr = get_index_node(ctx, token.string);
            }
            return *expr;
        }
        
        default: {
            stb_c_lexer_get_location(&ctx->lex, NULL, &pos);
            
            fprintf(stderr, "%s:%d:%d Unexpected token in expression: %ld\n",ctx->filepath, pos.line_number, pos.line_offset, token.token);

            return false;
        }
    }
    return true;
}
bool parse_quote(l_vm_state_t* state, const char* quote) {
    char* store_buffer = NULL;
    bool success = false;
    size_t i;
    stb_lex_token token;
    stb_lex_location pos;
    parser_ctx_t parse_context = {0};
    
    store_buffer = (char*)malloc(1024);
    if (!store_buffer) {
        perror("Buy more RAM");
        goto cleanup;
    }

    stb_c_lexer_init(&parse_context.lex, quote, quote + strlen(quote), store_buffer, 1024);
    parse_context.filepath = "";

    dym_push(&state->constants,
        ((kv_constant_t) {
            .name = lasm_strdup("Y"),
            .node = lm_mk_y_combinator(),
        })
    );
    dym_push(&state->constants,
        ((kv_constant_t) {
            .name = lasm_strdup("S"),
            .node = lm_mk_s_combinator(),
        })
    );
    dym_push(&state->constants,
        ((kv_constant_t) {
            .name = lasm_strdup("K"),
            .node = lm_mk_k_combinator(),
        })
    );
    dym_push(&state->constants,
        ((kv_constant_t) {
            .name = lasm_strdup("I"),
            .node = lm_mk_i_combinator(),
        })
    );
    
    if (!parse_expression(&parse_context, &state->entry_point, state)) goto cleanup;

    success = true;
cleanup:
    if (store_buffer) free(store_buffer);
    destroy_parse_context(&parse_context);
    return success;
}

bool parse_file(l_vm_state_t* state, const char* filepath) {
    bool success = false;
    char *input_stream = NULL, *store_buffer = NULL;
    size_t file_size = 0, i;
    FILE *file = NULL; 
    stb_lex_token token;
    stb_lex_location pos;
    parser_ctx_t parse_context = {0};

    file = fopen_big(filepath, "rb");
    if (!file) {
        perror("Failed to open file");
        goto cleanup;
    }
    if (!get_file_size(file, &file_size)) {
        perror("Failed to get file size");
        goto cleanup;
    }
    
    input_stream = (char*)malloc(file_size + 1);
    if (!input_stream) {
        perror("Buy more RAM");
        goto cleanup;
    }

    if (fread(input_stream, file_size, 1, file) != 1) {
        perror("Failed to read file");
        goto cleanup;
    }
    *(input_stream + file_size) = '\0';

    store_buffer = (char*)malloc(1024);
    if (!store_buffer) {
        perror("Buy more RAM");
        goto cleanup;
    }

    stb_c_lexer_init(&parse_context.lex, input_stream, input_stream + file_size, store_buffer, 1024);
    parse_context.filepath = filepath;
    
    while (stb_c_lexer_get_token(&parse_context.lex, &token)) {
        if (token.token == CLEX_id && strncmp(token.string, "const", 5) == 0) {
            if (!parse_constant(&parse_context, state)) {
                stb_c_lexer_get_location(&parse_context.lex, NULL, &pos);

                fprintf(stderr, "%s:%d:%d Failed to parse constant\n", filepath, pos.line_number, pos.line_offset, token.string);
                goto cleanup;
            }
        } else {
            stb_c_lexer_get_location(&parse_context.lex, NULL, &pos);
            
            fprintf(stderr, "%s:%d:%d Unexpected top-level token `%s`(`%c`)\n", filepath, pos.line_number, pos.line_offset, token.string, (char)token.token);
            goto cleanup;
        }
    }

    success = true;
cleanup:
    if (input_stream) free(input_stream);
    if (store_buffer) free(store_buffer);
    if (file) fclose(file);
    destroy_parse_context(&parse_context);
    return success;
}

void lm_node_cool_print(const lm_node_t* node) {
    size_t i;

    if (node == NULL) {
        printf("NULL");
        return;
    }
    switch (node->tag) {
        case LM_NODE_ABSTRACTION:
            printf("λ %%%ld.", node->as.abstraction.var_index);
            lm_node_cool_print(node->as.abstraction.body);
            break;
            
        case LM_NODE_APPLICATION:
            printf("(");
            lm_node_cool_print(node->as.application.func);
            printf(" ");
            lm_node_cool_print(node->as.application.arg);
            printf(")");
            break;
            
        case LM_NODE_VARIABLE:
            printf("%%%ld", node->as.variable);
            break;
            
        case LM_NODE_VALUE:
            printf("%lld", node->as.value);
            break;
            
        case LM_NODE_PRIMITIVE:
            printf("([opcode=%d] ", node->as.primitive.opcode);
            for (i = 0; i < _LMACHINE_PRIMITIVE_ARGS_COUNT; ++i) {
                lm_node_cool_print(node->as.primitive.args[i]);
                if (i != _LMACHINE_PRIMITIVE_ARGS_COUNT - 1) printf(" ");
            }
            printf(")");
            break;
            
        case LM_NODE_THUNK:
            printf("([thunk] ");
            lm_node_cool_print(node->as.thunk);
            printf(")");
            break;
            
        default:
            printf("Unknown node type (%d)\n", node->tag);
            break;
    }
}


int main(int argc, char** argv) {
    config_t config;
    l_vm_state_t state = {0};
    lm_node_t* evaluated = NULL;
    bool success = false;
    process_arguments(&config, argc, argv);
    
    if (config.quote) {
        if (!parse_quote(&state, config.quote)) {
            fprintf(stderr, "Failed to parse quote\n");
            goto cleanup;
        }
    } else if (config.input_filepath) {
        if (!parse_file(&state, config.input_filepath)) {
            fprintf(stderr, "Failed to parse file\n");
            goto cleanup;
        }
    }

    if (!state.entry_point) {
        fprintf(stderr, "No entry point(`main`) found\n");
        goto cleanup;
    }
    evaluated = lm_evaluate(state.entry_point); //  `state.entry_point` destroyed
    if (!config.no_print_resuk) {
        lm_node_cool_print(evaluated);
        printf("\n");
    }
    
    
    success = true;
cleanup:
    if (evaluated) lm_destroy_node(evaluated);

    if (!success) exit(EXIT_FAILURE);
    return 0;
}
