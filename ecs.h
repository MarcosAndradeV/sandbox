#ifndef ECS_H_
#define ECS_H_
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef NOB_H_
#include "nob.h"
#endif
#define ecs_expand

#define Component(name, ...) \
    static size_t COMP_##name; \
    typedef struct __VA_ARGS__ name; \
    static name name##_components[100]; \
    void register_##name() { if(COMP_##name == 0) COMP_##name = 1 << component_type_iota(); }\
    name* get_##name(size_t id) { return &name##_components[id]; } \
    void add_##name(size_t id, name value) { \
        if(COMP_##name == 0) {nob_log(NOB_ERROR, "Forgot to register `%s` componet first", #name); abort();}\
        entities.items[id].mask |= COMP_##name; \
        name##_components[id] = value; \
    }

#define System(name) void name##_system
#define QueryByComponents(e, ...) \
    nob_da_foreach(Entity, e, &entities) if(has_components(e->id, __VA_ARGS__))
    // for (size_t id = 0; id < entities.count; id++) if(has_components(id, __VA_ARGS__))

// ----------------------
// ECS "registry"
// ----------------------
typedef struct {
    unsigned int mask; // bitmask dos componentes
    size_t id;
} Entity;

typedef struct {
    Entity *items;
    size_t capacity, count;
} Entities;

static Entities entities;

// ----------------------
// Helpers
// ----------------------

size_t create_entity();
bool has_components(size_t id, unsigned int mask) ;
static inline size_t component_type_iota();

#ifdef ECS_IMPLEMENTATION

size_t create_entity() {
    size_t id = entities.count;
    Entity e = {
        .id = id,
    };
    nob_da_append(&entities, e);
    return id;
}

bool has_components(size_t id, unsigned int mask) {
    return (entities.items[id].mask & mask) == mask;
}

static size_t component_type_iota() {
    static size_t id = 0;
    return id++;
}

#endif // ECS_IMPLEMENTATION

#endif // ECS_H_
