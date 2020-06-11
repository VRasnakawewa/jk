#ifndef STACK_H
#define STACK_H

#define ARRAYLEN(a) (sizeof(a)/sizeof(a[0]))

#define STACK(name, size) int name[(size)+1] 
#define STACK_INIT(s) s[0] = 0
#define STACK_PUSH(s, value) (s[++s[0]] = value)
#define STACK_POP(s) (s[s[0]--])
#define STACK_EMPTY(s) (s[0] == 0)
#define STACK_FULL(s) (s[0] == ARRAYLEN(s))

#endif
