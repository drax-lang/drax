
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dtypes.h"
#include "dparser.h"
#include "dlex.h"

#define LOCAL_SIZE_FACTOR 30

parser_state parser;
parser_builder* current = NULL;

static d_local_registers* new_locals() {
  d_local_registers* l = (d_local_registers*) malloc(sizeof(d_local_registers));
  l->length = 0;
  l->capacity = LOCAL_SIZE_FACTOR;
  l->vars = (char**) malloc(sizeof(char*) * l->capacity);
  return l;
}

static void reset_locals() {
  int i;
  for (i = 0; i < parser.locals->length; i++) {
    free(parser.locals->vars[i]);
  }

  free(parser.locals->vars);
  free(parser.locals);

  parser.locals = new_locals();
}

static void put_local(char* name) {
  if (parser.locals->length >= parser.locals->capacity) {
    parser.locals->capacity = parser.locals->capacity + LOCAL_SIZE_FACTOR;
    parser.locals->vars = (char**) realloc(parser.locals->vars, sizeof(char*) * parser.locals->capacity);
  }

  size_t size = strlen(name);
  char* new_name = (char*) malloc(sizeof(char) * size + 1);
  strcpy(new_name, name);
  new_name[size] = '\0';

  parser.locals->vars[parser.locals->length++] = new_name;
}

static int get_local(char* name) {
  int i;
  for (i = parser.locals->length - 1; i >= 0; i--) {
    if (strcmp(parser.locals->vars[i], name) == 0) {
      return i;
    }
  }

  return -1;
}

static void init_parser(d_vm* vm) {
  vm->active_instr = vm->instructions;

  parser.locals = new_locals();
}

static operation_line op_lines[] = {
  make_op_line(DTK_PAR_OPEN,  process_grouping,    process_call,   iCALL),
  make_op_line(DTK_PAR_CLOSE, NULL,                NULL,           iNONE),
  make_op_line(DTK_BKT_OPEN,  process_list,        process_index,  iCALL),
  make_op_line(DTK_BKT_CLOSE, NULL,                NULL,           iNONE),
  make_op_line(DTK_CBR_OPEN,  process_struct,      NULL,           iNONE),
  make_op_line(DTK_CBR_CLOSE, NULL,                NULL,           iNONE),
  make_op_line(DTK_DO,        NULL,                NULL,           iNONE),
  make_op_line(DTK_END,       NULL,                NULL,           iNONE),
  make_op_line(DTK_COMMA,     NULL,                NULL,           iNONE),
  make_op_line(DTK_DOT,       NULL,                process_dot,    iCALL),
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
  make_op_line(DTK_AMP,       process_amper,       NULL,           iTERM),
  make_op_line(DTK_STRING,    process_string,      NULL,           iNONE),
  make_op_line(DTK_NUMBER,    process_number,      NULL,           iNONE),
  make_op_line(DTK_AND,       NULL,                process_and,    iAND),
  make_op_line(DTK_ELSE,      NULL,                NULL,           iNONE),
  make_op_line(DTK_FALSE,     literal_translation, NULL,           iNONE),
  make_op_line(DTK_NIL,       literal_translation, NULL,           iNONE),
  make_op_line(DTK_TRUE,      literal_translation, NULL,           iNONE),
  make_op_line(DTK_LAMBDA,    process_lambda,      NULL,           iNONE),
  make_op_line(DTK_IF,        NULL,                NULL,           iNONE),
  make_op_line(DTK_OR,        NULL,                process_or,     iOR),
  make_op_line(DTK_PRINT,     NULL,                NULL,           iNONE),
  make_op_line(DTK_ERROR,     NULL,                NULL,           iNONE),
  make_op_line(DTK_EOF,       NULL,                NULL,           iNONE),
  make_op_line(DTK_CONCAT,    NULL,                process_binary, iTERM),
  make_op_line(DTK_ID,        process_variable,    NULL,           iNONE),
  make_op_line(DTK_IMPORT,    process_import,      NULL,           iNONE),
  make_op_line(DTK_EXPORT,    process_export,      NULL,           iNONE),
};

#define GET_PRIORITY(v) (&op_lines[v])

#define GET_CURRENT_BYTE() (&current->block->function->d_byte)

#define FATAL(v) dfatal(&parser.prev, v)

#define FATAL_CURR(v) dfatal(&parser.current, v)

#define IS_GLOBAL_SCOPE(v) (v->instructions == v->active_instr)

/**
 * VM Helpers
*/

#define GET_INSTRUCTION(vm) vm->active_instr

static void put_instruction(d_vm* vm, drax_value o) {
  vm->active_instr->instr_count++;

  if (vm->active_instr->instr_count >= vm->active_instr->instr_size) {
    vm->active_instr->instr_size = vm->active_instr->instr_size + MAX_INSTRUCTIONS;
    vm->active_instr->values = (drax_value*) realloc(vm->active_instr->values, sizeof(drax_value) * vm->active_instr->instr_size);
    vm->active_instr->lines = (int*) realloc(vm->active_instr->lines, sizeof(int) * vm->active_instr->instr_size);
  }

  vm->active_instr->lines[vm->active_instr->instr_count -1] = parser.prev.line;
  vm->active_instr->values[vm->active_instr->instr_count -1] = o;
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

static dlex_types get_current_token() {
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
  put_instruction(vm, instruction);
  put_instruction(vm, 0xff);
  put_instruction(vm, 0xff);
  return GET_INSTRUCTION(vm)->instr_count - 2;
}


static void put_const(d_vm* vm, drax_value value) {
  put_pair(vm, OP_CONST, value);
}

static void patch_jump(d_vm* vm, int offset) {
  int jump = GET_INSTRUCTION(vm)->instr_count - offset - 2;

  if (jump > UINT16_MAX) {
    FATAL("Too much code to jump over.");
  }

  GET_INSTRUCTION(vm)->values[offset] = (jump >> 8) & 0xff;
  GET_INSTRUCTION(vm)->values[offset + 1] = jump & 0xff;
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

void process_and(d_vm* vm, bool v) {
  UNUSED(v);
  int end = put_jmp(vm, OP_JMF);

  put_instruction(vm, OP_POP);
  parse_priorities(vm, iAND);

  patch_jump(vm, end);
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

void process_amper(d_vm* vm, bool v) {
  UNUSED(v);
  process_token(DTK_ID, "Expect 'identifier' after '&'.");

  d_token ctk = parser.prev;
  char* name = (char*) malloc(sizeof(char) * (ctk.length + 1));
  strncpy(name, ctk.first, ctk.length);
  name[ctk.length] = '\0';

  process_token(DTK_DIV, "Expect '/' after identifier.");
  process_token(DTK_NUMBER, "Expect 'number' after '/'.");
  process_number(vm, false);

  put_pair(vm, OP_GET_REF, (drax_value) name);
}

void literal_translation(d_vm* vm, bool v) {
  UNUSED(v);
  switch (parser.prev.type) {
    case DTK_NIL:   put_instruction(vm, OP_NIL);   break;
    case DTK_FALSE: put_instruction(vm, OP_FALSE); break;
    case DTK_TRUE:  put_instruction(vm, OP_TRUE);  break;
    default: return;
  }
}

void process_grouping(d_vm* vm, bool v) {
  UNUSED(v);
  expression(vm);
  process_token(DTK_PAR_CLOSE, "Expect ')' after expression.");
}

void process_index(d_vm* vm, bool v) {
  UNUSED(v);
  put_pair(vm, OP_PUSH, (drax_value) "get");
  expression(vm);
  process_token(DTK_BKT_CLOSE, "Expect ']' after index.");
  put_pair(vm, OP_CALL_I, 1);
}

void process_list(d_vm* vm, bool v) {
  UNUSED(v);

  if (eq_and_next(DTK_BKT_CLOSE)) {
    put_const(vm, NUMBER_VAL(0));
    put_instruction(vm, OP_LIST);
    return;
  }

  double lc = 0;
  do {
    expression(vm);
    lc++;
  } while (eq_and_next(DTK_COMMA));

  process_token(DTK_BKT_CLOSE, "Expect ']' after elements.");
  put_const(vm, NUMBER_VAL(lc));
  put_instruction(vm, OP_LIST);
}

void process_struct(d_vm* vm, bool v) {
  UNUSED(v);

  if (eq_and_next(DTK_CBR_CLOSE)) {
    put_const(vm, NUMBER_VAL(0));
    put_instruction(vm, OP_FRAME);
    return;
  }

  double lc = 0;
  do {
    process_token(DTK_ID, "Expect 'identifier' as an key.");
    
    char* name = (char*) malloc(sizeof(char) * (parser.prev.length + 1));
    strncpy(name, parser.prev.first, parser.prev.length);
    name[parser.prev.length] = '\0';

    put_pair(vm, OP_PUSH, (drax_value) name);
    process_token(DTK_COLON, "Expect ':' after elements.");
    expression(vm);
    lc += 2;
  } while (eq_and_next(DTK_COMMA));

  process_token(DTK_CBR_CLOSE, "Expect '}' after elements.");
  put_const(vm, NUMBER_VAL(lc));
  put_instruction(vm, OP_FRAME);
}

void process_number(d_vm* vm, bool v) {
  UNUSED(v);
  double value;
  long int lv;
  char* endptr = (char*) parser.prev.first + parser.prev.length;

  switch (parser.prev.num_type) {
    case DNT_DECIMAL:
      value = strtod(parser.prev.first, &endptr);
      break;

    case DNT_BIN:
      lv = strtol(parser.prev.first + 2, &endptr, 2);
      value = (double) lv;
      break;

    case DNT_OCT:
      lv = strtol(parser.prev.first + 2, &endptr, 8);
      value = (double) lv;
      break;
    
    case DNT_HEX:
      lv = strtol(parser.prev.first + 2, &endptr, 16);
      value = (double) lv;
      break;

    default:
      FATAL("Invalid number format.");
      break;
  }    

  put_pair(vm, OP_PUSH, num_to_draxvalue(value));
}

void process_or(d_vm* vm, bool v) {
  UNUSED(v);
  int elsj = put_jmp(vm, OP_JMF);
  int endj = put_jmp(vm, OP_JMP);

  patch_jump(vm, elsj);
  put_instruction(vm, OP_POP);

  parse_priorities(vm, iOR);
  patch_jump(vm, endj);
}

void process_unary(d_vm* vm, bool v) {
  UNUSED(v);
  dlex_types operatorType = parser.prev.type;

  parse_priorities(vm, iUNARY);

  switch (operatorType) {
    case DTK_BNG: put_instruction(vm, OP_NOT); break;
    case DTK_SUB: put_instruction(vm, OP_NEG); break;
    default: return;
  }
}

void process_string(d_vm* vm, bool v) {
  UNUSED(v);
  put_const(vm, DS_VAL(copy_dstring(vm, parser.prev.first + 1, parser.prev.length - 2)));
}

void process_variable(d_vm* vm, bool v) {
  UNUSED(v);
  d_token ctk = parser.prev;
  int is_global = IS_GLOBAL_SCOPE(vm);

  char* name = (char*) malloc(sizeof(char) * (ctk.length + 1));
  strncpy(name, ctk.first, ctk.length);
  name[ctk.length] = '\0';

  if (eq_and_next(DTK_EQ)) {
      expression(vm);      

      put_pair(vm, is_global ? OP_SET_G_ID : OP_SET_L_ID, (drax_value) name);
      
      if (!is_global) {
        put_local(name);
        vm->active_instr->local_range++;
      }
      return;
  }

  if (get_current_token() == DTK_PAR_OPEN) {
    put_pair(vm, OP_PUSH, (drax_value) name);
    return;
  }

  if (is_global || get_local(name) == -1) {
    put_pair(vm, OP_GET_G_ID, (drax_value) name);
    return;
  }

  put_pair(vm, OP_GET_L_ID, (drax_value) name);
}

void process_import(d_vm* vm, bool v) {
  UNUSED(vm);
  UNUSED(v);
}

void process_export(d_vm* vm, bool v) {
  UNUSED(vm);
  UNUSED(v);
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

static void fun_declaration(d_vm* vm, int is_anonymous) {
  const int max_arity = 255;
  d_instructions* gi = vm->active_instr;
  drax_function* f = new_function(vm);
  vm->active_instr = f->instructions;

  if (is_anonymous) {
    f->name = NULL;
  } else {
    get_next_token();
    d_token ctk = parser.prev;
    f->name = (char*) malloc(sizeof(char) * (ctk.length + 1));
    strncpy(f->name, ctk.first, ctk.length);
    f->name[ctk.length] = '\0';
  }

  process_token(DTK_PAR_OPEN, "Expect '(' after function name.");

  char** stack_args = (char**) malloc(sizeof(char*) * max_arity);

  if (get_current_token() != DTK_PAR_CLOSE) {
    do {
      f->arity++;
      if (f->arity > max_arity) {
        FATAL_CURR("Can't have more than 255 parameters.");
      }

      drax_value constant = parse_variable(vm, "Expect parameter name.");
      drax_string* s = CAST_STRING(constant);

      int i;
      for(i = 0; i < f->arity -1; i++) {
        if(strcmp(s->chars, stack_args[i]) == 0) {
          FATAL("duplicate argument in function definition");
        }
      }

      stack_args[f->arity -1] = s->chars;
    } while (eq_and_next(DTK_COMMA));
  }

  int i;
  for (i = f->arity; i > 0 ; i--) {
    size_t sz = strlen(stack_args[i - 1]);
    char* s = (char*) malloc(sizeof(char) * (sz + 1));
    strcpy(s, stack_args[i - 1]);
    s[sz] = '\0';
    put_pair(vm, OP_SET_L_ID, (drax_value) s);
    put_local(stack_args[i - 1]);
  }
  vm->active_instr->local_range = f->arity;

  free(stack_args);

  process_token(DTK_PAR_CLOSE, "Expect ')' after parameters.");

  if (!is_anonymous) {
    process_token(DTK_DO, "Expect 'do' before function body.");
  }

  block(vm);
  put_instruction(vm, OP_RETURN);
  vm->active_instr = gi;
  put_pair(vm, is_anonymous ? OP_AFUN : OP_FUN, DS_VAL(f));
  reset_locals();
}

void process_lambda(d_vm* vm, bool v) {
  UNUSED(v);
  fun_declaration(vm, 1);
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

void process_call(d_vm* vm, bool v) {
  UNUSED(v);
  int is_global = IS_GLOBAL_SCOPE(vm);
  drax_value arg_count = process_arguments(vm);

  put_pair(vm, is_global ? OP_CALL_G : OP_CALL_L, arg_count);
}

void process_dot(d_vm* vm, bool v) {
  UNUSED(v);
  process_token(DTK_ID, "Expect 'identifier' after '.'.");

  char* k = malloc(sizeof(char) * parser.prev.length + 1);
  memcpy(k, parser.prev.first, parser.prev.length);
  k[parser.prev.length] = '\0';
  put_pair(vm, OP_PUSH, (drax_value) k);

  if (eq_and_next(DTK_PAR_OPEN)) {
    drax_value arg_count = process_arguments(vm);
    put_pair(vm, OP_CALL_I, arg_count);
  } else if (eq_and_next(DTK_EQ)) {
    expression(vm);
    put_instruction(vm, OP_SET_I_ID);
  } else {
    put_instruction(vm, OP_GET_I_ID);
  }
}

static void if_definition(d_vm* vm) {
  process_token(DTK_PAR_OPEN, "Expect '(' after 'if'.");
  expression(vm);
  process_token(DTK_PAR_CLOSE, "Expect ')' after condition.");
  process_token(DTK_DO, "Expect 'do' after condition.");

  int then_jump = put_jmp(vm, OP_JMF);
  put_instruction(vm, OP_POP);

  while (
    (get_current_token() != DTK_EOF) &&
    (get_current_token() != DTK_END) &&
    (get_current_token() != DTK_ELSE)) 
  {
    process(vm);
  }

  int else_jump = put_jmp(vm, OP_JMP);

  patch_jump(vm, then_jump);
  put_instruction(vm, OP_POP);

  if (eq_and_next(DTK_ELSE)) {
    while (
      (get_current_token() != DTK_EOF) &&
      (get_current_token() != DTK_END))
    {
      process(vm);
    }
  }

  patch_jump(vm, else_jump);

  process_token(DTK_END, "Expect 'end' after if definition.");
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
      fun_declaration(vm, 0);
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
      get_next_token();

      while ((get_current_token() != DTK_END) && (get_current_token() != DTK_EOF)) {
        process(vm);
      }

      break;
    }

    default: {
      expression(vm);
      break;
    }
  }
}

int __build__(d_vm* vm, const char* input) {
  init_lexan(input);
  init_parser(vm);

  parser.has_error = false;
  parser.panic_mode = false;

  get_next_token();

  while (get_current_token() != DTK_EOF) {
    process(vm);
  }

  if (parser.has_error) {
    return 0;
  }

  put_pair(vm, OP_EXIT, 0xff);
  return 1;
}

void dfatal(d_token* token, const char* message) {
  if (parser.panic_mode) return;
  parser.panic_mode = true;
  fprintf(stderr, "Error, line %d:\n", token->line);

  if (token->type == DTK_EOF) {
    fprintf(stderr, "  end of file");
  }

  fprintf(stderr, " '%s'\n", message);
  parser.has_error = true;
}
