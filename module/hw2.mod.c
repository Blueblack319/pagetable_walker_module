#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/export-internal.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif


static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x82ee90dc, "timer_delete_sync" },
	{ 0x45db931b, "proc_create" },
	{ 0x2364c85a, "tasklet_init" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0x15ba50a6, "jiffies" },
	{ 0x24d273d1, "add_timer" },
	{ 0x9d2ab8ac, "__tasklet_schedule" },
	{ 0xd1d6e0b7, "boot_cpu_data" },
	{ 0xdad13544, "ptrs_per_p4d" },
	{ 0xbec1f61d, "pv_ops" },
	{ 0x1d19f77b, "physical_mask" },
	{ 0x7cd8d75e, "page_offset_base" },
	{ 0x72d79d83, "pgdir_shift" },
	{ 0x8a35b432, "sme_me_mask" },
	{ 0x97651e6c, "vmemmap_base" },
	{ 0xba8fbd64, "_raw_spin_lock" },
	{ 0xb5b54b34, "_raw_spin_unlock" },
	{ 0xa648e561, "__ubsan_handle_shift_out_of_bounds" },
	{ 0x554bf6bd, "__tracepoint_mmap_lock_start_locking" },
	{ 0x668b19a1, "down_read" },
	{ 0x6839cbfc, "__tracepoint_mmap_lock_acquire_returned" },
	{ 0x9acf31c6, "mas_find" },
	{ 0x73e2cb, "__tracepoint_mmap_lock_released" },
	{ 0x53b954a2, "up_read" },
	{ 0xe7b34410, "__mmap_lock_do_trace_start_locking" },
	{ 0xc268c634, "__mmap_lock_do_trace_acquire_returned" },
	{ 0xfd68a473, "__mmap_lock_do_trace_released" },
	{ 0xa19b956, "__stack_chk_fail" },
	{ 0x854ad34a, "init_task" },
	{ 0xc38c83b8, "mod_timer" },
	{ 0xd8e57183, "seq_read" },
	{ 0x6e7fd3b8, "seq_lseek" },
	{ 0x8c5a766b, "seq_release" },
	{ 0xbdfb6dbb, "__fentry__" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0xb12fd881, "seq_open" },
	{ 0x34db050b, "_raw_spin_lock_irqsave" },
	{ 0xd65b4e21, "seq_printf" },
	{ 0xd35cce70, "_raw_spin_unlock_irqrestore" },
	{ 0x87a21cb3, "__ubsan_handle_out_of_bounds" },
	{ 0xc4f0da12, "ktime_get_with_offset" },
	{ 0xac0dde12, "remove_proc_entry" },
	{ 0xea3c74e, "tasklet_kill" },
	{ 0x453e7dc, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "9A7126C1ED6024F0268A1C3");
