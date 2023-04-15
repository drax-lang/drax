/* drax Lang - 2022
 * Jean Carlos (jeantux)
 */

#ifndef __DPARSER
#define __DPARSER
 
#include "dlex.h"
#include "dvm.h"
#include "ddefs.h"

#define callback_table(n) void n(d_vm* vm, bool b);

#define make_op_line(i, l, m, r) [i] = {l, m, r}

typedef struct d_local_registers {
  char** vars;
  int length;
  int capacity;
} d_local_registers;

typedef struct parser_state {
  d_token current;
  d_token prev;
  d_local_registers* locals;
  bool has_error;
  bool panic_mode;
} parser_state;

typedef enum priorities {
  iNONE = 0,
  iASSIGNMENT,
  iOR, 
  iAND,
  iEQUALITY,
  iDIFF,
  iTERM,
  iFACTOR,
  iUNARY,
  iCALL,
  iPRIMARY
} priorities;

typedef void (*parser_callback) (d_vm* vm, bool v);

typedef struct {
  parser_callback prefix;
  parser_callback infix;
  priorities priorities;
} operation_line;

typedef enum scope_type {
  SCP_BLOCK,
  SCP_FUN,
  SCP_ROOT
} scope_type;

typedef struct parser_builder {
  struct parser_builder* owner;
  scope_type type;
  int curr_level;
} parser_builder;

int __build__(d_vm* vm, const char* input);

callback_table(process_grouping);
callback_table(process_list);
callback_table(process_call);
callback_table(process_unary);
callback_table(process_binary);
callback_table(process_variable);
callback_table(process_string);
callback_table(process_number);
callback_table(process_and);
callback_table(process_or);
callback_table(literal_translation);

void dfatal(d_token* token, const char* message);

#endif
