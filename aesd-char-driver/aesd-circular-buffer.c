/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#include <asm-generic/errno-base.h>
#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#include <stdbool.h>
#endif

#include "aesd-circular-buffer.h"

long aesd_circular_buffer_offset_adjust(struct aesd_circular_buffer *buffer,
            size_t cmd_offset, size_t char_offset)
{
    int i = 0;
    int real_offset = 0;
    struct aesd_buffer_entry* entry;
    while (i != cmd_offset) {
        entry = buffer->entry[i];
        if (entry == 0){
            return -EINVAL;
        }
        real_offset += entry->size;
        i = (i + 1) % 10;
    };
    entry = buffer->entry[i];
    if (entry == 0){
        return -EINVAL;
    }
    if (entry->size < char_offset){
        return -EINVAL;
    }
    real_offset += char_offset;
    return real_offset;
}

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    /**
    * TODO: implement per description
    */
    
    int starting_entry_index = buffer->out_offs;
    int target_entry_index = starting_entry_index;
    struct aesd_buffer_entry* target_entry;
    int target_offset = char_offset;

    do {
        target_entry = buffer->entry[target_entry_index];
        if(target_entry == 0){
            return NULL;
        }
        if ( target_offset < target_entry->size){
            *entry_offset_byte_rtn = target_offset;
            return target_entry;
        }else {
            target_offset = target_offset - (target_entry->size);
        }
        target_entry_index = (target_entry_index + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    }while (target_entry_index != starting_entry_index);
    
    return NULL;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
struct aesd_buffer_entry * aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    /**
    * TODO: implement per description
    */
    int target_in_offs = buffer->in_offs;
    int next_in_offs = (buffer->in_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
    struct aesd_buffer_entry* last_entry = buffer->entry[target_in_offs];

    buffer->entry[target_in_offs] = (struct aesd_buffer_entry *)add_entry;
    buffer->in_offs = next_in_offs;
    buffer->size += add_entry->size;
    if (buffer->full){
        buffer->out_offs = (buffer->out_offs + 1) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        buffer->size -= last_entry->size;
        return last_entry;
    }

    if (next_in_offs < target_in_offs){
        buffer->full = true;
    }
    return NULL;
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}
