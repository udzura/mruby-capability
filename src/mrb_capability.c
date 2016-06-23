/*
** mrb_capability - Linux Capability class for mruby
**
** Copyright (c) mod_mruby developers 2012-
**
** Permission is hereby granted, free of charge, to any person obtaining
** a copy of this software and associated documentation files (the
** "Software"), to deal in the Software without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Software, and to
** permit persons to whom the Software is furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be
** included in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**
** [ MIT license: http://www.opensource.org/licenses/mit-license.php ]
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <sys/capability.h>

#include "mruby.h"
#include "mruby/variable.h"
#include "mruby/data.h"
#include "mruby/string.h"
#include "mruby/array.h"
#include "mruby/error.h"

#define CAP_NUM 38
#define DONE    mrb_gc_arena_restore(mrb, 0);

typedef struct {
    pid_t pid;
    cap_t cap;
    cap_value_t capval[CAP_NUM];
} mrb_cap_context;

static void mrb_cap_context_free(mrb_state *mrb, void *p)
{
    mrb_cap_context *ctx = (mrb_cap_context *)p;
    //cap_free(ctx->cap);
}

static const struct mrb_data_type mrb_cap_context_type = {
    "mrb_cap_context", mrb_cap_context_free,
};

static mrb_cap_context *mrb_cap_get_context(mrb_state *mrb,  mrb_value self, const char *ctx_flag)
{
    mrb_cap_context *c;
    mrb_value context;

    context = mrb_iv_get(mrb, self, mrb_intern_cstr(mrb, ctx_flag));
    Data_Get_Struct(mrb, context, &mrb_cap_context_type, c);
    if (!c)
        mrb_raise(mrb, E_RUNTIME_ERROR, "get mrb_cap_context failed");

    return c;
}

mrb_value mrb_cap_init(mrb_state *mrb, mrb_value self)
{
    mrb_cap_context *cap_ctx = (mrb_cap_context *)mrb_malloc(mrb, sizeof(mrb_cap_context));

    prctl(PR_SET_KEEPCAPS, 1);
    cap_ctx->cap = cap_init();
    cap_ctx->pid = -1;

    mrb_iv_set(mrb
        , self
        , mrb_intern_cstr(mrb, "mrb_cap_context")
        , mrb_obj_value(Data_Wrap_Struct(mrb
            , mrb->object_class
            , &mrb_cap_context_type
            , (void *)cap_ctx)
        )
    );

    return self;
}

mrb_value mrb_cap_set(mrb_state *mrb, mrb_value self)
{
    mrb_cap_context *cap_ctx = mrb_cap_get_context(mrb, self, "mrb_cap_context");

    int i;
    mrb_value ary;
    mrb_int identify;

    mrb_get_args(mrb, "iA", &identify, &ary);
    struct RArray *a = mrb_ary_ptr(ary);
    int ncap = a->len;

    for (i = 0; i < ncap; i++) {
        cap_ctx->capval[i] = (cap_value_t)mrb_fixnum(a->ptr[i]);
    }

    cap_set_flag(cap_ctx->cap, identify, ncap, cap_ctx->capval, CAP_SET);
    if (cap_set_proc(cap_ctx->cap) != 0)
        mrb_raise(mrb, E_RUNTIME_ERROR, "cap_set_proc() failed on set");
    //cap_free(cap_ctx->cap);

    mrb_iv_set(mrb
        , self
        , mrb_intern_cstr(mrb, "mrb_cap_context")
        , mrb_obj_value(Data_Wrap_Struct(mrb
            , mrb->object_class
            , &mrb_cap_context_type
            , (void *)cap_ctx)
        )
    );

    return self;
}

mrb_value mrb_cap_get(mrb_state *mrb, mrb_value self)
{
    mrb_cap_context *cap_ctx = mrb_cap_get_context(mrb, self, "mrb_cap_context");

    cap_ctx->cap = cap_get_proc();
    if(cap_ctx->cap != NULL) {
        cap_ctx->pid = getpid();
    }

    mrb_iv_set(mrb
        , self
        , mrb_intern_cstr(mrb, "mrb_cap_context")
        , mrb_obj_value(Data_Wrap_Struct(mrb
            , mrb->object_class
            , &mrb_cap_context_type
            , (void *)cap_ctx)
        )
    );

    return self;
}

mrb_value mrb_cap_get_pid(mrb_state *mrb, mrb_value self)
{
    mrb_cap_context *cap_ctx = mrb_cap_get_context(mrb, self, "mrb_cap_context");

    pid_t pid;
    mrb_get_args(mrb, "i", &pid);

    cap_ctx->cap = cap_get_pid(pid);
    if(cap_ctx->cap != NULL) {
        cap_ctx->pid = pid;
    }

    mrb_iv_set(mrb
        , self
        , mrb_intern_cstr(mrb, "mrb_cap_context")
        , mrb_obj_value(Data_Wrap_Struct(mrb
            , mrb->object_class
            , &mrb_cap_context_type
            , (void *)cap_ctx)
        )
    );

    return self;
}

mrb_value mrb_cap_clear(mrb_state *mrb, mrb_value self)
{
    mrb_cap_context *cap_ctx = mrb_cap_get_context(mrb, self, "mrb_cap_context");

    int i;
    mrb_value ary;
    mrb_int identify;

    mrb_get_args(mrb, "iA", &identify, &ary);
    struct RArray *a = mrb_ary_ptr(ary);
    int ncap = a->len;

    for (i = 0; i < ncap; i++) {
        cap_ctx->capval[i] = (cap_value_t)mrb_fixnum(a->ptr[i]);
    }

    cap_set_flag(cap_ctx->cap, identify, ncap, cap_ctx->capval, CAP_CLEAR);
    if (cap_set_proc(cap_ctx->cap) != 0)
        mrb_raise(mrb, E_RUNTIME_ERROR, "cap_set_proc() failed on clear");
    //cap_free(cap_ctx->cap);

    mrb_iv_set(mrb
        , self
        , mrb_intern_cstr(mrb, "mrb_cap_context")
        , mrb_obj_value(Data_Wrap_Struct(mrb
            , mrb->object_class
            , &mrb_cap_context_type
            , (void *)cap_ctx)
        )
    );

    return self;
}

mrb_value mrb_cap_set_flag(mrb_state *mrb, mrb_value self)
{
    mrb_cap_context *cap_ctx = mrb_cap_get_context(mrb, self, "mrb_cap_context");

    int i;
    mrb_value ary;
    mrb_int identify;
    mrb_int state;

    mrb_get_args(mrb, "iAi", &identify, &ary, &state);
    struct RArray *a = mrb_ary_ptr(ary);
    int ncap = a->len;

    for (i = 0; i < ncap; i++) {
        cap_ctx->capval[i] = (cap_value_t)mrb_fixnum(a->ptr[i]);
    }

    cap_set_flag(cap_ctx->cap, identify, ncap, cap_ctx->capval, state);
    if (cap_set_proc(cap_ctx->cap) != 0)
        mrb_raise(mrb, E_RUNTIME_ERROR, "cap_set_proc() failed on set_flag");
    //cap_free(cap_ctx->cap);

    mrb_iv_set(mrb
        , self
        , mrb_intern_cstr(mrb, "mrb_cap_context")
        , mrb_obj_value(Data_Wrap_Struct(mrb
            , mrb->object_class
            , &mrb_cap_context_type
            , (void *)cap_ctx)
        )
    );

    return self;
}

mrb_value mrb_cap_free(mrb_state *mrb, mrb_value self)
{
    mrb_cap_context *cap_ctx = mrb_cap_get_context(mrb, self, "mrb_cap_context");
    cap_free(cap_ctx->cap);
    return self;
}

mrb_value mrb_cap_to_text(mrb_state *mrb, mrb_value self)
{
    mrb_cap_context *cap_ctx = mrb_cap_get_context(mrb, self, "mrb_cap_context");
    char *to_s = cap_to_text(cap_ctx->cap, NULL);
    if (to_s == NULL) {
        mrb_sys_fail(mrb, "failed to get txt from cap");
    }

    return mrb_str_new_cstr(mrb, to_s);
}

mrb_value mrb_cap_current_pid(mrb_state *mrb, mrb_value self)
{
    mrb_cap_context *cap_ctx = mrb_cap_get_context(mrb, self, "mrb_cap_context");
    return mrb_fixnum_value(cap_ctx->pid);
}

// test
mrb_value mrb_cap_setuid(mrb_state *mrb, mrb_value self)
{
    uid_t uid;
    mrb_int argv;

    mrb_get_args(mrb, "i", &argv);
    uid = (uid_t)argv;

    setuid(uid);

    return self;
}

mrb_value mrb_cap_setgid(mrb_state *mrb, mrb_value self)
{
    uid_t gid;
    mrb_int argv;

    mrb_get_args(mrb, "i", &argv);
    gid = (gid_t)argv;

    setgid(gid);

    return self;
}

mrb_value mrb_cap_getuid(mrb_state *mrb, mrb_value self)
{
    return mrb_fixnum_value((mrb_int)getuid());
}

mrb_value mrb_cap_getgid(mrb_state *mrb, mrb_value self)
{
    return mrb_fixnum_value((mrb_int)getgid());
}

static mrb_value mrb_cap_get_bound(mrb_state *mrb, mrb_value self)
{
    mrb_int cap;
    int ret;
    mrb_get_args(mrb, "i", &cap);

    ret = cap_get_bound((cap_value_t)cap);
    if(ret < 0){
        mrb_sys_fail(mrb, "cap_get_bound failed.");
    }

    return mrb_fixnum_value(ret);
}

static mrb_value mrb_cap_drop_bound(mrb_state *mrb, mrb_value self)
{
    mrb_int cap;
    int ret;
    mrb_get_args(mrb, "i", &cap);

    ret = cap_drop_bound((cap_value_t)cap);
    if(ret < 0){
        mrb_sys_fail(mrb, "cap_drop_bound failed.");
    }

    return mrb_fixnum_value(ret);
}

static mrb_value mrb_cap_is_supported(mrb_state *mrb, mrb_value self)
{
    mrb_int cap;
    mrb_get_args(mrb, "i", &cap);

    if CAP_IS_SUPPORTED((cap_value_t)cap) {
        return mrb_true_value();
    } else {
        return mrb_false_value();
    }
}

static mrb_value mrb_cap_from_name(mrb_state *mrb, mrb_value self)
{
    const char *cap_name;
    cap_value_t cap;
    int ret;
    mrb_get_args(mrb, "z", &cap_name);

    ret = cap_from_name(cap_name, &cap);
    if(ret < 0){
        mrb_sys_fail(mrb, "cap_from_name failed.");
    }

    return mrb_fixnum_value((mrb_int)cap);
}

void mrb_mruby_capability_gem_init(mrb_state *mrb)
{
    struct RClass *capability;

    capability = mrb_define_class(mrb, "Capability", mrb->object_class);

    mrb_define_method(mrb, capability, "initialize",    mrb_cap_init,       MRB_ARGS_NONE());
    mrb_define_method(mrb, capability, "get",           mrb_cap_get,        MRB_ARGS_NONE());
    mrb_define_method(mrb, capability, "get_proc",      mrb_cap_get,        MRB_ARGS_NONE());
    mrb_define_method(mrb, capability, "get_pid",       mrb_cap_get_pid,    MRB_ARGS_NONE());
    mrb_define_method(mrb, capability, "set",           mrb_cap_set,        MRB_ARGS_ANY());
    mrb_define_method(mrb, capability, "set_proc",      mrb_cap_set,        MRB_ARGS_ANY());
    mrb_define_method(mrb, capability, "clear",         mrb_cap_clear,      MRB_ARGS_ANY());
    mrb_define_method(mrb, capability, "unset",         mrb_cap_clear,      MRB_ARGS_ANY());
    mrb_define_method(mrb, capability, "set_flag",      mrb_cap_set_flag,   MRB_ARGS_ANY());
    mrb_define_method(mrb, capability, "free",          mrb_cap_free,       MRB_ARGS_NONE());
    mrb_define_method(mrb, capability, "to_text",       mrb_cap_to_text,    MRB_ARGS_NONE());
    mrb_define_method(mrb, capability, "pid",           mrb_cap_current_pid, MRB_ARGS_NONE());

    // class methods
    mrb_define_class_method(mrb, capability, "get_bound",  mrb_cap_get_bound,    MRB_ARGS_REQ(1));
    mrb_define_class_method(mrb, capability, "drop_bound", mrb_cap_drop_bound,   MRB_ARGS_REQ(1));
    mrb_define_class_method(mrb, capability, "supported?", mrb_cap_is_supported, MRB_ARGS_REQ(1));
    mrb_define_class_method(mrb, capability, "from_name",  mrb_cap_from_name,    MRB_ARGS_REQ(1));

    // test
    mrb_define_method(mrb, capability, "setuid",        mrb_cap_setuid,      MRB_ARGS_ANY());
    mrb_define_method(mrb, capability, "setgid",        mrb_cap_setgid,      MRB_ARGS_ANY());
    mrb_define_method(mrb, capability, "getuid",        mrb_cap_getuid,      MRB_ARGS_NONE());
    mrb_define_method(mrb, capability, "getgid",        mrb_cap_getgid,      MRB_ARGS_NONE());

    mrb_define_const(mrb, capability, "CAP_CLEAR",              mrb_fixnum_value(CAP_CLEAR));
    mrb_define_const(mrb, capability, "CAP_SET",                mrb_fixnum_value(CAP_SET));

    mrb_define_const(mrb, capability, "CAP_EFFECTIVE",          mrb_fixnum_value(CAP_EFFECTIVE));
    mrb_define_const(mrb, capability, "CAP_PERMITTED",          mrb_fixnum_value(CAP_PERMITTED));
    mrb_define_const(mrb, capability, "CAP_INHERITABLE",        mrb_fixnum_value(CAP_INHERITABLE));

    mrb_define_const(mrb, capability, "CAP_CHOWN",              mrb_fixnum_value(CAP_CHOWN));
    mrb_define_const(mrb, capability, "CAP_DAC_OVERRIDE",       mrb_fixnum_value(CAP_DAC_OVERRIDE));
    mrb_define_const(mrb, capability, "CAP_DAC_READ_SEARCH",    mrb_fixnum_value(CAP_DAC_READ_SEARCH));
    mrb_define_const(mrb, capability, "CAP_FOWNER",             mrb_fixnum_value(CAP_FOWNER));
    mrb_define_const(mrb, capability, "CAP_FSETID",             mrb_fixnum_value(CAP_FSETID));
    mrb_define_const(mrb, capability, "CAP_KILL",               mrb_fixnum_value(CAP_KILL));
    mrb_define_const(mrb, capability, "CAP_SETGID",             mrb_fixnum_value(CAP_SETGID));
    mrb_define_const(mrb, capability, "CAP_SETUID",             mrb_fixnum_value(CAP_SETUID));
    mrb_define_const(mrb, capability, "CAP_SETPCAP",            mrb_fixnum_value(CAP_SETPCAP));
    mrb_define_const(mrb, capability, "CAP_LINUX_IMMUTABLE",    mrb_fixnum_value(CAP_LINUX_IMMUTABLE));
    mrb_define_const(mrb, capability, "CAP_NET_BIND_SERVICE",   mrb_fixnum_value(CAP_NET_BIND_SERVICE));
    mrb_define_const(mrb, capability, "CAP_NET_BROADCAST",      mrb_fixnum_value(CAP_NET_BROADCAST));
    mrb_define_const(mrb, capability, "CAP_NET_ADMIN",          mrb_fixnum_value(CAP_NET_ADMIN));
    mrb_define_const(mrb, capability, "CAP_NET_RAW",            mrb_fixnum_value(CAP_NET_RAW));
    mrb_define_const(mrb, capability, "CAP_IPC_LOCK",           mrb_fixnum_value(CAP_IPC_LOCK));
    mrb_define_const(mrb, capability, "CAP_IPC_OWNER",          mrb_fixnum_value(CAP_IPC_OWNER));
    mrb_define_const(mrb, capability, "CAP_SYS_MODULE",         mrb_fixnum_value(CAP_SYS_MODULE));
    mrb_define_const(mrb, capability, "CAP_SYS_RAWIO",          mrb_fixnum_value(CAP_SYS_RAWIO));
    mrb_define_const(mrb, capability, "CAP_SYS_CHROOT",         mrb_fixnum_value(CAP_SYS_CHROOT));
    mrb_define_const(mrb, capability, "CAP_SYS_PTRACE",         mrb_fixnum_value(CAP_SYS_PTRACE));
    mrb_define_const(mrb, capability, "CAP_SYS_PACCT",          mrb_fixnum_value(CAP_SYS_PACCT));
    mrb_define_const(mrb, capability, "CAP_SYS_ADMIN",          mrb_fixnum_value(CAP_SYS_ADMIN));
    mrb_define_const(mrb, capability, "CAP_SYS_BOOT",           mrb_fixnum_value(CAP_SYS_BOOT));
    mrb_define_const(mrb, capability, "CAP_SYS_NICE",           mrb_fixnum_value(CAP_SYS_NICE));
    mrb_define_const(mrb, capability, "CAP_SYS_RESOURCE",       mrb_fixnum_value(CAP_SYS_RESOURCE));
    mrb_define_const(mrb, capability, "CAP_SYS_TIME",           mrb_fixnum_value(CAP_SYS_TIME));
    mrb_define_const(mrb, capability, "CAP_SYS_TTY_CONFIG",     mrb_fixnum_value(CAP_SYS_TTY_CONFIG));
    mrb_define_const(mrb, capability, "CAP_MKNOD",              mrb_fixnum_value(CAP_MKNOD));
    mrb_define_const(mrb, capability, "CAP_LEASE",              mrb_fixnum_value(CAP_LEASE));
    mrb_define_const(mrb, capability, "CAP_AUDIT_WRITE",        mrb_fixnum_value(CAP_AUDIT_WRITE));
    mrb_define_const(mrb, capability, "CAP_AUDIT_CONTROL",      mrb_fixnum_value(CAP_AUDIT_CONTROL));
    //mrb_define_const(mrb, capability, "CAP_SETFCAP",            mrb_fixnum_value(CAP_SETFCAP));
    //mrb_define_const(mrb, capability, "CAP_MAC_OVERRIDE",       mrb_fixnum_value(CAP_MAC_OVERRIDE));
    //mrb_define_const(mrb, capability, "CAP_MAC_ADMIN",          mrb_fixnum_value(CAP_MAC_ADMIN));
    //mrb_define_const(mrb, capability, "CAP_SYSLOG",             mrb_fixnum_value(CAP_SYSLOG));
    //mrb_define_const(mrb, capability, "CAP_WAKE_ALARM",         mrb_fixnum_value(CAP_WAKE_ALARM));
    //mrb_define_const(mrb, capability, "CAP_BLOCK_SUSPEND",      mrb_fixnum_value(CAP_BLOCK_SUSPEND));
    //mrb_define_const(mrb, capability, "CAP_COMPROMISE_KERNEL",  mrb_fixnum_value(CAP_COMPROMISE_KERNEL));

    DONE;
}

void mrb_mruby_capability_gem_final(mrb_state *mrb)
{
}
