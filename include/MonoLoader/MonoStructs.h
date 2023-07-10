//
// Created by Aleudillonam on 5/12/2023.
//

#ifndef OJOIE_MONOSTRUCTS_H
#define OJOIE_MONOSTRUCTS_H

typedef struct MonoVTable MonoVTable;
typedef struct _MonoThreadsSync MonoThreadsSync;

struct _MonoObject {
    MonoVTable *vtable;
    MonoThreadsSync *synchronisation;
};

#endif//OJOIE_MONOSTRUCTS_H
