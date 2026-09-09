#include <drivers/arm/mhu.h>
#include <mm/core_memprot.h>
#include <initcall.h>

static struct io_pa_va mhu_pbx = {
	.pa = VEXPRESS_MHU_PBX_BASE,
};

static struct io_pa_va mhu_mbx = {
	.pa = VEXPRESS_MHU_MBX_BASE,
};

static TEE_Result platform_mhu_init(void)
{
	vaddr_t pbx;
	vaddr_t mbx;

	pbx = io_pa_or_va_secure(&mhu_pbx, MHU_V3_X_FRAME_SIZE);
	mbx = io_pa_or_va_secure(&mhu_mbx, MHU_V3_X_FRAME_SIZE);

	mhu_init_sender((uintptr_t)pbx);
	mhu_init_receiver((uintptr_t)mbx);
}
driver_init(platform_mhu_init);