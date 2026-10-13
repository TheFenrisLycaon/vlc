#include "mlqueryparams.hpp"

heimdall_ml_query_params_t MLQueryParams::toCQueryParams() const
{
    heimdall_ml_query_params_t params = heimdall_ml_query_params_create();
    if (!searchPatternUtf8.isNull())
        params.psz_pattern = searchPatternUtf8.constData();
    params.i_nbResults = nbResults;
    params.i_offset = offset;
    params.i_sort = sort;
    params.b_desc = desc;
    return params;
}
