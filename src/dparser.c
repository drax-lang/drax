
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dtypes.h"
#include "dparser.h"
#include "dlex.h"

#define LOCAL_SIZE_FACTOR 30

parser_state parser;
parser_builder* current = NULL;

static void init_parser() {
  parser.locals = (d_local_registers*) malloc(sizeof(d_local_registers));
  parser.locals->length = 0;
  parser.locals->capacity = LOCAL_SIZE_FACTOR;
  parser.locals->vars = (char**) malloc(sizeof(char*) * parser.locals->capacity);
}

static operation_line op_lines[] = {
  make_op_line(DTK_PAR_OPEN,  process_grouping,    process_call,   iCALL),
  make_op_line(DTK_PAR_CLOSE, NULL,                NULL,           iNONE),
  make_op_line(DTK_BKT_OPEN,  process_list,        NULL,           iNONE),
  make_op_line(DTK_BKT_CLOSE, NULL,                NULL,           iNONE),
  make_op_line(DTK_DO,        NULL,                NULL,           iNONE),
  make_op_line(DTK_END,       NULL,                NULL,           iNONE),
  make_op_line(DTK_COMMA,     NULL,                NULL,           iNONE),
  make_op_line(DTK_DOT,       NULL,                NULL,           iNONE),
  make_op_line(DTK_SUB,       process_unary,       process_binary, iTERM),
  make_op_line(DTK_ADD,       NULL,                process_binary, iTERM),
  make_op_line(DTK_DIV,       NULL,                process_binary, iFACTOR),
  make_op_line(DTK_MUL,       NULL,                process_binary, iFACTOR),
  make_op_line(DTK_BNG,       process_unary,       NULL,           iNONE),
  make_op_line(DTK_BNG_EQ,    NULL,                process_binary, iEQUALITY),
  make_op_line(DTK_EQ,        NULL,                NULL,           iNONE),
  make_op_line(DTK_DEQ,       NULL,                process_binary, iEQUALITY),
  make_op_line(DTK_BG,        NULL,                process_binary, iDIFF),
  make_op_line(DTK_BE,        NULL,                process_binary, iDIFF),
  make_op_line(DTK_LS,        NULL,                process_binary, iDIFF),
  make_op_line(DTK_LE,        NULL,                process_binary, iDIFF),
  make_op_line(DTK_STRING,    process_string,      NULL,           iNONE),
  make_op_line(DTK_NUMBER,    process_number,      NULL,           iNONE),
  make_op_line(DTK_AND,       NULL,                process_and,    iAND),
  make_op_line(DTK_ELSE,      NULL,                NULL,           iNONE),
  make_op_line(DTK_FALSE,     literal_translation, NULL,           iNONE),
  make_op_line(DTK_NIL,       literal_translation, NULL,           iNONE),
  make_op_line(DTK_TRUE,      literal_translation, NULL,           iNONE),
  make_op_line(DTK_FUN,       NULL,                NULL,           iNONE),
  make_op_line(DTK_IF,        NULL,                NULL,           iNONE),
  make_op_line(DTK_OR,        NULL,                process_or,     iOR),
  make_op_line(DTK_PRINT,     NULL,                NULL,           iNONE),
  make_op_line(DTK_ERROR,     NULL,                NULL,           iNONE),
  make_op_line(DTK_EOF,       NULL,                NULL,           iNONE),
  make_op_line(DTK_CONCAT,    NULL,                process_binary, iTERM),
  make_op_line(DTK_ID,        process_variable,    NULL,           iNONE),
};

#define GET_PRIORITY(v) (&op_lines[v])

#define GET_CURRENT_BYTE() (&current->block->function->d_byte)

#define FATAL(v) dfatal(&parser.prev, v)

#define FATAL_CURR(v) dfatal(&parser.current, v)


/**
 * VM Helpers
*/

static void put_instruction(d_vm* vm, drax_value o) {
  // parser.prev.line
  vm->instructions->instr_count++;

  if (vm->instructions->instr_count >= vm->instructions->instr_size) {
    vm->instructions->instr_size = vm->instructions->instr_size + MAX_INSTRUCTIONS;
    vm->instructions = (d_instructions*) realloc(vm->instructions, sizeof(d_instructions));
  }

  vm->instructions->values[vm->instructions->instr_count -1] = o;
}

static void put_pair(d_vm* vm, d_op_code o, drax_value v) {
  put_instruction(vm, (drax_value) o);
  put_instruction(vm, v);
}

/* Lexer Helpers */

static void get_next_token() {
  parser.prev = parser.current;

  while(true) {
    parser.current = next_token();
    if (parser.current.type != DTK_ERROR) break;

    FATAL_CURR(parser.current.first);
  }
}

static void process_token(dlex_types type, const char* message) {
  if (parser.current.type == type) {
    get_next_token();
    return;
  }

  FATAL_CURR(message);
}

static drax_value get_current_token() {
  return parser.current.type;
}

static bool eq_and_next(dlex_types type) {
  if (parser.current.type == type) {
    get_next_token();
    return true;
  };

  return false;
}

/* Compiler */

static int put_jmp(d_vm* vm, drax_value instruction) {
  // put_instruction(vm, instruction);
  // put_instruction(vm, 0xff);
  // put_instruction(vm, 0xff);
  // return GET_CURRENT_BYTE()->count - 2;
}


static void put_const(d_vm* vm, drax_value value) {
  put_pair(vm, OP_CONST, value);
}

static void patch_jump(d_vm* vm, int offset) {
  // int jump = GET_CURRENT_BYTE()->count - offset - 2;

  // if (jump > UINT16_MAX) {
  //   FATAL("Too much code to jump over.");
  // }

  // GET_CURRENT_BYTE()->code[offset] = (jump >> 8) & 0xff;
  // GET_CURRENT_BYTE()->code[offset + 1] = jump & 0xff;
}

/**
 * Create a new scope
 * 
 */

static void expression(d_vm* vm);

static void process(d_vm* vm);

static void parse_priorities(d_vm* vm, priorities priorities);

static drax_value identifier_constant(d_vm* vm, d_token* name) {
  return DS_VAL(copy_dstring(vm, name->first, name->length));
}

static drax_value parse_variable(d_vm* vm, const char* e) {
  process_token(DTK_ID, e);
  return identifier_constant(vm, &parser.prev);
}

static drax_value process_arguments(d_vm* vm) {
  drax_value arg_count = 0;
  if (get_current_token() != DTK_PAR_CLOSE) {
    do {
      expression(vm);
      if (arg_count == 255) {
        FATAL("Can't have more than 255 arguments.");
      }
      arg_count++;
    } while (eq_and_next(DTK_COMMA));
  }
  process_token(DTK_PAR_CLOSE, "Expect ')' after arguments.");
  return arg_count;
}

void process_and(d_vm* vm, bool v) {
  // UNUSED(v);
  // int end = put_jmp(OP_JMF);

  // put_instruction(vm, OP_POP);
  // parse_priorities(iAND);

  // patch_jump(end);
}

void process_binary(d_vm* vm, bool v) {
  UNUSED(v);
  dlex_types opt = parser.prev.type;
  operation_line* rule = GET_PRIORITY(opt);
  parse_priorities(vm, (priorities)(rule->priorities + 1));

  switch (opt) {
    case DTK_BNG_EQ: put_pair(vm, OP_EQUAL, OP_NOT); break;
    case DTK_DEQ:    put_instruction(vm, OP_EQUAL); break;
    case DTK_BG:     put_instruction(vm, OP_GREATER); break;
    case DTK_BE:     put_pair(vm, OP_LESS, OP_NOT); break;
    case DTK_LS:     put_instruction(vm, OP_LESS); break;
    case DTK_LE:     put_pair(vm, OP_GREATER, OP_NOT); break;
    case DTK_ADD:    put_instruction(vm, OP_ADD); break;
    case DTK_SUB:    put_instruction(vm, OP_SUB); break;
    case DTK_MUL:    put_instruction(vm, OP_MUL); break;
    case DTK_DIV:    put_instruction(vm, OP_DIV); break;
    case DTK_CONCAT: put_instruction(vm, OP_CONCAT); break;
    default: return;
  }
}

void process_call(d_vm* vm, bool v) {
  // UNUSED(v);
  // drax_value arg_count = process_arguments();
  // PUT_PAIR_DBYTES(OP_CALL, arg_count);
}

void literal_translation(d_vm* vm, bool v) {
  // UNUSED(v);
  // switch (parser.prev.type) {
  //   case DTK_NIL:   put_instruction(vm, OP_NIL);   break;
  //   case DTK_FALSE: put_instruction(vm, OP_FALSE); break;
  //   case DTK_TRUE:  put_instruction(vm, OP_TRUE);  break;
  //   default: return;
  // }
}

void process_grouping(d_vm* vm, bool v) {
  // UNUSED(v);
  // expression();
  // process_token(DTK_PAR_CLOSE, "Expect ')' after expression.");
}

void process_list(d_vm* vm, bool v) {
  // UNUSED(v);
  // double lc = 0;
  // do {
  //   expression();
  //   lc++;
  // } while (eq_and_next(DTK_COMMA));

  // process_token(DTK_BKT_CLOSE, "Expect ']' after elements.");
  // put_const(NUMBER_VAL(lc));
  // put_instruction(vm, OP_LIST);
}

void process_number(d_vm* vm, bool v) {
  UNUSED(v);
  double value = strtod(parser.prev.first, NULL);

  put_pair(vm, OP_PUSH, num_to_draxvalue(value));
}

void process_or(d_vm* vm, bool v) {
  // UNUSED(v);
  // int elsj = put_jmp(OP_JMF);
  // int endj = put_jmp(OP_JMP);

  // patch_jump(elsj);
  // put_instruction(vm, OP_POP);

  // parse_priorities(iOR);
  // patch_jump(endj);
}

void process_string(d_vm* vm, bool v) {
  UNUSED(v);
  put_const(vm, DS_VAL(copy_dstring(vm, parser.prev.first + 1, parser.prev.length - 2)));
}

static bool var_is_local(char* name, int size) {
  for (int i = 0; i < parser.locals->length; i++) {
    if (strncmp(parser.locals->vars[i], name, size) == 0) {
      return true;
    }
  }
  return false;
}

static void add_new_local(char* name) {
  if (parser.locals->length >= parser.locals->capacity) {
    parser.locals->capacity = parser.locals->capacity + LOCAL_SIZE_FACTOR;
    parser.locals->vars = (char**) realloc(parser.locals->vars, sizeof(parser.locals->capacity));
  }

  parser.locals->length++;
  parser.locals->vars[parser.locals->length -1] = name;
}

void process_variable(d_vm* vm, bool v) {
  d_token ctk = parser.prev;

  // drax_value id = identifier_constant(vm, &ctk);

  if (eq_and_next(DTK_EQ)) {
      char* name = (char*) calloc(ctk.length, sizeof(char));
      strncpy(name, ctk.first, ctk.length);
      expression(vm);
      put_pair(vm, OP_VAR, (drax_value) name);
      return;
  }
}

void process_unary(d_vm* vm, bool v) {
  // UNUSED(v);
  // dlex_types operatorType = parser.prev.type;

  // parse_priorities(iUNARY);

  // switch (operatorType) {
  //   case DTK_BNG: put_instruction(vm, OP_NOT); break;
  //   case DTK_SUB: put_instruction(vm, OP_NEGATE); break;
  //   default: return;
  // }
}

/* end of processors functions */

static void parse_priorities(d_vm* vm, priorities p) {
  get_next_token();
  
  parser_callback p_call = GET_PRIORITY(parser.prev.type)->prefix;
  if (p_call == NULL) {
    FATAL("Expect expression.");
    return;
  }

  bool v = p <= iASSIGNMENT;
  p_call(vm, v);

  while (p <= GET_PRIORITY(parser.current.type)->priorities) {
    get_next_token();
    parser_callback infix_rule = GET_PRIORITY(parser.prev.type)->infix;

    infix_rule(vm, v);
  }

  if (v && eq_and_next(DTK_EQ)) {
    FATAL("Invalid assignment target.");
  }
}

static void expression(d_vm* vm) {
  parse_priorities(vm, iASSIGNMENT);
}

static void block(d_vm* vm) {
  while ((get_current_token() != DTK_END) && (get_current_token() != DTK_EOF)) {
    process(vm);
  }

  process_token(DTK_END, "Expect 'end' after block.");
}

static void function(d_vm* vm, scope_type type) {
//
}

static void fun_declaration(d_vm* vm) {
//
}

static void if_definition(d_vm* vm) {
//
}

static void print_definition(d_vm* vm) {
  process_token(DTK_PAR_OPEN, "Expect '(' after arguments.");
  expression(vm);
  put_instruction(vm, OP_PRINT);
  process_token(DTK_PAR_CLOSE, "Expect ')' after arguments.");
}

static void process(d_vm* vm) {
  switch (get_current_token()) {
    case DTK_FUN: {
      get_next_token();
      fun_declaration(vm);
      break;
    }

    case DTK_IF: {
      get_next_token();
      if_definition(vm);
      break;
    }

    case DTK_PRINT: {
      get_next_token();
      print_definition(vm);
      break;
    }

    case DTK_DO: {
      break;
    }

    default: {
      expression(vm);
      // put_instruction(vm, OP_POP);
      break;
    }
  }
}

void __build__(d_vm* vm, const char* input) {
  init_lexan(input);
  init_parser();

  parser.has_error = false;
  parser.panic_mode = false;

  get_next_token();

  while (get_current_token() != DTK_EOF) {
    process(vm);
  }

  if (parser.has_error) {
    exit(1);
  }

  put_pair(vm, OP_EXIT, 0xff);
}

void dfatal(d_token* token, const char* message) {
  if (parser.panic_mode) return;
  parser.panic_mode = true;
  fprintf(stderr, "Error, line %d:\n", token->line);

  if (token->type == DTK_EOF) {
    fprintf(stderr, "  end of file");
  }  else {
    fprintf(stderr, " '%.*s'", token->length, token->first);
  }

  fprintf(stderr, " %s\n", message);
  parser.has_error = true;
}
