/**
  * _sbrk implementation for newlib heap management.
  */

#include <errno.h>
#include <stdint.h>

extern uint8_t end;  /* Defined by the linker (symbol 'end') */

void *_sbrk(int incr)
{
    static uint8_t *heap_end = NULL;
    uint8_t *prev_heap_end;

    if (heap_end == NULL)
        heap_end = &end;

    prev_heap_end = heap_end;

    /* Check against stack pointer for collision */
    register uint8_t *stack_ptr __asm__("sp");
    if (heap_end + incr > stack_ptr)
    {
        errno = ENOMEM;
        return (void *)-1;
    }

    heap_end += incr;
    return (void *)prev_heap_end;
}
