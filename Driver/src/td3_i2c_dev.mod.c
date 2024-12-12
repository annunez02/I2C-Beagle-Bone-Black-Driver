#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

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
	{ 0x3726c6aa, "module_layout" },
	{ 0x51a910c0, "arm_copy_to_user" },
	{ 0x88db9f48, "__check_object_size" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0x86332725, "__stack_chk_fail" },
	{ 0x49970de8, "finish_wait" },
	{ 0x647af474, "prepare_to_wait_event" },
	{ 0x1000e51, "schedule" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0x37a0cba, "kfree" },
	{ 0x2d6fcc06, "__kmalloc" },
	{ 0xf8252269, "platform_driver_unregister" },
	{ 0x98171fcf, "device_destroy" },
	{ 0x8900a5ba, "__platform_driver_register" },
	{ 0x5abf8d2b, "class_destroy" },
	{ 0x8781d48, "device_create" },
	{ 0x77abe509, "cdev_del" },
	{ 0xbf451cca, "__class_create" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x83c50091, "cdev_add" },
	{ 0x2f31c9f4, "cdev_init" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0xa5ad86c9, "cdev_alloc" },
	{ 0x3dcf1ffa, "__wake_up" },
	{ 0x9c6febfc, "add_uevent_var" },
	{ 0x92d5838e, "request_threaded_irq" },
	{ 0xd81c5ffc, "platform_get_irq" },
	{ 0xf9a482f9, "msleep" },
	{ 0xe97c4103, "ioremap" },
	{ 0x822137e2, "arm_heavy_mb" },
	{ 0xc5850110, "printk" },
	{ 0xedc03953, "iounmap" },
	{ 0xc1514a3b, "free_irq" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("of:N*T*Ctd3_ani");
MODULE_ALIAS("of:N*T*Ctd3_aniC*");

MODULE_INFO(srcversion, "EF42343E5CC14F42F26B962");
