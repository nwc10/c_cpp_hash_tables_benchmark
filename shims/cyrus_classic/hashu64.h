/* +++Date last modified: 05-Jul-1997 */
#ifndef __CYRUS_HASHU64_H__
#define __CYRUS_HASHU64_H__

#include <stddef.h>           /* For size_t     */

#define HASHU64_TABLE_INITIALIZER {0, NULL, NULL}

/*
** A hash table consists of an array of these buckets.  Each bucket
** holds a copy of the key, a pointer to the data associated with the
** key, and a pointer to the next bucket that collided with this one,
** if there was one.
*/

typedef struct bucketu64 {
    uint64_t key;
    void *data;
    struct bucketu64 *next;
} bucketu64;

/*
** This is what you actually declare an instance of to create a table.
** You then call 'construct_table' with the address of this structure,
** and a guess at the size of the table.  Note that more nodes than this
** can be inserted in the table, but performance degrades as this
** happens.  Performance should still be quite adequate until 2 or 3
** times as many nodes have been inserted as the table was created with.
*/

typedef struct hashu64_table {
    size_t size;
    bucketu64 **table;
    struct mpool *pool;
} hashu64_table;

/*
** This is used to construct the table.  If it doesn't succeed, it sets
** the table's size to 0, and the pointer to the table to NULL.
*/

hashu64_table *construct_hashu64_table(hashu64_table *table, size_t size,
                                 int use_mpool);

/*
** Inserts a pointer to 'data' in the table, with a copy of 'key' as its
** key.  Note that this does NOT make a copy of the
** associated data.
*/

void *hashu64_insert(uint64_t key,void *data,hashu64_table *table);

/*
** Returns a pointer to the data associated with a key.  If the key has
** not been inserted in the table, returns NULL.
*/

// void *hashu64_lookup(uint64_t key,hashu64_table *table);

/*
** Deletes an entry from the table.  Returns a pointer to the data that
** was associated with the key so the calling code can dispose of it
** properly.
*/
/* Warning: use this function judiciously if you are using memory pools,
 * since it will leak memory until you get rid of the entire hash table */
void *hashu64_del(uint64_t key,hashu64_table *table);

/*
** Goes through a hash table and calls the function passed to it
** for each node that has been inserted.  The function is passed
** a pointer to the key, a pointer to the data associated
** with it and 'rock'.
*/

void hashu64_enumerate(hashu64_table *table,void (*func)(uint64_t ,void *,void *),
                    void *rock);


/* just count how many items are in the table */
size_t hashu64_count(hashu64_table *table);

/*
** Frees a hash table.  For each node that was inserted in the table,
** it calls the function whose address it was passed, with a pointer
** to the data that was in the table.  The function is expected to
** free the data.  Typical usage would be:
** free_table(&table, free);
** if the data placed in the table was dynamically allocated, or:
** free_table(&table, NULL);
** if not.  ( If the parameter passed is NULL, it knows not to call
** any function with the data. )
*/

void free_hashu64_table(hashu64_table *table, void (*func)(void *));

#endif /* __CYRUS_HASHU64_H__ */

/* BONUS */

typedef struct hashu64_iter {
    hashu64_table *table;
    size_t i;
    bucketu64 *peek;
    bucketu64 *curr;
} hashu64_iter;

EXPORTED void hashu64_iter_reset(hashu64_iter *iter)
{
    hashu64_table *table = iter->table;
    iter->curr = NULL;
    iter->peek = NULL;
    for (iter->i = 0; iter->i < table->size; iter->i++) {
        if ((iter->peek = table->table[iter->i])) {
            break;
        }
    }
}

EXPORTED hashu64_iter *hashu64_table_iter(hashu64_table *table)
{
    hashu64_iter *iter = (hashu64_iter *) xzmalloc(sizeof(struct hashu64_iter));
    iter->table = table;
    hashu64_iter_reset(iter);
    return iter;
}

EXPORTED int hashu64_iter_has_next(hashu64_iter *iter)
{
    return iter->peek != NULL;
}

EXPORTED void *hashu64_iter_next(hashu64_iter *iter)
{
    hashu64_table *table = iter->table;
    iter->curr = iter->peek;
    iter->peek = NULL;
    if (iter->curr == NULL)
        return NULL;
    else if (iter->curr->next)
        iter->peek = iter->curr->next;
    else if (iter->i < table->size) {
        for (iter->i = iter->i + 1; iter->i < table->size; iter->i++) {
            if ((iter->peek = table->table[iter->i])) {
                break;
            }
        }
    }
    return iter->curr->data; /* HACK */
}

EXPORTED uint64_t hashu64_iter_key(hashu64_iter *iter)
{
    return iter->curr->key;
}

EXPORTED void **hashu64_iter_val(hashu64_iter *iter)
{
    return &iter->curr->data;
}

EXPORTED void hashu64_iter_free(hashu64_iter **iterptr)
{
    if (iterptr) {
        free(*iterptr);
        *iterptr = NULL;
    }
}

/*
** public domain code by Jerry Coffin, with improvements by HenkJan Wolthuis.
**
** Tested with Visual C 1.0 and Borland C 3.1.
** Compiles without warnings, and seems like it should be pretty
** portable.
**
** Modified for use with libcyrus by Ken Murchison.
**  - prefixed functions with 'hash_' to avoid symbol clashing
**  - use xmalloc() and xstrdup()
**  - cleaned up free_hash_table(), doesn't use enumerate anymore
**  - added 'rock' to hash_enumerate()
**
** Further modified by Rob Siemborski.
**  - xmalloc can never return NULL, so don't worry about it
**  - sort the buckets for faster searching
**  - actually, we'll just use a memory pool for this sucker
**    (atleast, in the cases where it is advantageous to do so)
*/

/* Initialize the hashu64_table to the size asked for.  Allocates space
** for the correct number of pointers and sets them to NULL.  If it
** can't allocate sufficient memory, signals error by setting the size
** of the table to 0.
*/

static inline int u64cmp(uint64_t a, uint64_t b)
{
    return (a < b ? -1 : (a > b ? 1 : 0));
}

EXPORTED hashu64_table *construct_hashu64_table(hashu64_table *table, size_t size, int use_mpool)
{
      assert(table);
      assert(size);

      table->size  = size;

      /* Allocate the table -- different for using memory pools and not */
      if(use_mpool) {
          /* Allocate an initial memory pool for 32 byte keys + the hash table
           * + the buckets themselves */
          table->pool =
              new_mpool(size * (32 + sizeof(bucketu64*) + sizeof(bucketu64)));
          table->table =
              (bucketu64 **)mpool_malloc(table->pool,sizeof(bucketu64 *) * size);
      } else {
          table->pool = NULL;
          table->table = (bucketu64**)xmalloc(sizeof(bucketu64 *) * size);
      }

      /* Allocate the table and initialize it */
      memset(table->table, 0, sizeof(bucketu64 *) * size);

      return table;
}

/*
** Insert 'key' into hashu64 table.
** Returns a non-NULL pointer which is either the passed @data pointer
** or, if there was already an entry for @key, the old data pointer.
*/

EXPORTED void *hashu64_insert(uint64_t key, void *data, hashu64_table *table)
{
      unsigned val = key % table->size;
      bucketu64 *ptr, *newptr;
      bucketu64 **prev;

      /*
      ** NULL means this bucket hasn't been used yet.  We'll simply
      ** allocate space for our new bucket and put our data there, with
      ** the table pointing at it.
      */
      if (!((table->table)[val]))
      {
          if(table->pool) {
              (table->table)[val] =
                  (bucketu64 *)mpool_malloc(table->pool, sizeof(bucketu64));
              (table->table)[val] -> key = key;
          } else {
              (table->table)[val] = (bucketu64 *)xmalloc(sizeof(bucketu64));
              (table->table)[val] -> key = key;
          }
          (table->table)[val] -> next = NULL;
          (table->table)[val] -> data = data;
          return (table->table)[val] -> data;
      }

      /*
      ** This spot in the table is already in use.  See if the current string
      ** has already been inserted, and if so, increment its count.
      */
      for (prev = &((table->table)[val]), ptr=(table->table)[val];
           ptr;
           prev=&(ptr->next),ptr=ptr->next) {
          int cmpresult = u64cmp(key, ptr->key);
          if (!cmpresult) {
              /* Match! Replace this value and return the old */
              void *old_data;

              old_data = ptr->data;
              ptr -> data = data;
              return old_data;
          } else if (cmpresult < 0) {
              /* The new key is smaller than the current key--
               * insert a node and return this data */
              if(table->pool) {
                  newptr = (bucketu64 *)mpool_malloc(table->pool, sizeof(bucketu64));
                  newptr->key = key;
              } else {
                  newptr = (bucketu64 *)xmalloc(sizeof(bucketu64));
                  newptr->key = key;
              }
              newptr->data = data;
              newptr->next = ptr;
              *prev = newptr;
              return data;
          }
      }

      /*
      ** This key is the largest one so far.  Add it to the end
      ** of the list (*prev should be correct)
      */
      if(table->pool) {
          newptr=(bucketu64 *)mpool_malloc(table->pool,sizeof(bucketu64));
          newptr->key = key;
      } else {
          newptr=(bucketu64 *)xmalloc(sizeof(bucketu64));
          newptr->key = key;
      }
      newptr->data = data;
      newptr->next = NULL;
      *prev = newptr;
      return data;
}


/*
** Look up a key and return the associated data.  Returns NULL if
** the key is not in the table.
*/

// NOTE changed to write to an iterator
EXPORTED void hashu64_lookup(uint64_t key, hashu64_table *table, hashu64_iter *result)
{
  result->curr = NULL;

      if (!table->size)
          return;

      unsigned val = key % table->size;
      bucketu64 *ptr;

      if (!(table->table)[val])
            return;

      for ( ptr = (table->table)[val];NULL != ptr; ptr = ptr->next )
      {
          int cmpresult = u64cmp(key, ptr->key);
          if (!cmpresult) {
            result->i = val;
            result->table = table;
            result->peek = ptr;
            /* This will "move" ->peek into ->curr */
            hashu64_iter_next(result);
            return;
          }
          else if(cmpresult < 0) /* key < ptr->key -- we passed it */
              return;
      }
      return;
}

/*
** Delete a key from the hashu64 table and return associated
** data, or NULL if not present.
*/
/* Warning: use this function judiciously if you are using memory pools,
 * since it will leak memory until you get rid of the entire hash table */
EXPORTED void *hashu64_del(uint64_t key, hashu64_table *table)
{
      unsigned val = key % table->size;
      void *data;
      bucketu64 *ptr, *last = NULL;

      if (!(table->table)[val])
            return NULL;

      /*
      ** Traverse the list, keeping track of the previous node in the list.
      ** When we find the node to delete, we set the previous node's next
      ** pointer to point to the node after ourself instead.  We then delete
      ** the key from the present node, and return a pointer to the data it
      ** contains.
      */

      for (last = NULL, ptr = (table->table)[val];
            NULL != ptr;
            last = ptr, ptr = ptr->next)
      {
          int cmpresult = u64cmp(key, ptr->key);
          if (!cmpresult)
          {
              if (last != NULL)
              {
                  data = ptr->data;
                  last->next = ptr->next;
                  if(!table->pool) {
                      free(ptr);
                  }
                  return data;
              }

              /*
              ** If 'last' still equals NULL, it means that we need to
              ** delete the first node in the list. This simply consists
              ** of putting our own 'next' pointer in the array holding
              ** the head of the list.  We then dispose of the current
              ** node as above.
              */

              else
              {
                  data = ptr->data;
                  (table->table)[val] = ptr->next;
                  if(!table->pool) {
                      free(ptr);
                  }
                  return data;
              }
          } else if (cmpresult < 0) {
              /* its not here! */
              return NULL;
          }
      }

      /*
      ** If we get here, it means we didn't find the item in the table.
      ** Signal this by returning NULL.
      */
      return NULL;
}

/*
** Frees a complete table by iterating over it and freeing each node.
** the second parameter is the address of a function it will call with a
** pointer to the data associated with each node.  This function is
** responsible for freeing the data, or doing whatever is needed with
** it.
*/

EXPORTED void free_hashu64_table(hashu64_table *table, void (*func)(void *))
{
      unsigned i;
      bucketu64 *ptr, *temp;

      /* If we have a function to free the data, apply it everywhere */
      /* We also need to traverse this anyway if we aren't using a memory
       * pool */
      if(func || !table->pool) {
          for (i=0;i<table->size; i++)
          {
              ptr = (table->table)[i];
              while (ptr)
              {
                  temp = ptr;
                  ptr = ptr->next;
                  if (func)
                      func(temp->data);
                  if(!table->pool) {
                      free(temp);
                  }
              }
          }
      }

      /* Free the main structures */
      if(table->pool) {
          free_mpool(table->pool);
          table->pool = NULL;
      } else {
          free(table->table);
      }
      table->table = NULL;
      table->size = 0;
}
