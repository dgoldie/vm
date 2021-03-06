#include <stdio.h>
#include <assert.h>
#include "runtime.h"
#include "opcode.h"
#include "bytecode.h"

// Helpers to play with the stack
#define STACK_MAX      10
#define STACK_PUSH(I)  do {                             \
                          assert(sp-stack < STACK_MAX); \
                          *(++sp) = (I);                \
                          retain(*sp); } while(0)
#define STACK_POP()    (*sp--)


void run(void *literals[], byte instructions[]) {
  byte    *ip = instructions;      // instruction pointer
  
  Object  *stack[STACK_MAX];       // the famous stack
  Object **sp = stack;             // stack pointer
  
  Object  *locals[STACK_MAX] = {}; // where we store our local variables
  
  // Setup the runtime
  Object *self = Object_new();
  retain(self);
  
  // Start processing instructions
  while (1) {
    switch (*ip) {
      case CALL: {
        ip++; // advance to operand (message literal index)
        char *message = (char *)literals[*ip];
        ip++;
        int argc = *ip;
        assert(argc < 10); // HACK max 10 args
        Object *argv[10];
        int i;
        
        // Pop all args from the stack and compile as an array
        for (i = 0; i < argc; ++i) argv[i] = STACK_POP();
        Object *receiver = STACK_POP();
        
        Object *result = call(receiver, message, argv, argc);
        STACK_PUSH(result);

        // Release all objects
        release(receiver);
        for (i = 0; i < argc; ++i) release(argv[i]);
        
        break;
      }
      case PUSH_NUMBER:
        ip++; // advance to operand (literal index)
        STACK_PUSH(Number_new((long)literals[*ip]));
        break;
        
      case PUSH_STRING:
        ip++; // advance to operand (literal index)
        STACK_PUSH(String_new((char *)literals[*ip]));
        break;
        
      case PUSH_SELF:
        STACK_PUSH(self);
        break;
        
      case PUSH_NIL:
        STACK_PUSH(NilObject);
        break;
      
      case PUSH_BOOL:
        ip++; // advance to operand (0 = false, 1 = true)
        if (*ip == 0) {
          STACK_PUSH(FalseObject);
        } else {
          STACK_PUSH(TrueObject);
        }
        break;
        
      case GET_LOCAL:
        ip++; // advance to operand (local index)
        STACK_PUSH(locals[*ip]);
        break;
        
      case SET_LOCAL:
        ip++; // advance to operand (local index)
        locals[*ip] = STACK_POP();
        break;
        
      case ADD: {
        Object *a = STACK_POP();
        Object *b = STACK_POP();
        
        STACK_PUSH(Number_new(Number_value(a) + Number_value(b)));

        release(a);
        release(b);
        
        break;
      }
      case JUMP_UNLESS: {
        ip++; // advance to operand (offset)
        byte offset = *ip;
        Object *test = STACK_POP();
        
        if (!Object_is_true(test)) ip += offset;

        release(test);
        
        break;
      }
      case JUMP: {
        ip++; // advance to operand (offset)
        byte offset = *ip;
        
        ip += offset;
        
        break;
      }
      case RETURN:
        goto cleanup;
        
    }
    ip++;
  }

cleanup:
  release(self);
  int i;
  for(i = 0; i < 10; i++) if (locals[i]) release(locals[i]);
  while (sp > stack) release(STACK_POP());
}

int main (int argc, char const *argv[]) {
  void *literals[] = LITERALS;
  byte instructions[] = INSTRUCTIONS;
  
  init_runtime();
  run(literals, instructions);
  destroy_runtime();
  
  return 0;
}