#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>

#include "dtypes.h"
#include "dparser.h"
#include "dlex.h"
#include "deval.h"

#include "dbuiltin.h"

#define LOCAL_SIZE_FACTOR 10

#define DISABLE_PIPE_PROCESS() parser.is_pipe = 0
#define ENABLE_PIPE_PROCESS()  parser.is_pipe = 1

#define DISABLE_REFR_PROCESS() parser.is_refr = 0
#define ENABLE_REFR_PROCESS()  parser.is_refr = 1

parser_state parser;
parser_builder* current = NULL;

/**
 * Module List
 * 
 * used in the parser to determine 
 * whether functions have external references,
 * ignoring modules for optimization.
 */
const char *str_module_list[] = {
 "Math",
 "TCPServer",
 "List",
 "Frame",
 "Number",
 "Core",
 "Os",
 NULL,
};

static d_local_registers* create_local_registers() {
  d_local_registers* l = (d_local_registers*) malloc(sizeof(d_local_registers));
  l->length = 0;
  l->capacity = LOCAL_SIZE_FACTOR;
  l->vars = (char**) malloc(sizeof(char*) * l->capacity);
  return l;
}

static void init_locals(parser_state* psr) {
  psr->locals_length = 1;
  psr->locals_capacity = 10;
  psr->locals = (d_local_registers**) malloc(sizeof(d_local_registers*) * psr->locals_capacity);

  psr->locals[0] = create_local_registers();
}

/**
 * when a new function is created
 * we need the locals to manager definitions
 */
static void new_locals_register(parser_state* psr) {
  if (psr->locals_length >= psr->locals_capacity) {
    psr->locals_capacity += 4;
    psr->locals = (d_local_registers**) realloc(psr->locals, sizeof(d_local_registers*) * psr->locals_capacity);
  }

  psr->locals[psr->locals_length++] = create_local_registers();
}

static void put_local(parser_state* psr, char* name) {
  d_local_registers* lc = psr->locals[psr->locals_length -1];
  
  if (lc->length >= lc->capacity) {
    lc->capacity += LOCAL_SIZE_FACTOR;
    lc->vars = (char**) realloc(lc->vars, sizeof(char*) * lc->capacity);
  }

  size_t size = strlen(name);
  char* new_name = (char*) malloc(sizeof(char) * size + 1);
  strcpy(new_name, name);
  new_name[size] = '\0';

  lc->vars[lc->length++] = new_name;
}

static int get_local(parser_state* psr, char* name) {
  int i;
  d_local_registers* lc = psr->locals[psr->locals_length -1];
  for (i = lc->length - 1; i >= 0; i--) {
    if (strcmp(lc->vars[i], name) == 0) {
      return i;
    }
  }

  return -1;
}

static void remove_locals_registers(parser_state* psr) {
  int i;
  d_local_registers* lc = psr->locals[psr->locals_length -1];
  for (i = 0; i < lc->length; i++) {
    free(lc->vars[i]);
  }

  free(lc->vars);
  free(lc);
  psr->locals_length--;

  if (psr->locals_length == 0) {
    psr->locals_length++;
    psr->locals[0] = create_local_registers(psr);
  }

  DISABLE_PIPE_PROCESS();
  DISABLE_REFR_PROCESS();
}

static void init_parser(d_vm* vm) {
  vm->active_instr = vm->instructions;
  DISABLE_PIPE_PROCESS();
  init_locals(&parser);
}

static operation_line op_lines[] = {
  make_op_line(DTK_PAR_OPEN,  process_grouping,    process_call,   iCALL),
  make_op_line(DTK_PAR_CLOSE, NULL,                NULL,           iNONE),
  make_op_line(DTK_BKT_OPEN,  process_list,        NULL,           iCALL),
  make_op_line(DTK_BKT_CLOSE, NULL,                NULL,           iNONE),
  make_op_line(DTK_CBR_OPEN,  process_frame,       NULL,           iNONE),
  make_op_line(DTK_CBR_CLOSE, NULL,                NULL,           iNONE),
  make_op_line(DTK_DO,        process_do,          NULL,           iNONE),
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
  make_op_line(DTK_LL,        process_scalar,        process_binary, iCALL),
  make_op_line(DTK_GG,        NULL,                NULL,           iNONE),
  make_op_line(DTK_PIPE,      NULL,                process_pipe,   iTERM),
  make_op_line(DTK_AMP,       process_amper,       NULL,           iNONE),
  make_op_line(DTK_STRING,    process_string,      NULL,           iNONE),
  make_op_line(DTK_DSTRING,   process_dstring,     NULL,           iNONE),
  make_op_line(DTK_MSTRING,   process_mstring,     NULL,           iNONE),
  make_op_line(DTK_NUMBER,    process_number,      NULL,           iNONE),
  make_op_line(DTK_AND,       NULL,                process_and,    iAND),
  make_op_line(DTK_ELSE,      NULL,                NULL,           iNONE),
  make_op_line(DTK_FALSE,     literal_translation, NULL,           iNONE),
  make_op_line(DTK_NIL,       literal_translation, NULL,           iNONE),
  make_op_line(DTK_TRUE,      literal_translation, NULL,           iNONE),
  make_op_line(DTK_FUN,       process_function,    NULL,           iNONE),
  make_op_line(DTK_IF,        process_if,          NULL,           iNONE),
  make_op_line(DTK_OR,        NULL,                process_or,     iOR),
  make_op_line(DTK_ERROR,     NULL,                NULL,           iNONE),
  make_op_line(DTK_EOF,       NULL,                NULL,           iNONE),
  make_op_line(DTK_CONCAT,    NULL,                process_binary, iTERM),
  make_op_line(DTK_ID,        process_variable,    NULL,           iNONE),
  make_op_line(DTK_RETURN,    process_return,      NULL,           iNONE),
  make_op_line(DTK_IMPORT,    process_import,      NULL,           iNONE),
  make_op_line(DTK_EXPORT,    process_export,      NULL,           iNONE),
  make_op_line(DTK_LB,        process_line_break,  NULL,           iNONE),
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
    drax_value* f_before = &vm->active_instr->values[0];
    drax_value* new_val;

    vm->active_instr->instr_size = vm->active_instr->instr_size + MAX_INSTRUCTIONS;
    new_val = (drax_value*) realloc(vm->active_instr->values, sizeof(drax_value) * vm->active_instr->instr_size);
    vm->active_instr->lines = (int*) realloc(vm->active_instr->lines, sizeof(int) * vm->active_instr->instr_size);

    if (NULL == new_val) {
      FATAL_CURR("Failed to expand instruction block");
      return;
    }

    /**
     * Update external ref references after realloc
     * 
     * This change will affect all nested lambdas, 
     * as they share references.
     */
    drax_value* f_after = &new_val[0];
    int offset = f_after - f_before;
    drax_value* f_before_end = &vm->active_instr->values[vm->active_instr->instr_count -2];

    if (offset != 0)  {
      int i;
      for (i = 0; i < vm->active_instr->extrn_ref_count; i++) {

        /**
         * checks if the value belongs to the 
         * changed block.
         */
        if (
          vm->active_instr->extrn_ref[i] > f_before &&
          vm->active_instr->extrn_ref[i] < f_before_end
        ) {
          drax_value* ite = (drax_value*) vm->active_instr->extrn_ref[i] + offset;
          vm->active_instr->extrn_ref[i] = ite;
        }

      }
    }

    vm->active_instr->values = new_val;
  }

  vm->active_instr->lines[vm->active_instr->instr_count -1] = parser.prev.line;
  vm->active_instr->values[vm->active_instr->instr_count -1] = o;
}

static void process_external_ref(d_vm* vm) {  
  if (vm->active_instr->extrn_ref_count >= vm->active_instr->extrn_ref_capacity) {
    vm->active_instr->extrn_ref_capacity += 4;
    vm->active_instr->extrn_ref = (drax_value**) realloc(vm->active_instr->extrn_ref, sizeof(drax_value*) * vm->active_instr->extrn_ref_capacity);
  }

  /**
   * records references to variables outside of lambda, but which 
   * are not global, to be resolved at factory time
   */
  drax_value* ip = &vm->active_instr->values[vm->active_instr->instr_count - 2];
  vm->active_instr->extrn_ref[vm->active_instr->extrn_ref_count++] = ip;
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

  if ((unsigned int) jump > UINT16_MAX) {
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

void process_pipe(d_vm* vm, bool v) {
  UNUSED(v);
  ENABLE_PIPE_PROCESS();
  
  dlex_types opt = parser.prev.type;
  operation_line* rule = GET_PRIORITY(opt);
  parse_priorities(vm, (priorities)(rule->priorities + 1));
}

void process_amper(d_vm* vm, bool v) {
  UNUSED(v);
  if (eq_and_next(DTK_PAR_OPEN)) {
    int _ispipe = parser.is_pipe;
    DISABLE_PIPE_PROCESS();

    if (parser.active_fun == NULL) {
      FATAL("call out of scope.");
    }

    char* name = (char*) malloc(strlen(parser.active_fun->name) * sizeof(char));
    strcpy(name, parser.active_fun->name);
    put_pair(vm, OP_GET_G_ID, (drax_value) name);

    drax_value arg_count = process_arguments(vm) + _ispipe;
    put_pair(vm, OP_D_CALL, arg_count);
    return;
  }

  FATAL("Expect '(' after &.");
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

void process_scalar(d_vm* vm, bool v) {
  UNUSED(v);

  if (eq_and_next(DTK_GG)) {
    put_const(vm, NUMBER_VAL(0));
    put_instruction(vm, OP_SCALAR);
    return;
  }

  double lc = 0;
  do {
    expression(vm);
    lc++;
  } while (eq_and_next(DTK_COMMA));

  process_token(DTK_GG, "Expect '>>' after elements.");
  put_const(vm, NUMBER_VAL(lc));
  put_instruction(vm, OP_SCALAR);
}

void process_frame(d_vm* vm, bool v) {
  UNUSED(v);

  if (eq_and_next(DTK_CBR_CLOSE)) {
    put_const(vm, NUMBER_VAL(0));
    put_instruction(vm, OP_FRAME);
    return;
  }

  double lc = 0;
  do {
    char* name;
    if(eq_and_next(DTK_ID)) {
      name = (char*) malloc(sizeof(char) * (parser.prev.length + 1));
      strncpy(name, parser.prev.first, parser.prev.length);
      name[parser.prev.length] = '\0';
    } else {

      if(
        eq_and_next(DTK_STRING) ||
        eq_and_next(DTK_DSTRING)
      ) {
        name = (char*) malloc(sizeof(char) * (parser.prev.length - 2));
        strncpy(name, parser.prev.first + 1, parser.prev.length - 2);
        name[parser.prev.length - 2] = '\0';
      } else {
        FATAL_CURR("Expect 'identifier' as an key.");
        return;
      }
    }
    
    put_pair(vm, OP_PUSH, (drax_value) name);
    process_token(DTK_COLON, "Expect ':' after elements.");
    expression(vm);
    lc += 2;
  } while (eq_and_next(DTK_COMMA) && parser.current.type != DTK_CBR_CLOSE);

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

void process_dstring(d_vm* vm, bool v) {
  UNUSED(v);
  put_pair(vm, OP_DSTR, DS_VAL(copy_dstring(vm, parser.prev.first + 1, parser.prev.length - 2)));
}

void process_mstring(d_vm* vm, bool v) {
  UNUSED(v);
  put_pair(vm, OP_DSTR, DS_VAL(copy_dstring(vm, parser.prev.first + 3, parser.prev.length - 6)));
}

void process_variable(d_vm* vm, bool v) {
  UNUSED(v);
  d_token ctk = parser.prev;
  int is_global = IS_GLOBAL_SCOPE(vm);

  char* name = (char*) malloc(sizeof(char) * (ctk.length + 1));
  strncpy(name, ctk.first, ctk.length);
  name[ctk.length] = '\0';

  if (parser.is_refr == 1 && get_current_token() == DTK_DIV) {
    /** disabled */
    DISABLE_REFR_PROCESS();
    return;
  }

  if (eq_and_next(DTK_EQ)) {
    expression(vm);
    put_pair(vm, is_global ? OP_SET_G_ID : OP_SET_L_ID, (drax_value) name);
    
    if (!is_global) {
      put_local(&parser, name);
      vm->active_instr->local_range++;
    }
    return;
  }

  if (is_global || get_local(&parser, name) == -1) {
    put_pair(vm, OP_GET_G_ID, (drax_value) name);

    bool is_module = false;
    int i;
    for (i = 0; str_module_list[i] != NULL; i++) {
      if (strcmp(str_module_list[i], name) == 0) {
        is_module = true;
        break;
      }
    }

    if (!is_module) {
      process_external_ref(vm);
    }
    return;
  }

  put_pair(vm, OP_GET_L_ID, (drax_value) name);
}

void process_import(d_vm* vm, bool v) {
  UNUSED(v);
  process_token(DTK_PAR_OPEN, "Expect '(' after import.");
  expression(vm);
  process_token(DTK_PAR_CLOSE, "Expect ')' before expression on import.");
  put_instruction(vm, OP_IMPORT);
}

void process_return(d_vm* vm, bool v) {
  UNUSED(v);
  expression(vm);
  put_instruction(vm, (drax_value) OP_RETURN);
}

void process_export(d_vm* vm, bool v) {
  UNUSED(v);
  process_token(DTK_PAR_OPEN, "Expect '(' after export.");
  create_function(vm, true, true);
  process_token(DTK_PAR_CLOSE, "Expect ')' before expression on export.");
  put_pair(vm, OP_D_CALL, 0);
  put_instruction(vm, (drax_value) OP_EXPORT);
}

void process_line_break(d_vm* vm, bool v) {
  UNUSED(v);
  UNUSED(vm);
  get_next_token();
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

    if (infix_rule == NULL) {
      FATAL("Expect expression.");
      return;
    }

    infix_rule(vm, v);
  }

  if (v && eq_and_next(DTK_EQ)) {
    FATAL("Invalid assignment target.");
  }
}

static void expression(d_vm* vm) {
  parse_priorities(vm, iASSIGNMENT);
}

static void expression_with_lb(d_vm* vm) {
  int cl = parser.current.line;
    
  expression(vm);
  
  if (
    cl == parser.current.line &&
    get_current_token() != DTK_EOF
  ) {
    FATAL("unspected expression.");
  }
}

static void block(d_vm* vm) {
  while ((get_current_token() != DTK_END) && (get_current_token() != DTK_EOF)) {
    int cl = parser.current.line;
    
    expression(vm);
    
    if (
      cl == parser.current.line &&
      get_current_token() != DTK_EOF &&
      get_current_token() != DTK_END
    ) {
      FATAL("unspected expression.");
    }
  }

  process_token(DTK_END, "Expect 'end' after block.");
}

drax_function* create_function(d_vm* vm, bool is_internal, bool is_single_line) {
  int is_global = IS_GLOBAL_SCOPE(vm);
  const int max_arity = 255;
  int i;

  d_instructions* gi = vm->active_instr;
  drax_function* f = new_function(vm);
  parser.active_fun = f;
  f->instructions->file = vm->active_instr->file;
  vm->active_instr = f->instructions;
  new_locals_register(&parser);

  bool is_anonymous = (parser.current.type == DTK_PAR_OPEN) || is_internal;

  if (!is_anonymous) {
    get_next_token();
    d_token ctk = parser.prev;
    f->name = (char*) malloc(sizeof(char) * (ctk.length + 1));
    strncpy(f->name, ctk.first, ctk.length);
    f->name[ctk.length] = '\0';
  }

  if (!is_anonymous && !is_global) {
    FATAL_CURR("syntax error");
    printf(
    "\n  named functions are only accepted in the global scope.\n\n"
    "  you can proceed by changing this:\n"
    "    fun %s(...) do ... end\n"
    "  to:\n    %s = fun(...) do ... end\n\n",
    f->name, f->name);
  }

  if (!is_internal) {
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

        for(i = 0; i < f->arity -1; i++) {
          if(strcmp(s->chars, stack_args[i]) == 0) {
            FATAL("duplicate argument in function definition");
          }
        }

        char* ts = (char*) malloc(sizeof(char) * s->length + 1);
        strcpy(ts, s->chars);
        stack_args[f->arity -1] = ts;
      } while (eq_and_next(DTK_COMMA));
    }

    for (i = f->arity; i > 0 ; i--) {
      size_t sz = strlen(stack_args[i - 1]);
      char* s = (char*) malloc(sizeof(char) * (sz + 1));
      strcpy(s, stack_args[i - 1]);
      put_pair(vm, OP_SET_L_ID, (drax_value) s);
      put_local(&parser, s);
    }
    vm->active_instr->local_range = f->arity;

    for (i = 0; i < f->arity ; i++) {
      if (NULL != stack_args[i]) {
        free(stack_args[i]);
      }
    }

    free(stack_args);

    process_token(DTK_PAR_CLOSE, "Expect ')' after parameters.");

    if (!is_anonymous) {
      process_token(DTK_DO, "Expect 'do' before function body.");
    }
  }

  if (is_single_line) {
    expression(vm);
  } else {
    block(vm);
  }
  put_instruction(vm, OP_RETURN);
  vm->active_instr = gi;
  remove_locals_registers(&parser);

  /**
   * Copy the external references
   */
  if (f->instructions->extrn_ref_count > 0) {
    int new_cap = f->instructions->extrn_ref_count + vm->active_instr->extrn_ref_capacity;
    if (new_cap > vm->active_instr->extrn_ref_capacity) {
      vm->active_instr->extrn_ref = (drax_value**) realloc(
        vm->active_instr->extrn_ref,
        sizeof(drax_value*) * (new_cap)
      );

      vm->active_instr->extrn_ref_capacity = new_cap;
    }

    /**
     * If the variable is not present in the
     * local definitions, it means it is an 
     * external reference.
     */
    for (i = 0; i < f->instructions->extrn_ref_count; i++) {
      drax_value* ip = f->instructions->extrn_ref[i];
      drax_value gv = *(ip + 1);

      char* k = (char*) gv;
      if (get_local(&parser, k) == -1) {
        vm->active_instr->extrn_ref[vm->active_instr->extrn_ref_count++] = ip;
      }
    }
  }
  put_pair(vm, OP_FUN, DS_VAL(f));
  
  /**
   * used to identify if the lambda factory 
   * has external references.
   */
  put_instruction(vm, f->instructions->extrn_ref_count > 0 ? DRAX_TRUE_VAL : DRAX_FALSE_VAL);
  parser.active_fun = NULL;
  return f;
}

void process_function(d_vm* vm, bool v) {
  UNUSED(v);
  int is_global = IS_GLOBAL_SCOPE(vm);
  drax_function* f = create_function(vm, false, false);

  if (f->name != NULL) {
    /**
     * If is not anonymous
     */
    char* n = (char*) malloc(strlen(f->name) + 1);
    strcpy(n, f->name);
    put_pair(vm, is_global ? OP_SET_G_ID : OP_SET_L_ID, (drax_value) n);
  }
}

drax_value process_arguments(d_vm* vm) {
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
  int _ispipe = parser.is_pipe;
  DISABLE_PIPE_PROCESS();
  drax_value arg_count = process_arguments(vm) + _ispipe;
  put_pair(vm, _ispipe ? OP_D_CALL_P : OP_D_CALL, arg_count);
}

void process_dot(d_vm* vm, bool v) {
  UNUSED(v);
  process_token(DTK_ID, "Expect 'identifier' after '.'.");

  char* k = malloc(sizeof(char) * parser.prev.length + 1);
  memcpy(k, parser.prev.first, parser.prev.length);
  k[parser.prev.length] = '\0';

  if (eq_and_next(DTK_EQ)) {
    expression(vm);
    put_pair(vm, OP_PUSH, (drax_value) k);
    put_instruction(vm, OP_SET_I_ID);
  } else {
    put_pair(vm, OP_PUSH, (drax_value) k);
    put_instruction(vm, OP_GET_I_ID);
  }
}

void process_do(d_vm* vm, bool v) {
  UNUSED(v);
  while ((get_current_token() != DTK_END) && (get_current_token() != DTK_EOF)) {
    int cl = parser.current.line;
    
    expression(vm);
    
    if (
      cl == parser.current.line &&
      get_current_token() != DTK_EOF &&
      get_current_token() != DTK_END
    ) {
      FATAL("unspected expression.");
    }
  }
}

void process_if(d_vm* vm, bool v) {
  UNUSED(v);
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
    expression(vm);
  }

  int else_jump = put_jmp(vm, OP_JMP);

  patch_jump(vm, then_jump);
  put_instruction(vm, OP_POP);

  if (eq_and_next(DTK_ELSE)) {
    while (
      (get_current_token() != DTK_EOF) &&
      (get_current_token() != DTK_END))
    {
      expression(vm);
    }
  }

  patch_jump(vm, else_jump);

  process_token(DTK_END, "Expect 'end' after if definition.");
}

static int get_realpath(const char* path, char* resolved_path) {
  #ifdef _WIN32
      if (GetFullPathName(path, _PC_PATH_MAX, resolved_path, NULL) == 0) {
        return 0;
    }

    return 1;
  #else
    return realpath(path, resolved_path);
  #endif
}

static long get_path_max(const char* path) {
  #ifdef _WIN32
      return _PC_PATH_MAX;
  #else
      return pathconf(path, _PC_PATH_MAX);
  #endif
}

int __build__(d_vm* vm, const char* input, char* path) {
  init_lexan(input);
  init_parser(vm);

  if (path != NULL) {
    long path_max = get_path_max(path);
    parser.file = (char*) malloc(sizeof(char) * path_max);
    if (get_realpath(path, parser.file) == 0) {
      fprintf(stderr, "fail to make full path: '%s'.\n", path);
    }
    vm->active_instr->file = (char*) malloc(sizeof(char) * strlen(parser.file) + 1);
    strcpy(vm->active_instr->file, parser.file);
  }

  parser.has_error = false;
  parser.panic_mode = false;
  parser.active_fun = NULL;

  get_next_token();

  while (get_current_token() != DTK_EOF) {
    expression_with_lb(vm);
    /*put_instruction(vm, OP_POP);*/
  }

  if (parser.has_error) {
    return 0;
  }
  remove_locals_registers(&parser);
  free(parser.locals);

  put_pair(vm, OP_EXIT, 0xff);
  return 1;
}

void dfatal(d_token* token, const char* message) {
  if (parser.panic_mode) return;
  parser.panic_mode = true;
  if (NULL != parser.file) {
    fprintf(stderr, D_COLOR_RED"Error, '%s:%d'\n"D_COLOR_RESET, parser.file, token->line);
  } else {
    fprintf(stderr, D_COLOR_RED"Error, line: %d\n"D_COLOR_RESET, token->line);
  }

  if (token->type == DTK_EOF) {
    fprintf(stderr, "  end of file");
  }

  fprintf(stderr, " '%s'\n", message);
  parser.has_error = true;
}
