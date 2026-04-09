#ifndef EXPORTED
#  define EXPORTED
#endif

static inline void *xmalloc(size_t size)
{
  void *ret;

  ret = malloc(size);
  if (ret != NULL) return ret;

  abort();
  return 0; /*NOTREACHED*/
}

static inline void *xzmalloc(size_t size)
{
  void *ret = xmalloc(size);
  memset(ret, 0, size);
  return ret;
}

struct mpool *new_mpool(size_t size) {
  return NULL;
}

static inline void free_mpool(struct mpool *pool) {
}

static inline void *mpool_malloc(struct mpool *pool, size_t size) {
  return NULL;
}

static inline char *mpool_strdup(struct mpool *pool, const char *str) {
  return NULL;
}

static inline char *mpool_strndup(struct mpool *pool, const char *str, size_t n) {
  return NULL;
}
