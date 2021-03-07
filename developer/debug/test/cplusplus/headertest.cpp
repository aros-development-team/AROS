/*
    Copyright (C) 1995-2021, The AROS Development Team. All rights reserved.
*/

// purpose of this test is to find out if our headers are C++ compatible

// #include <acpica/acapps.h>
// #include <acpica/acbuffer.h>
// #include <acpica/acclib.h>
// #include <acpica/accommon.h>
// #include <acpica/acconfig.h>
// #include <acpica/acconvert.h>
// #include <acpica/acdebug.h>
// #include <acpica/acdisasm.h>
// #include <acpica/acdispat.h>
// #include <acpica/acevents.h>
// #include <acpica/acexcep.h>
// #include <acpica/acglobal.h>
// #include <acpica/achware.h>
// #include <acpica/acinterp.h>
// #include <acpica/aclocal.h>
// #include <acpica/acmacros.h>
// #include <acpica/acnames.h>
// #include <acpica/acnamesp.h>
// #include <acpica/acobject.h>
// #include <acpica/acopcode.h>
// #include <acpica/acoutput.h>
// #include <acpica/acparser.h>
// #include <acpica/acpi.h>
// #include <acpica/acpiosxf.h>
// #include <acpica/acpixf.h>
// #include <acpica/acpredef.h>
// #include <acpica/acresrc.h>
// #include <acpica/acrestyp.h>
// #include <acpica/acstruct.h>
// #include <acpica/actables.h>
// #include <acpica/actbinfo.h>
// #include <acpica/actbl1.h>
// #include <acpica/actbl2.h>
// #include <acpica/actbl3.h>
// #include <acpica/actbl.h>
// #include <acpica/actypes.h>
// #include <acpica/acutils.h>
// #include <acpica/acuuid.h>
// #include <acpica/amlcode.h>
// #include <acpica/amlresrc.h>

#include <aros/64bit.h>

#ifdef __aarch64__
#include <aros/aarch64/atomic.h>
#include <aros/aarch64/atomic_v8.h>
#include <aros/aarch64/cpucontext.h>
#include <aros/aarch64/cpu.h>
#include <aros/aarch64/fenv.h>
#endif

#ifdef __arm__
#include <aros/arm/atomic.h>
#include <aros/arm/atomic_v6.h>
#include <aros/arm/atomic_v7.h>
#include <aros/arm/cpucontext.h>
#include <aros/arm/cpu.h>
#include <aros/arm/cpu-thumb2.h>

#include <aros/armeb/atomic.h>
#include <aros/armeb/atomic_v6.h>
#include <aros/armeb/atomic_v7.h>
#include <aros/armeb/cpucontext.h>
#include <aros/armeb/cpu.h>
#include <aros/armeb/cpu-thumb2.h>
#include <aros/armeb/fenv.h>
#include <aros/armeb/fenv_soft.h>
#include <aros/armeb/fenv_vfp.h>
#include <aros/armeb/genmodule.h>
#include <aros/armeb/genmodule-thumb2.h>

#include <aros/arm/fenv.h>
#include <aros/arm/fenv_soft.h>
#include <aros/arm/fenv_vfp.h>
#include <aros/arm/genmodule.h>
#include <aros/arm/genmodule-thumb2.h>
#endif

#include <aros/arosbase.h>
#include <aros/arossupportbase.h>
#include <aros/asmcall.h>
#include <aros/atomic.h>
#include <aros/autoinit.h>
#include <aros/bigendianio.h>
#include <aros/bootloader.h>
#include <aros/build.h>
#include <aros/config.h>
#include <aros/cpu.h>
#include <aros/crt_replacement.h>
#include <aros/debug.h>
#include <aros/detach.h>
#include <aros/features.h>
#include <aros/genmodule.h>

#ifdef __i386__
#include <aros/i386/atomic.h>
#include <aros/i386/cpucontext.h>
#include <aros/i386/cpu.h>
#include <aros/i386/fenv.h>
#include <aros/i386/genmodule.h>
#include <aros/i386/libcall.h>
#endif

#include <aros/inquire.h>
#include <aros/io.h>
#include <aros/irqtypes.h>
#include <aros/kernel.h>
#include <aros/libcall.h>
#include <aros/locale.h>

#ifdef __mc68000__
#include <aros/m68k/ammxcontext.h>
#include <aros/m68k/asmcall.h>
#include <aros/m68k/atomic.h>
#include <aros/m68k/cpucontext.h>
#include <aros/m68k/cpu.h>
#include <aros/m68k/fenv.h>
#include <aros/m68k/fpucontext.h>
#include <aros/m68k/genmodule.h>
#include <aros/m68k/libcall_cc.h>
#include <aros/m68k/libcall.h>
#endif

#include <aros/macros.h>
#include <aros/mathieee64bitdefines.h>

#ifdef __MORPHOS__
#include <aros/morphos/cpu.h>
#endif

#include <aros/multiboot2.h>

#include <aros/multiboot.h>
#include <aros/oldprograms.h>

#include <aros/posixc/alloca.h>
#include <aros/posixc/dirent.h>
#include <aros/posixc/errno.h>
#include <aros/posixc/fcntl.h>
#include <aros/posixc/getopt.h>
#include <aros/posixc/grp.h>
#include <aros/posixc/libgen.h>
#include <aros/posixc/limits.h>
#include <aros/posixc/locale.h>
#include <aros/posixc/pwd.h>
#include <aros/posixc/regex.h>
#include <aros/posixc/setjmp.h>
#include <aros/posixc/signal.h>
#include <aros/posixc/stdio.h>
#include <aros/posixc/stdlib.h>
#include <aros/posixc/string.h>
#include <aros/posixc/sys/mount.h>
#include <aros/posixc/sys/param.h>
#include <aros/posixc/sys/resource.h>
#include <aros/posixc/sys/select.h>
#include <aros/posixc/sys/stat.h>
#include <aros/posixc/sys/timeb.h>
#include <aros/posixc/sys/time.h>
#include <aros/posixc/sys/times.h>
#include <aros/posixc/sys/types.h>
#include <aros/posixc/sys/uio.h>
#include <aros/posixc/sys/utsname.h>
#include <aros/posixc/sys/wait.h>
#include <aros/posixc/termios.h>
#include <aros/posixc/time.h>
#include <aros/posixc/ucontext.h>
#include <aros/posixc/unistd.h>
#include <aros/posixc/utime.h>

#ifdef __powerpc__
#include <aros/ppc/atomic.h>
#include <aros/ppc/cpucontext.h>
#include <aros/ppc/cpu.h>
#include <aros/ppc/fenv.h>
#include <aros/ppc/genmodule.h>
#endif

#include <aros/preprocessor/array/cast2iptr.hpp>
#include <aros/preprocessor/array/cast2tagitem.hpp>
#include <aros/preprocessor/array.hpp>
#include <aros/preprocessor.hpp>
#include <aros/preprocessor/variadic/cast2iptr.hpp>
#include <aros/preprocessor/variadic/cast2tagitem.hpp>
#include <aros/preprocessor/variadic/elem.hpp>
#include <aros/preprocessor/variadic.hpp>
#include <aros/preprocessor/variadic/size.hpp>
#include <aros/printertag.h>

#include <aros/purify.h>

#include <aros/shcommands_embedded.h>
#include <aros/shcommands.h>
#include <aros/shcommands_notembedded.h>

#include <aros/startup.h>

#include <aros/stdc/assert.h>
#include <aros/stdc/complex.h>
#include <aros/stdc/ctype.h>
#include <aros/stdc/errno.h>
#include <aros/stdc/fenv.h>
#include <aros/stdc/inttypes.h>
#include <aros/stdc/iso646.h>
#include <aros/stdc/limits.h>
#include <aros/stdc/locale.h>
#include <aros/stdc/math.h>
#include <aros/stdc/memory.h>
#include <aros/stdc/sdgstd.h>
#include <aros/stdc/setjmp.h>
#include <aros/stdc/signal.h>
#include <aros/stdc/stdbool.h>
#include <aros/stdc/stddef.h>
#include <aros/stdc/stdint.h>
#include <aros/stdc/stdio.h>
#include <aros/stdc/stdlib.h>
#include <aros/stdc/string.h>
#include <aros/stdc/_strings.h>
#include <aros/stdc/strings.h>
#include <aros/stdc/tgmath.h>
#include <aros/stdc/time.h>
#include <aros/stdc/wchar.h>
#include <aros/stdc/wctype.h>

#include <aros/symbolsets.h>
#include <aros/system.h>
#include <aros/systypes.h>

#include <aros/types/blk_t.h>
#include <aros/types/clockid_t.h>
#include <aros/types/clock_t.h>
#include <aros/types/dev_t.h>
#include <aros/types/fpos_t.h>
#include <aros/types/fs_t.h>
#include <aros/types/gid_t.h>
#include <aros/types/id_t.h>
#include <aros/types/ino_t.h>
#include <aros/types/intptr_t.h>
#include <aros/types/int_t.h>
#include <aros/types/iovec_s.h>
#include <aros/types/itimerspec_s.h>
#include <aros/types/key_t.h>
#include <aros/types/mbstate_t.h>
#include <aros/types/mode_t.h>
#include <aros/types/nlink_t.h>
#include <aros/types/null.h>
#include <aros/types/off_t.h>
#include <aros/types/pid_t.h>
#include <aros/types/ptrdiff_t.h>
#include <aros/types/regoff_t.h>
#include <aros/types/seek.h>
#include <aros/types/sigaction_s.h>
#include <aros/types/sigevent_s.h>
#include <aros/types/__sighandler_t.h>
#include <aros/types/siginfo_t.h>
#include <aros/types/sigset_t.h>
#include <aros/types/size_t.h>
#include <aros/types/socklen_t.h>
#include <aros/types/spinlock_s.h>
#include <aros/types/ssize_t.h>
#include <aros/types/stack_t.h>
#include <aros/types/suseconds_t.h>
#include <aros/types/timer_t.h>
#include <aros/types/timespec_s.h>
#include <aros/types/time_t.h>
#include <aros/types/timeval_s.h>
#include <aros/types/ucontext_t.h>
#include <aros/types/uid_t.h>
#include <aros/types/uintptr_t.h>
#include <aros/types/useconds_t.h>
#include <aros/types/wchar_t.h>
#include <aros/types/wctrans_t.h>
#include <aros/types/wctype_t.h>
#include <aros/types/wint_t.h>

#ifdef __x86_64__
#include <aros/x86_64/atomic.h>
#include <aros/x86_64/cpucontext.h>
#include <aros/x86_64/cpu.h>
#include <aros/x86_64/fenv.h>
#include <aros/x86_64/genmodule.h>
#endif

#include <arpa/ftp.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <arpa/telnet.h>
#include <arpa/tftp.h>

#include <asm/aarch64/mmu.h>
#include <asm/arm/cp15.h>
#include <asm/arm/cpu.h>
#include <asm/armeb/cp15.h>
#include <asm/armeb/cpu.h>
#include <asm/armeb/mmu.h>
#include <asm/armeb/mx51.h>
#include <asm/arm/mmu.h>
#include <asm/arm/mx51.h>
#include <asm/cpu.h>

// #include <asm/i386/cpu.h>
// #include <asm/i386/io.h>
// #include <asm/imx51.h>
// #include <asm/io.h>
// #include <asm/ppc/cpu.h>
// #include <asm/ppc/io.h>
// #include <asm/ppc/ppc740.h>
// #include <asm/x86_64/cpu.h>

#include <bluetooth/assignednumbers.h>
#include <bluetooth/avdtp.h>
#include <bluetooth/hci.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/rfcomm.h>
#include <bluetooth/sdp.h>

// #include <boost/preprocessor/arithmetic/add.hpp>
// #include <boost/preprocessor/arithmetic/dec.hpp>
// #include <boost/preprocessor/arithmetic/detail/div_base.hpp>
// #include <boost/preprocessor/arithmetic/div.hpp>
// #include <boost/preprocessor/arithmetic.hpp>
// #include <boost/preprocessor/arithmetic/inc.hpp>
// #include <boost/preprocessor/arithmetic/mod.hpp>
// #include <boost/preprocessor/arithmetic/mul.hpp>
// #include <boost/preprocessor/arithmetic/sub.hpp>
// #include <boost/preprocessor/array/data.hpp>
// #include <boost/preprocessor/array/detail/get_data.hpp>
// #include <boost/preprocessor/array/elem.hpp>
// #include <boost/preprocessor/array/enum.hpp>
// #include <boost/preprocessor/array.hpp>
// #include <boost/preprocessor/array/insert.hpp>
// #include <boost/preprocessor/array/pop_back.hpp>
// #include <boost/preprocessor/array/pop_front.hpp>
// #include <boost/preprocessor/array/push_back.hpp>
// #include <boost/preprocessor/array/push_front.hpp>
// #include <boost/preprocessor/array/remove.hpp>
// #include <boost/preprocessor/array/replace.hpp>
// #include <boost/preprocessor/array/reverse.hpp>
// #include <boost/preprocessor/array/size.hpp>
// #include <boost/preprocessor/array/to_list.hpp>
// #include <boost/preprocessor/array/to_seq.hpp>
// #include <boost/preprocessor/array/to_tuple.hpp>
// #include <boost/preprocessor/assert_msg.hpp>
// #include <boost/preprocessor/cat.hpp>
// #include <boost/preprocessor/comma.hpp>
// #include <boost/preprocessor/comma_if.hpp>
// #include <boost/preprocessor/comparison/equal.hpp>
// #include <boost/preprocessor/comparison/greater_equal.hpp>
// #include <boost/preprocessor/comparison/greater.hpp>
// #include <boost/preprocessor/comparison.hpp>
// #include <boost/preprocessor/comparison/less_equal.hpp>
// #include <boost/preprocessor/comparison/less.hpp>
// #include <boost/preprocessor/comparison/not_equal.hpp>
// #include <boost/preprocessor/config/config.hpp>
// #include <boost/preprocessor/config/limits.hpp>
// #include <boost/preprocessor/control/deduce_d.hpp>
// #include <boost/preprocessor/control/detail/dmc/while.hpp>
// #include <boost/preprocessor/control/detail/edg/while.hpp>
// #include <boost/preprocessor/control/detail/msvc/while.hpp>
// #include <boost/preprocessor/control/detail/while.hpp>
// #include <boost/preprocessor/control/expr_if.hpp>
// #include <boost/preprocessor/control/expr_iif.hpp>
// #include <boost/preprocessor/control.hpp>
// #include <boost/preprocessor/control/if.hpp>
// #include <boost/preprocessor/control/iif.hpp>
// #include <boost/preprocessor/control/while.hpp>
// #include <boost/preprocessor/debug/assert.hpp>
// #include <boost/preprocessor/debug/error.hpp>
// #include <boost/preprocessor/debug.hpp>
// #include <boost/preprocessor/debug/line.hpp>
// #include <boost/preprocessor/dec.hpp>
// #include <boost/preprocessor/detail/auto_rec.hpp>
// #include <boost/preprocessor/detail/check.hpp>
// #include <boost/preprocessor/detail/dmc/auto_rec.hpp>
// #include <boost/preprocessor/detail/is_binary.hpp>
// #include <boost/preprocessor/detail/is_nullary.hpp>
// #include <boost/preprocessor/detail/is_unary.hpp>
// #include <boost/preprocessor/detail/null.hpp>
// #include <boost/preprocessor/detail/split.hpp>
// #include <boost/preprocessor/empty.hpp>
// #include <boost/preprocessor/enum.hpp>
// #include <boost/preprocessor/enum_params.hpp>
// #include <boost/preprocessor/enum_params_with_a_default.hpp>
// #include <boost/preprocessor/enum_params_with_defaults.hpp>
// #include <boost/preprocessor/enum_shifted.hpp>
// #include <boost/preprocessor/enum_shifted_params.hpp>
// #include <boost/preprocessor/expand.hpp>
// #include <boost/preprocessor/expr_if.hpp>
// #include <boost/preprocessor/facilities/apply.hpp>
// #include <boost/preprocessor/facilities/detail/is_empty.hpp>
// #include <boost/preprocessor/facilities/empty.hpp>
// #include <boost/preprocessor/facilities/expand.hpp>
// #include <boost/preprocessor/facilities.hpp>
// #include <boost/preprocessor/facilities/identity.hpp>
// #include <boost/preprocessor/facilities/intercept.hpp>
// #include <boost/preprocessor/facilities/is_1.hpp>
// #include <boost/preprocessor/facilities/is_empty.hpp>
// #include <boost/preprocessor/facilities/is_empty_or_1.hpp>
// #include <boost/preprocessor/facilities/is_empty_variadic.hpp>
// #include <boost/preprocessor/facilities/overload.hpp>
// #include <boost/preprocessor/for.hpp>
// #include <boost/preprocessor.hpp>
// #include <boost/preprocessor/identity.hpp>
// #include <boost/preprocessor/if.hpp>
// #include <boost/preprocessor/inc.hpp>
// #include <boost/preprocessor/iterate.hpp>
// #include <boost/preprocessor/iteration/detail/bounds/lower1.hpp>
// #include <boost/preprocessor/iteration/detail/bounds/lower2.hpp>
// #include <boost/preprocessor/iteration/detail/bounds/lower3.hpp>
// #include <boost/preprocessor/iteration/detail/bounds/lower4.hpp>
// #include <boost/preprocessor/iteration/detail/bounds/lower5.hpp>
// #include <boost/preprocessor/iteration/detail/bounds/upper1.hpp>
// #include <boost/preprocessor/iteration/detail/bounds/upper2.hpp>
// #include <boost/preprocessor/iteration/detail/bounds/upper3.hpp>
// #include <boost/preprocessor/iteration/detail/bounds/upper4.hpp>
// #include <boost/preprocessor/iteration/detail/bounds/upper5.hpp>
// #include <boost/preprocessor/iteration/detail/finish.hpp>
// #include <boost/preprocessor/iteration/detail/iter/forward1.hpp>
// #include <boost/preprocessor/iteration/detail/iter/forward2.hpp>
// #include <boost/preprocessor/iteration/detail/iter/forward3.hpp>
// #include <boost/preprocessor/iteration/detail/iter/forward4.hpp>
// #include <boost/preprocessor/iteration/detail/iter/forward5.hpp>
// #include <boost/preprocessor/iteration/detail/iter/reverse1.hpp>
// #include <boost/preprocessor/iteration/detail/iter/reverse2.hpp>
// #include <boost/preprocessor/iteration/detail/iter/reverse3.hpp>
// #include <boost/preprocessor/iteration/detail/iter/reverse4.hpp>
// #include <boost/preprocessor/iteration/detail/iter/reverse5.hpp>
// #include <boost/preprocessor/iteration/detail/local.hpp>
// #include <boost/preprocessor/iteration/detail/rlocal.hpp>
// #include <boost/preprocessor/iteration/detail/self.hpp>
// #include <boost/preprocessor/iteration/detail/start.hpp>
// #include <boost/preprocessor/iteration.hpp>
// #include <boost/preprocessor/iteration/iterate.hpp>
// #include <boost/preprocessor/iteration/local.hpp>
// #include <boost/preprocessor/iteration/self.hpp>
// #include <boost/preprocessor/library.hpp>
// #include <boost/preprocessor/limits.hpp>
// #include <boost/preprocessor/list/adt.hpp>
// #include <boost/preprocessor/list/append.hpp>
// #include <boost/preprocessor/list/at.hpp>
// #include <boost/preprocessor/list/cat.hpp>
// #include <boost/preprocessor/list/detail/dmc/fold_left.hpp>
// #include <boost/preprocessor/list/detail/edg/fold_left.hpp>
// #include <boost/preprocessor/list/detail/edg/fold_right.hpp>
// #include <boost/preprocessor/list/detail/fold_left.hpp>
// #include <boost/preprocessor/list/detail/fold_right.hpp>
// #include <boost/preprocessor/list/enum.hpp>
// #include <boost/preprocessor/list/filter.hpp>
// #include <boost/preprocessor/list/first_n.hpp>
// #include <boost/preprocessor/list/fold_left.hpp>
// #include <boost/preprocessor/list/fold_right.hpp>
// #include <boost/preprocessor/list/for_each.hpp>
// #include <boost/preprocessor/list/for_each_i.hpp>
// #include <boost/preprocessor/list/for_each_product.hpp>
// #include <boost/preprocessor/list.hpp>
// #include <boost/preprocessor/list/rest_n.hpp>
// #include <boost/preprocessor/list/reverse.hpp>
// #include <boost/preprocessor/list/size.hpp>
// #include <boost/preprocessor/list/to_array.hpp>
// #include <boost/preprocessor/list/to_seq.hpp>
// #include <boost/preprocessor/list/to_tuple.hpp>
// #include <boost/preprocessor/list/transform.hpp>
// #include <boost/preprocessor/logical/and.hpp>
// #include <boost/preprocessor/logical/bitand.hpp>
// #include <boost/preprocessor/logical/bitnor.hpp>
// #include <boost/preprocessor/logical/bitor.hpp>
// #include <boost/preprocessor/logical/bitxor.hpp>
// #include <boost/preprocessor/logical/bool.hpp>
// #include <boost/preprocessor/logical/compl.hpp>
// #include <boost/preprocessor/logical.hpp>
// #include <boost/preprocessor/logical/nor.hpp>
// #include <boost/preprocessor/logical/not.hpp>
// #include <boost/preprocessor/logical/or.hpp>
// #include <boost/preprocessor/logical/xor.hpp>
// #include <boost/preprocessor/max.hpp>
// #include <boost/preprocessor/min.hpp>
// #include <boost/preprocessor/punctuation/comma.hpp>
// #include <boost/preprocessor/punctuation/comma_if.hpp>
// #include <boost/preprocessor/punctuation/detail/is_begin_parens.hpp>
// #include <boost/preprocessor/punctuation.hpp>
// #include <boost/preprocessor/punctuation/is_begin_parens.hpp>
// #include <boost/preprocessor/punctuation/paren.hpp>
// #include <boost/preprocessor/punctuation/paren_if.hpp>
// #include <boost/preprocessor/punctuation/remove_parens.hpp>
// #include <boost/preprocessor/repeat_2nd.hpp>
// #include <boost/preprocessor/repeat_3rd.hpp>
// #include <boost/preprocessor/repeat_from_to_2nd.hpp>
// #include <boost/preprocessor/repeat_from_to_3rd.hpp>
// #include <boost/preprocessor/repeat_from_to.hpp>
// #include <boost/preprocessor/repeat.hpp>
// #include <boost/preprocessor/repetition/deduce_r.hpp>
// #include <boost/preprocessor/repetition/deduce_z.hpp>
// #include <boost/preprocessor/repetition/detail/dmc/for.hpp>
// #include <boost/preprocessor/repetition/detail/edg/for.hpp>
// #include <boost/preprocessor/repetition/detail/for.hpp>
// #include <boost/preprocessor/repetition/detail/msvc/for.hpp>
// #include <boost/preprocessor/repetition/enum_binary_params.hpp>
// #include <boost/preprocessor/repetition/enum.hpp>
// #include <boost/preprocessor/repetition/enum_params.hpp>
// #include <boost/preprocessor/repetition/enum_params_with_a_default.hpp>
// #include <boost/preprocessor/repetition/enum_params_with_defaults.hpp>
// #include <boost/preprocessor/repetition/enum_shifted_binary_params.hpp>
// #include <boost/preprocessor/repetition/enum_shifted.hpp>
// #include <boost/preprocessor/repetition/enum_shifted_params.hpp>
// #include <boost/preprocessor/repetition/enum_trailing_binary_params.hpp>
// #include <boost/preprocessor/repetition/enum_trailing.hpp>
// #include <boost/preprocessor/repetition/enum_trailing_params.hpp>
// #include <boost/preprocessor/repetition/for.hpp>
// #include <boost/preprocessor/repetition.hpp>
// #include <boost/preprocessor/repetition/repeat_from_to.hpp>
// #include <boost/preprocessor/repetition/repeat.hpp>
// #include <boost/preprocessor/selection.hpp>
// #include <boost/preprocessor/selection/max.hpp>
// #include <boost/preprocessor/selection/min.hpp>
// #include <boost/preprocessor/seq/cat.hpp>
// #include <boost/preprocessor/seq/detail/binary_transform.hpp>
// #include <boost/preprocessor/seq/detail/is_empty.hpp>
// #include <boost/preprocessor/seq/detail/split.hpp>
// #include <boost/preprocessor/seq/elem.hpp>
// #include <boost/preprocessor/seq/enum.hpp>
// #include <boost/preprocessor/seq/filter.hpp>
// #include <boost/preprocessor/seq/first_n.hpp>
// #include <boost/preprocessor/seq/fold_left.hpp>
// #include <boost/preprocessor/seq/fold_right.hpp>
// #include <boost/preprocessor/seq/for_each.hpp>
// #include <boost/preprocessor/seq/for_each_i.hpp>
// #include <boost/preprocessor/seq/for_each_product.hpp>
// #include <boost/preprocessor/seq.hpp>
// #include <boost/preprocessor/seq/insert.hpp>
// #include <boost/preprocessor/seq/pop_back.hpp>
// #include <boost/preprocessor/seq/pop_front.hpp>
// #include <boost/preprocessor/seq/push_back.hpp>
// #include <boost/preprocessor/seq/push_front.hpp>
// #include <boost/preprocessor/seq/remove.hpp>
// #include <boost/preprocessor/seq/replace.hpp>
// #include <boost/preprocessor/seq/rest_n.hpp>
// #include <boost/preprocessor/seq/reverse.hpp>
// #include <boost/preprocessor/seq/seq.hpp>
// #include <boost/preprocessor/seq/size.hpp>
// #include <boost/preprocessor/seq/subseq.hpp>
// #include <boost/preprocessor/seq/to_array.hpp>
// #include <boost/preprocessor/seq/to_list.hpp>
// #include <boost/preprocessor/seq/to_tuple.hpp>
// #include <boost/preprocessor/seq/transform.hpp>
// #include <boost/preprocessor/seq/variadic_seq_to_seq.hpp>
// #include <boost/preprocessor/slot/counter.hpp>
// #include <boost/preprocessor/slot/detail/counter.hpp>
// #include <boost/preprocessor/slot/detail/def.hpp>
// #include <boost/preprocessor/slot/detail/shared.hpp>
// #include <boost/preprocessor/slot/detail/slot1.hpp>
// #include <boost/preprocessor/slot/detail/slot2.hpp>
// #include <boost/preprocessor/slot/detail/slot3.hpp>
// #include <boost/preprocessor/slot/detail/slot4.hpp>
// #include <boost/preprocessor/slot/detail/slot5.hpp>
// #include <boost/preprocessor/slot.hpp>
// #include <boost/preprocessor/slot/slot.hpp>
// #include <boost/preprocessor/stringize.hpp>
// #include <boost/preprocessor/tuple/detail/is_single_return.hpp>
// #include <boost/preprocessor/tuple/eat.hpp>
// #include <boost/preprocessor/tuple/elem.hpp>
// #include <boost/preprocessor/tuple/enum.hpp>
// #include <boost/preprocessor/tuple.hpp>
// #include <boost/preprocessor/tuple/insert.hpp>
// #include <boost/preprocessor/tuple/pop_back.hpp>
// #include <boost/preprocessor/tuple/pop_front.hpp>
// #include <boost/preprocessor/tuple/push_back.hpp>
// #include <boost/preprocessor/tuple/push_front.hpp>
// #include <boost/preprocessor/tuple/rem.hpp>
// #include <boost/preprocessor/tuple/remove.hpp>
// #include <boost/preprocessor/tuple/replace.hpp>
// #include <boost/preprocessor/tuple/reverse.hpp>
// #include <boost/preprocessor/tuple/size.hpp>
// #include <boost/preprocessor/tuple/to_array.hpp>
// #include <boost/preprocessor/tuple/to_list.hpp>
// #include <boost/preprocessor/tuple/to_seq.hpp>
// #include <boost/preprocessor/variadic/detail/is_single_return.hpp>
// #include <boost/preprocessor/variadic/elem.hpp>
// #include <boost/preprocessor/variadic.hpp>
// #include <boost/preprocessor/variadic/size.hpp>
// #include <boost/preprocessor/variadic/to_array.hpp>
// #include <boost/preprocessor/variadic/to_list.hpp>
// #include <boost/preprocessor/variadic/to_seq.hpp>
// #include <boost/preprocessor/variadic/to_tuple.hpp>
// #include <boost/preprocessor/while.hpp>
// #include <boost/preprocessor/wstringize.hpp>

#include <bsdsocket/socketbasetags.h>
#include <bsdsocket/types.h>

#include <bzlib.h>

#include <c++/exec/types.hpp>

// #include <c_iff.h>

// #include <clib/Aboutbox_protos.h>
#include <clib/Aboutmui_protos.h>
#include <clib/AboutWindow_protos.h>
#include <clib/ahi_protos.h>
#include <clib/ahi_sub_protos.h>
#include <clib/alib_protos.h>
#include <clib/amigaguide_protos.h>
#include <clib/aros_protos.h>
#include <clib/arossupport_protos.h>
#include <clib/asl_protos.h>
#include <clib/ata_protos.h>
#include <clib/Balance_protos.h>
#include <clib/battclock_protos.h>
#include <clib/bestcomm_protos.h>
#include <clib/Boopsi_protos.h>
#include <clib/boopsistubs.h>
#include <clib/bootloader_protos.h>
#include <clib/bsdsocket_protos.h>
#include <clib/bullet_protos.h>
#include <clib/Busy_protos.h>
#include <clib/bz2_protos.h>
#include <clib/Calendar_protos.h>
// #include <clib/Calltips_protos.h>
#include <clib/camd_protos.h>
#include <clib/cardres_protos.h>
#include <clib/cia_protos.h>
#include <clib/Clock_protos.h>
#include <clib/clocksource_protos.h>
#include <clib/codesets_protos.h>
// #include <clib/Coloradjust_protos.h>
// #include <clib/Colorfield_protos.h>
#include <clib/colorwheel_protos.h>
#include <clib/commodities_protos.h>
#include <clib/console_protos.h>
#include <clib/coolimages_protos.h>
// #include <clib/Crawling_protos.h>
#include <clib/cybergraphics_protos.h>
#include <clib/datatypes_protos.h>
#include <clib/debug_protos.h>
// #include <clib/Dirlist_protos.h>
#include <clib/diskfont_protos.h>
#include <clib/diskimage_protos.h>
#include <clib/dos_protos.h>
// #include <clib/Dtpic_protos.h>
#include <clib/dxtn_protos.h>
#include <clib/efi_protos.h>
#include <clib/execlock_protos.h>
#include <clib/exec_protos.h>
#include <clib/expansion_protos.h>
#include <clib/expat_protos.h>
// #include <clib/Floattext_protos.h>
#include <clib/freetype2_protos.h>
#include <clib/gadtools_protos.h>
#include <clib/gallium_protos.h>
#include <clib/gameport_protos.h>
#include <clib/gl_protos.h>
#include <clib/glu_protos.h>
#include <clib/graphics_protos.h>
#include <clib/Graph_protos.h>
#include <clib/hostlib_protos.h>
#include <clib/hpet_protos.h>
#include <clib/IconDrawerList_protos.h>
#include <clib/IconImage_protos.h>
#include <clib/IconList_protos.h>
#include <clib/IconListview_protos.h>
#include <clib/icon_protos.h>
#include <clib/IconVolumeList_protos.h>
#include <clib/identify_protos.h>
#include <clib/iffparse_protos.h>
#include <clib/input_protos.h>
#include <clib/intuition_protos.h>
#include <clib/isapnp_protos.h>
#include <clib/jfif_protos.h>
#include <clib/kernel_protos.h>
#include <clib/keymap_protos.h>
#include <clib/kms_protos.h>
// #include <clib/Knob_protos.h>
// #include <clib/Lamp_protos.h>
#include <clib/layers_protos.h>
#include <clib/lddemon_protos.h>
// #include <clib/Levelmeter_protos.h>
// #include <clib/Listtree_protos.h>
#include <clib/locale_protos.h>
#include <clib/lowlevel_protos.h>
#include <clib/macros.h>
#include <clib/mathffp_protos.h>
#include <clib/mathieeedoubbas_protos.h>
#include <clib/mathieeedoubtrans_protos.h>
#include <clib/mathieeesingbas_protos.h>
#include <clib/mathieeesingtrans_protos.h>
#include <clib/mathtrans_protos.h>
#include <clib/miamipanel_protos.h>
#include <clib/miami_protos.h>
#include <clib/misc_protos.h>
#include <clib/muimaster_protos.h>
#include <clib/muiscreen_protos.h>
// #include <clib/netlib_protos.h>
#include <clib/nonvolatile_protos.h>
#include <clib/nvdisk_protos.h>
#include <clib/oop_protos.h>
#include <clib/openfirmware_protos.h>
#include <clib/openurl_protos.h>
// #include <clib/Palette_protos.h>
#include <clib/partition_protos.h>
#include <clib/pccard_protos.h>
#include <clib/pngdt_protos.h>
#include <clib/png_protos.h>
// #include <clib/Popasl_protos.h>
// #include <clib/Popframe_protos.h>
// #include <clib/Popimage_protos.h>
// #include <clib/Poplist_protos.h>
// #include <clib/Poppen_protos.h>
// #include <clib/Popscreen_protos.h>
#include <clib/popupmenu_protos.h>
#include <clib/poseidon_protos.h>
// #include <clib/posixc_protos.h>
#include <clib/PrefsEditor_protos.h>
#include <clib/PrefsWindow_protos.h>
#include <clib/processor_protos.h>
// #include <clib/Process_protos.h>
#include <clib/prometheus_protos.h>
// #include <clib/Radio_protos.h>
#include <clib/ramdrive_protos.h>
// #include <clib/Rawimage_protos.h>
#include <clib/realtime_protos.h>
#include <clib/reqtools_protos.h>
#include <clib/rexxsupport_protos.h>
#include <clib/rexxsyslib_protos.h>
#include <clib/rtas_protos.h>
// #include <clib/Scrollgroup_protos.h>
#include <clib/scsi_protos.h>
// #include <clib/Settingsgroup_protos.h>
#include <clib/socket_protos.h>
// #include <clib/stdcio_protos.h>
// #include <clib/stdc_protos.h>
#include <clib/SystemPrefsWindow_protos.h>
#include <clib/task_protos.h>
#include <clib/timer_protos.h>
// #include <clib/Title_protos.h>
#include <clib/usbclass_protos.h>
#include <clib/utility_protos.h>
#include <clib/uuid_protos.h>
#include <clib/version_protos.h>
// #include <clib/Virtgroup_protos.h>
// #include <clib/Volumelist_protos.h>
#include <clib/wb_protos.h>
#include <clib/workbench_protos.h>
#include <clib/x11gfx_protos.h>
#include <clib/z1_protos.h>

#include <c++/swappedtype.hpp>

// #include <CUnit/CUCurses.h>
// #include <CUnit/CUError.h>
// #include <CUnit/CUnitCI.h>
// #include <CUnit/CUnitCITypes.h>
#include <CUnit/CUnit_intl.h>

#include <cybergraphx/cgxvideo.h>
#include <cybergraphx/cybergraphics.h>

#include <datatypes/amigaguideclass.h>
#include <datatypes/animationclassext.h>
#include <datatypes/animationclass.h>
#include <datatypes/datatypesclass.h>
#include <datatypes/datatypes.h>
#include <datatypes/PictureClassExt.h>
#include <datatypes/pictureclass.h>
#include <datatypes/soundclassext.h>
#include <datatypes/soundclass.h>
#include <datatypes/textclass.h>

#include <devices/ahi.h>
#include <devices/ata.h>
#include <devices/atascsi.h>
#include <devices/audio.h>
#include <devices/bluetoothhci.h>
#include <devices/bootblock.h>
#include <devices/cd.h>
#include <devices/clipboard.h>
#include <devices/console.h>
#include <devices/conunit.h>
// #include <devices/diskimage.h>
#include <devices/gameport.h>
#include <devices/hardblocks.h>
#include <devices/inputevent.h>
#include <devices/input.h>
#include <devices/irda.h>
#include <devices/keyboard.h>
#include <devices/keymap.h>
#include <devices/narrator.h>
#include <devices/newstyle.h>
#include <devices/parallel.h>
#include <devices/printer.h>
#include <devices/prtbase.h>
#include <devices/prtgfx.h>
#include <devices/rawkeycodes.h>
#include <devices/sana2.h>
#include <devices/sana2specialstats.h>
#include <devices/sana2wireless.h>
#include <devices/scsicmds.h>
#include <devices/scsidisk.h>
#include <devices/serial.h>
#include <devices/smart.h>
#include <devices/timer.h>
#include <devices/trackdisk.h>
#include <devices/usb_audio.h>
#include <devices/usb_cdc.h>
#include <devices/usb_dfu.h>
#include <devices/usb.h>
#include <devices/usbhardware.h>
#include <devices/usb_hid.h>
#include <devices/usb_hub.h>
#include <devices/usb_massstorage.h>
#include <devices/usb_printer.h>
#include <devices/usb_video.h>

#include <diskfont/diskfont.h>
#include <diskfont/diskfonttag.h>
#include <diskfont/glyph.h>
#include <diskfont/oterrors.h>

#include <dos/bptr.h>
#include <dos/cliinit.h>
#include <dos/datetime.h>
#include <dos/dosasl.h>
#include <dos/dosextens.h>
#include <dos/dos.h>
#include <dos/doshunks.h>
#include <dos/dostags.h>
#include <dos/elf.h>
#include <dos/exall.h>
#include <dos/filehandler.h>
#include <dos/filesystemids.h>
#include <dos/notify.h>
#include <dos/rdargs.h>
#include <dos/record.h>
#include <dos/stdio.h>
#include <dos/var.h>

// #include <EGL/eglextchromium.h>
// #include <EGL/eglext.h>
#include <EGL/egl.h>
// #include <EGL/eglmesaext.h>
// #include <EGL/eglplatform.h>

#include <endian.h>
#include <err.h>

#include <exec/alerts.h>
#include <exec/avl.h>
#include <exec/devices.h>
#include <exec/errors.h>
#include <exec/execbase.h>
#include <exec/exec.h>
#include <exec/initializers.h>
#include <exec/interrupts.h>
#include <exec/io.h>
#include <exec/libraries.h>
#include <exec/lists.h>
#include <exec/memheaderext.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <exec/ports.h>
#include <exec/rawfmt.h>
#include <exec/resident.h>
#include <exec/semaphores.h>
#include <exec/tasks.h>
#include <exec/types.h>

#include <expat_external.h>
#include <expat.h>

// #include <freetype/config/ftconfig.h>
// #include <freetype/config/ftheader.h>
// #include <freetype/config/ftmodule.h>
// #include <freetype/config/ftoption.h>
// #include <freetype/config/ftstdlib.h>
// #include <freetype/config/integer-types.h>
// #include <freetype/config/mac-support.h>
// #include <freetype/config/public-macros.h>
// #include <freetype/freetype.h>
// #include <freetype/ftadvanc.h>
// #include <freetype/ftbbox.h>
// #include <freetype/ftbdf.h>
// #include <freetype/ftbitmap.h>
// #include <freetype/ftbzip2.h>
// #include <freetype/ftcache.h>
// #include <freetype/ftchapters.h>
// #include <freetype/ftcid.h>
// #include <freetype/ftcolor.h>
// #include <freetype/ftdriver.h>
// #include <freetype/fterrdef.h>
// #include <freetype/fterrors.h>
// #include <freetype/ftfntfmt.h>
// #include <freetype/ftgasp.h>
// #include <freetype/ftglyph.h>
// #include <freetype/ftgxval.h>
// #include <freetype/ftgzip.h>
// #include <freetype/ftimage.h>
// #include <freetype/ftincrem.h>
// #include <freetype/ftlcdfil.h>
// #include <freetype/ftlist.h>
// #include <freetype/ftlzw.h>
// #include <freetype/ftmac.h>
// #include <freetype/ftmm.h>
// #include <freetype/ftmodapi.h>
// #include <freetype/ftmoderr.h>
// #include <freetype/ftotval.h>
// #include <freetype/ftoutln.h>
// #include <freetype/ftparams.h>
// #include <freetype/ftpfr.h>
// #include <freetype/ftrender.h>
// #include <freetype/ftsizes.h>
// #include <freetype/ftsnames.h>
// #include <freetype/ftstroke.h>
// #include <freetype/ftsynth.h>
// #include <freetype/ftsystem.h>
// #include <freetype/fttrigon.h>
// #include <freetype/fttypes.h>
// #include <freetype/ftwinfnt.h>
// #include <freetype/internal/autohint.h>
// #include <freetype/internal/cffotypes.h>
// #include <freetype/internal/cfftypes.h>
// #include <freetype/internal/compiler-macros.h>
// #include <freetype/internal/ftcalc.h>
// #include <freetype/internal/ftdebug.h>
// #include <freetype/internal/ftdrv.h>
// #include <freetype/internal/ftgloadr.h>
// #include <freetype/internal/fthash.h>
// #include <freetype/internal/ftmemory.h>
// #include <freetype/internal/ftobjs.h>
// #include <freetype/internal/ftpsprop.h>
// #include <freetype/internal/ftrfork.h>
// #include <freetype/internal/ftserv.h>
// #include <freetype/internal/ftstream.h>
// #include <freetype/internal/fttrace.h>
// #include <freetype/internal/ftvalid.h>
// #include <freetype/internal/psaux.h>
// #include <freetype/internal/pshints.h>
// #include <freetype/internal/services/svbdf.h>
// #include <freetype/internal/services/svcfftl.h>
// #include <freetype/internal/services/svcid.h>
// #include <freetype/internal/services/svfntfmt.h>
// #include <freetype/internal/services/svgldict.h>
// #include <freetype/internal/services/svgxval.h>
// #include <freetype/internal/services/svkern.h>
// #include <freetype/internal/services/svmetric.h>
// #include <freetype/internal/services/svmm.h>
// #include <freetype/internal/services/svotval.h>
// #include <freetype/internal/services/svpfr.h>
// #include <freetype/internal/services/svpostnm.h>
// #include <freetype/internal/services/svprop.h>
// #include <freetype/internal/services/svpscmap.h>
// #include <freetype/internal/services/svpsinfo.h>
// #include <freetype/internal/services/svsfnt.h>
// #include <freetype/internal/services/svttcmap.h>
// #include <freetype/internal/services/svtteng.h>
// #include <freetype/internal/services/svttglyf.h>
// #include <freetype/internal/services/svwinfnt.h>
// #include <freetype/internal/sfnt.h>
// #include <freetype/internal/t1types.h>
// #include <freetype/internal/tttypes.h>
// #include <freetype/internal/wofftypes.h>
// #include <freetype/t1tables.h>
// #include <freetype/ttnameid.h>
// #include <freetype/tttables.h>
// #include <freetype/tttags.h>
#include <ft2build.h>

#include <gadgets/arosmx.h>
#include <gadgets/colorwheel.h>
#include <gadgets/gradientslider.h>
#include <gadgets/tapedeck.h>

#include <gallium/gallium.h>

#include <GL/gla.h>
#include <GL/glext.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <graphics/clip.h>
#include <graphics/collide.h>
#include <graphics/copper.h>
#include <graphics/displayinfo.h>
#include <graphics/driver.h>
#include <graphics/gels.h>
#include <graphics/gfxbase.h>
#include <graphics/gfx.h>
#include <graphics/gfxmacros.h>
#include <graphics/gfxnodes.h>
#include <graphics/layersext.h>
#include <graphics/layers.h>
#include <graphics/modeid.h>
#include <graphics/monitor.h>
#include <graphics/rastport.h>
#include <graphics/regions.h>
#include <graphics/rpattr.h>
#include <graphics/scale.h>
#include <graphics/sprite.h>
#include <graphics/text.h>
#include <graphics/videocontrol.h>
#include <graphics/view.h>

// #include <hardware/ahci.h>
#include <hardware/arasan.h>
#include <hardware/ata.h>
#include <hardware/bcm2708_boot.h>
#include <hardware/bcm2708.h>
#include <hardware/blit.h>
#include <hardware/cia.h>
#include <hardware/cpu/cpu.h>
#include <hardware/cpu/cpu_i386.h>
#include <hardware/cpu/cpu_m68k.h>
#include <hardware/cpu/cpu_mpspec.h>
#include <hardware/cpu/memory.h>
#include <hardware/custom.h>
#include <hardware/efi/config.h>
#include <hardware/efi/console.h>
#include <hardware/efi/efi.h>
#include <hardware/efi/runtime.h>
#include <hardware/efi/tables.h>
#include <hardware/intbits.h>
#include <hardware/mmc.h>
#include <hardware/mx51_tzic.h>
#include <hardware/mx51_uart.h>
#include <hardware/nvme.h>
#include <hardware/openfirmware.h>
#include <hardware/pci.h>
#include <hardware/pic/pic.h>
#include <hardware/pit.h>
#include <hardware/pl011uart.h>
#include <hardware/scsi.h>
#include <hardware/sdhc.h>
#include <hardware/uart.h>
#include <hardware/usb2otg.h>
#include <hardware/vbe.h>
#include <hardware/videocore.h>

// #include <hidd/acpibutton.h>
#include <hidd/agp.h>
#include <hidd/ahci.h>
// #include <hidd/ata.h>
#include <hidd/bus.h>
#include <hidd/compositor.h>
#include <hidd/config.h>
#include <hidd/gallium.h>
#include <hidd/gfx.h>
#include <hidd/hidd.h>
#include <hidd/i2c.h>
#include <hidd/keyboard.h>
#include <hidd/mouse.h>
// #include <hidd/nvme.h>
#include <hidd/parallel.h>
#include <hidd/pci.h>
#include <hidd/scsi.h>
#include <hidd/serial.h>
#include <hidd/storage.h>
#include <hidd/system.h>
#include <hidd/unixio.h>
#include <hidd/usb.h>

#include <inetd.h>

#include <interface/Hidd_AHCIBus.h>
#include <interface/Hidd_AHCI.h>
#include <interface/Hidd_AHCIUnit.h>
// #include <interface/Hidd_ATABus.h>
#include <interface/Hidd_ATAUnit.h>
#include <interface/Hidd_BitMap.h>
#include <interface/Hidd_BMHistogram.h>
#include <interface/Hidd_Bus.h>
#include <interface/Hidd_ChunkyBM.h>
#include <interface/Hidd_ColorMap.h>
#include <interface/Hidd_GC.h>
#include <interface/Hidd_Gfx.h>
#include <interface/Hidd.h>
#include <interface/Hidd_Overlay.h>
#include <interface/Hidd_PCIDevice.h>
#include <interface/Hidd_PCIDriver.h>
#include <interface/Hidd_PCI.h>
#include <interface/Hidd_PixFmt.h>
#include <interface/Hidd_PlanarBM.h>
#include <interface/Hidd_SCSIBus.h>
#include <interface/Hidd_SCSIUnit.h>
#include <interface/Hidd_StorageBus.h>
#include <interface/Hidd_StorageController.h>
#include <interface/Hidd_Storage.h>
#include <interface/Hidd_StorageUnit.h>
#include <interface/Hidd_Sync.h>
#include <interface/Hidd_UnixIO.h>
#include <interface/HW.h>
// #include <interfaces/codesets.h>
// #include <interfaces/openurl.h>

#include <intuition/cghooks.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/diattr.h>
#include <intuition/extensions.h>
#include <intuition/gadgetclass.h>
#include <intuition/icclass.h>
#include <intuition/imageclass.h>
#include <intuition/intuitionbase.h>
#include <intuition/intuition.h>
#include <intuition/iobsolete.h>
#include <intuition/iprefs.h>
#include <intuition/menudecorclass.h>
#include <intuition/monitorclass.h>
#include <intuition/pointerclass.h>
#include <intuition/preferences.h>
#include <intuition/scrdecorclass.h>
#include <intuition/screens.h>
#include <intuition/sghooks.h>
#include <intuition/windecorclass.h>

#include <irda/irlap.h>

#include <jconfig.h>
#include <jerror.h>
#include <jmorecfg.h>
#include <jpeglib.h>

#include <KHR/khrplatform.h>

#include <libcore/compiler.h>

// #include <libraries/acpica.h>
#include <libraries/ahi_sub.h>
#include <libraries/amigaguide.h>
#include <libraries/arp.h>
#include <libraries/asl.h>
#include <libraries/bsdsocket.h>
#include <libraries/codesets.h>
#include <libraries/commodities.h>
#include <libraries/configregs.h>
#include <libraries/configvars.h>
#include <libraries/coolimages.h>
#include <libraries/cybergraphics.h>
#include <libraries/debug.h>
#include <libraries/desktop.h>
#include <libraries/diskfont.h>
#include <libraries/dosextens.h>
#include <libraries/dos.h>
#include <libraries/expansionbase.h>
#include <libraries/expansion.h>
#include <libraries/filehandler.h>
#include <libraries/gadtools.h>
#include <libraries/identify.h>
#include <libraries/iffparse.h>
#include <libraries/kms.h>
#include <libraries/locale.h>
#include <libraries/lowlevel_ext.h>
#include <libraries/lowlevel.h>
#include <libraries/mathffp.h>
#include <libraries/mathieeedp.h>
#include <libraries/mathieeesp.h>
#include <libraries/miami.h>
#include <libraries/miamipanel.h>
// #include <libraries/mufs.h>
#include <libraries/mui.h>
#include <libraries/muiscreen.h>
#include <libraries/nonvolatile.h>
#include <libraries/openurl.h>
#include <libraries/partition.h>
#include <libraries/pccard.h>
#include <libraries/pm.h>
#include <libraries/poseidon.h>
#include <libraries/posixc.h>
#include <libraries/prometheus.h>
#include <libraries/realtime.h>
#include <libraries/reqtools.h>
#include <libraries/security.h>
#include <libraries/stdc.h>
#include <libraries/stdcio.h>
#include <libraries/thread.h>
#include <libraries/usbclass.h>
// #include <libraries/usergroup.h>
#include <libraries/uuid.h>

#include <linklibs/coolimages.h>

#include <midi/camddevices.h>
#include <midi/camd.h>
#include <midi/mididefs.h>

#include <mui/Aboutbox_mcc.h>
#include <mui/BetterString_mcc.h>
#include <mui/Busy_mcc.h>
#include <mui/Calltips_mcc.h>
#include <mui/HotkeyString_mcc.h>
#include <mui/Lamp_mcc.h>
#include <mui/Listtree_mcc.h>
#include <mui/NBalance_mcc.h>
#include <mui/NBitmap_mcc.h>
#include <mui/NFloattext_mcc.h>
#include <mui/NList_mcc.h>
#include <mui/NListtree_mcc.h>
#include <mui/NListview_mcc.h>
#include <mui/NListviews_mcp.h>
#include <mui/Rawimage_mcc.h>
#include <mui/TextEditor_mcc.h>
#include <mui/TextEditor_mcp.h>

// #include <multiboot.h>

#include <netdb.h>
#include <net/if_arp.h>
#include <net/if_dl.h>
#include <net/if.h>
#include <net/if_llc.h>
// #include <net/if_slvar.h>
#include <net/if_types.h>

// #include <netinet/icmp_var.h>
#include <netinet/in.h>
// #include <netinet/in_pcb.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip_var.h>
#include <netinet/tcp.h>
// #include <netinet/tcp_var.h>
#include <netinet/udp.h>
#include <netinet/udp_var.h>

#include <net/radix.h>
#include <net/route.h>
#include <net/sana2errno.h>

#include <oop/ifmeta.h>
#include <oop/oop.h>
#include <oop/static_mid.h>

#include <pngconf.h>
#include <png.h>
#include <pnglibconf.h>

#include <prefs/font.h>
#include <prefs/icontrol.h>
#include <prefs/input.h>
#include <prefs/locale.h>
#include <prefs/overscan.h>
#include <prefs/palette.h>
#include <prefs/pointer.h>
#include <prefs/prefhdr.h>
#include <prefs/printergfx.h>
#include <prefs/printerps.h>
#include <prefs/printertxt.h>
#include <prefs/screenmode.h>
#include <prefs/serial.h>
#include <prefs/sound.h>
#include <prefs/trackdisk.h>
#include <prefs/wanderer.h>
#include <prefs/wbpattern.h>
#include <prefs/workbench.h>

// #include <proto/Aboutbox.h>
#include <proto/Aboutmui.h>
#include <proto/AboutWindow.h>
#include <proto/ahi.h>
#include <proto/ahi_sub.h>
#include <proto/alib.h>
#include <proto/amigaguide.h>
#include <proto/aros.h>
#include <proto/arossupport.h>
#include <proto/asl.h>
#include <proto/ata.h>
#include <proto/Balance.h>
#include <proto/battclock.h>
// #include <proto/battmem.h>
#include <proto/bestcomm.h>
#include <proto/Boopsi.h>
#include <proto/bootloader.h>
#include <proto/bsdsocket.h>
#include <proto/bullet.h>
#include <proto/Busy.h>
#include <proto/bz2.h>
#include <proto/Calendar.h>
// #include <proto/Calltips.h>
#include <proto/camd.h>
#include <proto/cardres.h>
#include <proto/cia.h>
#include <proto/Clock.h>
#include <proto/clocksource.h>
#include <proto/codesets.h>
// #include <proto/Coloradjust.h>
// #include <proto/Colorfield.h>
#include <proto/colorwheel.h>
#include <proto/commodities.h>
#include <proto/console.h>
#include <proto/coolimages.h>
// #include <proto/Crawling.h>
#include <proto/cybergraphics.h>
#include <proto/datatypes.h>
#include <proto/debug.h>
// #include <proto/Dirlist.h>
#include <proto/diskfont.h>
// #include <proto/disk.h>
#include <proto/diskimage.h>
#include <proto/dos.h>
#include <proto/dtclass.h>
// #include <proto/Dtpic.h>
#include <proto/dxtn.h>
#include <proto/efi.h>
#include <proto/exec.h>
#include <proto/execlock.h>
// #include <proto/exec_sysbase.h>
#include <proto/expansion.h>
#include <proto/expat.h>
// #include <proto/Floattext.h>
#include <proto/freetype2.h>
#include <proto/gadtools.h>
#include <proto/gallium.h>
#include <proto/gameport.h>
#include <proto/gl.h>
#include <proto/glu.h>
#include <proto/Graph.h>
#include <proto/graphics.h>
#include <proto/hostlib.h>
#include <proto/hpet.h>
#include <proto/IconDrawerList.h>
#include <proto/icon.h>
#include <proto/IconImage.h>
#include <proto/IconList.h>
#include <proto/IconListview.h>
#include <proto/IconVolumeList.h>
#include <proto/identify.h>
#include <proto/iffparse.h>
#include <proto/input.h>
#include <proto/intuition.h>
#include <proto/jfif.h>
#include <proto/kernel.h>
#include <proto/keymap.h>
#include <proto/kms.h>
// #include <proto/Knob.h>
// #include <proto/Lamp.h>
#include <proto/layers.h>
#include <proto/lddemon.h>
// #include <proto/Levelmeter.h>
// #include <proto/Listtree.h>
#include <proto/locale.h>
#include <proto/lowlevel.h>
#include <proto/mathffp.h>
#include <proto/mathieeedoubbas.h>
#include <proto/mathieeedoubtrans.h>
#include <proto/mathieeesingbas.h>
#include <proto/mathieeesingtrans.h>
#include <proto/mathtrans.h>
#include <proto/miami.h>
#include <proto/miamipanel.h>
#include <proto/misc.h>
#include <proto/muimaster.h>
#include <proto/muiscreen.h>
#include <proto/nonvolatile.h>
#include <proto/nvdisk.h>
#include <proto/oop.h>
#include <proto/openfirmware.h>
#include <proto/openurl.h>
// #include <proto/Palette.h>
#include <proto/partition.h>
#include <proto/pccard.h>
#include <proto/pngdt.h>
#include <proto/png.h>
// #include <proto/Popasl.h>
// #include <proto/Popframe.h>
// #include <proto/Popimage.h>
// #include <proto/Poplist.h>
// #include <proto/Poppen.h>
// #include <proto/Popscreen.h>
#include <proto/popupmenu.h>
#include <proto/poseidon.h>
// #include <proto/posixc.h>
// #include <proto/potgo.h>
#include <proto/PrefsEditor.h>
#include <proto/PrefsWindow.h>
// #include <proto/Process.h>
#include <proto/processor.h>
#include <proto/prometheus.h>
// #include <proto/Radio.h>
#include <proto/ramdrive.h>
// #include <proto/Rawimage.h>
#include <proto/realtime.h>
#include <proto/reqtools.h>
#include <proto/rexxsupport.h>
#include <proto/rexxsyslib.h>
#include <proto/rtas.h>
// #include <proto/Scrollgroup.h>
#include <proto/scsi.h>
// #include <proto/Settingsgroup.h>
#include <proto/socket.h>
// #include <proto/stdc.h>
// #include <proto/stdcio.h>
#include <proto/SystemPrefsWindow.h>
#include <proto/task.h>
#include <proto/timer.h>
// #include <proto/Title.h>
// #include <proto/translator.h>
#include <proto/usbclass.h>
#include <proto/utility.h>
#include <proto/uuid.h>
#include <proto/version.h>
// #include <proto/Virtgroup.h>
// #include <proto/Volumelist.h>
#include <proto/wb.h>
#include <proto/workbench.h>
#include <proto/x11gfx.h>
#include <proto/z1.h>

#include <pthread.h>

#include <resources/battclock.h>
#include <resources/card.h>
#include <resources/cia.h>
#include <resources/clocksource.h>
#include <resources/disk.h>
#include <resources/efi.h>
#include <resources/execlock.h>
#include <resources/filesysres.h>
#include <resources/hpet.h>
#include <resources/isapnp.h>
#include <resources/kernel.h>
#include <resources/lddemon.h>
#include <resources/misc.h>
#include <resources/pit.h>
#include <resources/potgo.h>
#include <resources/processor.h>
#include <resources/task.h>

#include <rexx/errors.h>
#include <rexx/rexxcall.h>
#include <rexx/rexxcall-m68k.h>
#include <rexx/rxslib.h>
#include <rexx/storage.h>

#include <sched.h>

#include <scsi/commands.h>
#include <scsi/values.h>

#include <SDI/SDI_compiler.h>
#include <SDI/SDI_hook.h>
#include <SDI/SDI_interrupt.h>
// #include <SDI/SDI_lib.h>
#include <SDI/SDI_misc.h>
#include <SDI/SDI_stdarg.h>

#include <semaphore.h>

#include <sys/cdefs.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <syslog.h>
// #include <sys/net_types.h>
#include <sys/queue.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/syslog.h>
#include <sys/un.h>

#include <turboprint.h>

#include <usb/hid.h>
#include <usb/mstorage.h>
#include <usb/usb_core.h>
#include <usb/usb.h>

#include <utility/date.h>
#include <utility/hooks.h>
#include <utility/name.h>
#include <utility/pack.h>
#include <utility/tagitem.h>
#include <utility/utility.h>

#include <vulkan/vk_icd.h>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan.h>

#include <workbench/handler.h>
#include <workbench/icon.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>

#include <zconf.h>
#include <zlib.h>

#include <zune/aboutwindow.h>
#include <zune/calendar.h>
#include <zune/clock.h>
#include <zune/customclasses.h>
#include <zune/graph.h>
#include <zune/iconimage.h>
#include <zune/loginwindow.h>
#include <zune/prefseditor.h>
#include <zune/prefswindow.h>
#include <zune/systemprefswindow.h>

int main(void)
{
    return 0;
}
