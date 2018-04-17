/* This is a managed file. Do not delete this comment. */

#include <include/test.h>

int16_t test_SparseMount_construct(
    test_SparseMount this)
{
    this->super.policy.filterResults = true;
    this->super.policy.ownership = CORTO_REMOTE_SOURCE;
    return 0;
}

void test_SparseMount_add_object(
    test_SparseMount this,
    const char *id,
    const char *parent,
    const char *type,
    corto_resultMask flags)
{
    corto_result__assign(
        corto_resultList__append_alloc(this->items),
        id,
        NULL,
        parent,
        type,
        0,
        flags
    );
}

corto_resultIter test_SparseMount_on_query(
    test_SparseMount this,
    corto_query *query)
{
    return corto_ll_iterAlloc(this->items);
}
