/* cx_dispatcher.h
 *
 * This file contains generated code. Do not modify!
 */

#ifndef cx_dispatcher_H
#define cx_dispatcher_H

#include "corto.h"
#include "cx__type.h"
#include "cx__api.h"
#include "cx__meta.h"

#ifdef __cplusplus
extern "C" {
#endif

/* virtual ::corto::lang::dispatcher::post(event e) */
void cx_dispatcher_post(cx_dispatcher _this, cx_event e);

/* ::corto::lang::dispatcher::post(event e) */
cx_void _cx_dispatcher_post_v(cx_dispatcher _this, cx_event e);
#define cx_dispatcher_post_v(_this, e) _cx_dispatcher_post_v(cx_dispatcher(_this), cx_event(e))

#ifdef __cplusplus
}
#endif
#endif

