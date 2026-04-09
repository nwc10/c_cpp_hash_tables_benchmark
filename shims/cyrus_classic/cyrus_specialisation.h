/* Need to fake up a C++ style iterator interface
   1) because find is expected to return an iterator
   2) because the shim "API" didn't consider that (real) iterators needed free

   Do this with a class. The first approach was:
   * Have a pointer to a (real) iterator returned by hashu64_table_iter
   * if it's non-NULL, it's a real iterator, and delegate to that
   * otherwise, track which key/value pair we just looked up and store it in
     the class
   * and throw an exception if anyone attempts to "iterate" the fake iterator

   However, turns out that our caller expects to be able to forward iterate
   from an arbitrary lookup (as part of the iteration tests) so it's plan B:

   We embed a real iterator directly into the class, and tweak the Cyrus
   lookup function to populate it. This isn't ideal, but we have the same
   amount of dynamic allocation as plan A, and whilst there's a bit more code
   for (pure) lookups, I believe that extra CPU cache misses will be
   unlikely, because it's just the "next" pointer, and possibly walking the
   hash top level array.
*/

#define CYRUS_SPECIALIZATION( blueprint, prefix, space )                \
                                                                        \
  template<> struct cyrus_classic< blueprint >                          \
  {                                                                     \
                                                                        \
    using table_type = space::prefix##_table *;                         \
                                                                        \
    class iterator {                                                    \
    public:                                                             \
      space::prefix##_iter true_iter;                                   \
                                                                        \
    iterator() {                                                        \
      true_iter.curr = NULL;                                            \
    }                                                                   \
                                                                        \
    bool is_valid() {                                                   \
      return true_iter.curr;                                            \
    }                                                                   \
                                                                        \
    blueprint::key_type &key() {                                        \
      if(!is_valid())                                                   \
        throw("Request for key on invalid iterator");                   \
      return true_iter.curr->key;                                       \
    }                                                                   \
                                                                        \
    blueprint::value_type &value() {                                    \
      if(!is_valid())                                                   \
        throw("Request for value on invalid iterator");                 \
      return *((blueprint::value_type *) &true_iter.curr->data);        \
    }                                                                   \
                                                                        \
    };                                                                  \
                                                                        \
                                                                        \
    static table_type create_table()                                    \
    {                                                                   \
      table_type table = (table_type) calloc(sizeof(*table), 1);        \
      return space::construct_##prefix##_table( table, KEY_COUNT / 2, 0 ); \
    }                                                                   \
                                                                        \
    static iterator find( table_type &table, const blueprint::key_type &key ) \
    {                                                                   \
      iterator result;                                                  \
      space::prefix##_lookup( key, table, &result.true_iter );          \
      return result;                                                    \
    }                                                                   \
                                                                        \
    static void insert( table_type &table, const blueprint::key_type &key ) \
    {                                                                   \
      space::prefix##_insert( key, (void *)blueprint::value_type(), table ); \
    }                                                                   \
                                                                        \
    static void erase( table_type &table, const blueprint::key_type &key ) \
    {                                                                   \
      space::prefix##_del( key, table );                                \
    }                                                                   \
                                                                        \
    static iterator begin_itr( table_type &table )                      \
    {                                                                   \
      iterator result;                                                  \
      result.true_iter.table = table;                                   \
      space::prefix##_iter_reset( &result.true_iter );                  \
      space::prefix##_iter_next( &result.true_iter );                   \
      return result;                                                    \
    }                                                                   \
                                                                        \
    static bool is_itr_valid( table_type &table, iterator &itr )        \
    {                                                                   \
      return itr.is_valid();                                            \
    }                                                                   \
                                                                        \
    static void increment_itr( table_type &table, iterator &itr )       \
    {                                                                   \
      space::prefix##_iter_next( &itr.true_iter );                      \
    }                                                                   \
                                                                        \
    static const blueprint::key_type &get_key_from_itr( table_type &table, iterator &itr ) \
    {                                                                   \
      return itr.key();                                                 \
    }                                                                   \
                                                                        \
    static const blueprint::value_type &get_value_from_itr( table_type &table, iterator &itr ) \
    {                                                                   \
      return itr.value();                                               \
    }                                                                   \
                                                                        \
    static void destroy_table( table_type &table )                      \
    {                                                                   \
      space::free_##prefix##_table( table, 0 );                         \
      free( table );                                                    \
    }                                                                   \
  };                                                                    \
