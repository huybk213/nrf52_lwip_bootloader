#include "gsm_context.h"

static gsm_ctx_t m_gsm_ctx;

gsm_ctx_t *gsm_ctx(void)
{
    return &m_gsm_ctx;
}
