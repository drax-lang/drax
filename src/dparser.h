/* drax Lang - 2022
 * Jean Carlos (jeantux)
 */

#ifndef __DPARSER
#define __DPARSER
 
#include "dlex.h"
#include "dvm.h"
#include "ddefs.h"

#ifndef UINT16_MAX
  #define UINT16_MAX 65535
#endif

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
  char* file;

  d_local_registers** locals;
  int locals_length;
  int locals_capacity;
  drax_function* active_fun;
  int is_refr;
  int is_pipe;
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

int __build__(d_vm* vm, const char* input, char* path);

drax_value process_arguments(d_vm* vm);

drax_function* create_function(d_vm* vm, bool is_internal, bool is_single_line);

callback_table(process_grouping);
callback_table(process_list);
callback_table(process_unary);
callback_table(process_binary);
callback_table(process_variable);
callback_table(process_string);
callback_table(process_dstring);
callback_table(process_mstring);
callback_table(process_number);
callback_table(process_and);
callback_table(process_or);
callback_table(process_import);
callback_table(process_export);
callback_table(process_frame);
callback_table(process_dot);
callback_table(process_function);
callback_table(literal_translation);
callback_table(process_amper);
callback_table(process_pipe);
callback_table(process_call);
callback_table(process_if);
callback_table(process_do);
callback_table(process_return);

void dfatal(d_token* token, const char* message);

#endif
