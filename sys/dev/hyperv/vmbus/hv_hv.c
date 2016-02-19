/*-
 * Copyright (c) 2009-2012 Microsoft Corp.
 * Copyright (c) 2012 NetApp Inc.
 * Copyright (c) 2012 Citrix Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * Implements low-level interactions with Hypver-V/Azure
 */
#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/pcpu.h>
#include <sys/timetc.h>
#include <machine/bus.h>
#include <machine/md_var.h>
#include <vm/vm.h>
#include <vm/vm_param.h>
#include <vm/pmap.h>


#include "hv_vmbus_priv.h"

#define HV_NANOSECONDS_PER_SEC		1000000000L


static u_int hv_get_timecount(struct timecounter *tc);

/**
 * Globals
 */
hv_vmbus_context hv_vmbus_g_context = {
	.syn_ic_initialized = FALSE,
	.hypercall_page = NULL,
};

static struct timecounter hv_timecounter = {
	hv_get_timecount, 0, ~0u, HV_NANOSECONDS_PER_SEC/100, "Hyper-V", HV_NANOSECONDS_PER_SEC/100
};

static u_int
hv_get_timecount(struct timecounter *tc)
{
	u_int now = rdmsr(HV_X64_MSR_TIME_REF_COUNT);
	return (now);
}

/**
 * @brief Query the cpuid for presence of windows hypervisor
 */
int
hv_vmbus_query_hypervisor_presence(void) 
{
	if (vm_guest != VM_GUEST_HV)
		return (0);

	return (hv_high >= HV_X64_CPUID_MIN && hv_high <= HV_X64_CPUID_MAX);
}

/**
 * @brief Get version of the windows hypervisor
 */
static int
hv_vmbus_get_hypervisor_version(void) 
{
	u_int regs[4];
	unsigned int maxLeaf;
	unsigned int op;

	/*
	 * Its assumed that this is called after confirming that
	 * Viridian is present
	 * Query id and revision.
	 */
	op = HV_CPU_ID_FUNCTION_HV_VENDOR_AND_MAX_FUNCTION;
	do_cpuid(op, regs);

	maxLeaf = regs[0];
	op = HV_CPU_ID_FUNCTION_HV_INTERFACE;
	do_cpuid(op, regs);

	if (maxLeaf >= HV_CPU_ID_FUNCTION_MS_HV_VERSION) {
	    op = HV_CPU_ID_FUNCTION_MS_HV_VERSION;
	    do_cpuid(op, regs);
	}
	return (maxLeaf);
}

/**
 * @brief Invoke the specified hypercall
 */
static uint64_t
hv_vmbus_do_hypercall(uint64_t control, void* input, void* output)
{
#ifdef __x86_64__
	uint64_t hv_status = 0;
	uint64_t input_address = (input) ? hv_get_phys_addr(input) : 0;
	uint64_t output_address = (output) ? hv_get_phys_addr(output) : 0;
	volatile void* hypercall_page = hv_vmbus_g_context.hypercall_page;

	__asm__ __volatile__ ("mov %0, %%r8" : : "r" (output_address): "r8");
	__asm__ __volatile__ ("call *%3" : "=a"(hv_status):
				"c" (control), "d" (input_address),
				"m" (hypercall_page));
	return (hv_status);
#else
	uint32_t control_high = control >> 32;
	uint32_t control_low = control & 0xFFFFFFFF;
	uint32_t hv_status_high = 1;
	uint32_t hv_status_low = 1;
	uint64_t input_address = (input) ? hv_get_phys_addr(input) : 0;
	uint32_t input_address_high = input_address >> 32;
	uint32_t input_address_low = input_address & 0xFFFFFFFF;
	uint64_t output_address = (output) ? hv_get_phys_addr(output) : 0;
	uint32_t output_address_high = output_address >> 32;
	uint32_t output_address_low = output_address & 0xFFFFFFFF;
	volatile void* hypercall_page = hv_vmbus_g_context.hypercall_page;

	__asm__ __volatile__ ("call *%8" : "=d"(hv_status_high),
				"=a"(hv_status_low) : "d" (control_high),
				"a" (control_low), "b" (input_address_high),
				"c" (input_address_low),
				"D"(output_address_high),
				"S"(output_address_low), "m" (hypercall_page));
	return (hv_status_low | ((uint64_t)hv_status_high << 32));
#endif /* __x86_64__ */
}

/**
 *  @brief Main initialization routine.
 *
 *  This routine must be called
 *  before any other routines in here are called
 */
int
hv_vmbus_init(void) 
{
	int					max_leaf;
	hv_vmbus_x64_msr_hypercall_contents	hypercall_msr;
	void* 					virt_addr = 0;

	memset(
	    hv_vmbus_g_context.syn_ic_event_page,
	    0,
	    sizeof(hv_vmbus_handle) * MAXCPU);

	memset(
	    hv_vmbus_g_context.syn_ic_msg_page,
	    0,
	    sizeof(hv_vmbus_handle) * MAXCPU);

	if (vm_guest != VM_GUEST_HV)
	    goto cleanup;

	max_leaf = hv_vmbus_get_hypervisor_version();

	/*
	 * Write our OS info
	 */
	uint64_t os_guest_info = HV_FREEBSD_GUEST_ID;
	wrmsr(HV_X64_MSR_GUEST_OS_ID, os_guest_info);
	hv_vmbus_g_context.guest_id = os_guest_info;

	/*
	 * See if the hypercall page is already set
	 */
	hypercall_msr.as_uint64_t = rdmsr(HV_X64_MSR_HYPERCALL);
	virt_addr = malloc(PAGE_SIZE, M_DEVBUF, M_NOWAIT | M_ZERO);
	KASSERT(virt_addr != NULL,
	    ("Error VMBUS: malloc failed to allocate page during init!"));
	if (virt_addr == NULL)
	    goto cleanup;

	hypercall_msr.u.enable = 1;
	hypercall_msr.u.guest_physical_address =
	    (hv_get_phys_addr(virt_addr) >> PAGE_SHIFT);
	wrmsr(HV_X64_MSR_HYPERCALL, hypercall_msr.as_uint64_t);

	/*
	 * Confirm that hypercall page did get set up
	 */
	hypercall_msr.as_uint64_t = 0;
	hypercall_msr.as_uint64_t = rdmsr(HV_X64_MSR_HYPERCALL);

	if (!hypercall_msr.u.enable)
	    goto cleanup;

	hv_vmbus_g_context.hypercall_page = virt_addr;

	tc_init(&hv_timecounter); /* register virtual timecount */

	hv_et_init();
	
	return (0);

	cleanup:
	if (virt_addr != NULL) {
	    if (hypercall_msr.u.enable) {
		hypercall_msr.as_uint64_t = 0;
		wrmsr(HV_X64_MSR_HYPERCALL,
					hypercall_msr.as_uint64_t);
	    }

	    free(virt_addr, M_DEVBUF);
	}
	return (ENOTSUP);
}

/**
 * @brief Cleanup routine, called normally during driver unloading or exiting
 */
void
hv_vmbus_cleanup(void) 
{
	hv_vmbus_x64_msr_hypercall_contents hypercall_msr;

	if (hv_vmbus_g_context.guest_id == HV_FREEBSD_GUEST_ID) {
	    if (hv_vmbus_g_context.hypercall_page != NULL) {
		hypercall_msr.as_uint64_t = 0;
		wrmsr(HV_X64_MSR_HYPERCALL,
					hypercall_msr.as_uint64_t);
		free(hv_vmbus_g_context.hypercall_page, M_DEVBUF);
		hv_vmbus_g_context.hypercall_page = NULL;
	    }
	}
}

/**
 * @brief Post a message using the hypervisor message IPC.
 * (This involves a hypercall.)
 */
hv_vmbus_status
hv_vmbus_post_msg_via_msg_ipc(
	hv_vmbus_connection_id	connection_id,
	hv_vmbus_msg_type	message_type,
	void*			payload,
	size_t			payload_size)
{
	struct alignedinput {
	    uint64_t alignment8;
	    hv_vmbus_input_post_message msg;
	};

	hv_vmbus_input_post_message*	aligned_msg;
	hv_vmbus_status 		status;
	size_t				addr;

	if (payload_size > HV_MESSAGE_PAYLOAD_BYTE_COUNT)
	    return (EMSGSIZE);

	addr = (size_t) malloc(sizeof(struct alignedinput), M_DEVBUF,
			    M_ZERO | M_NOWAIT);
	KASSERT(addr != 0,
	    ("Error VMBUS: malloc failed to allocate message buffer!"));
	if (addr == 0)
	    return (ENOMEM);

	aligned_msg = (hv_vmbus_input_post_message*)
	    (HV_ALIGN_UP(addr, HV_HYPERCALL_PARAM_ALIGN));

	aligned_msg->connection_id = connection_id;
	aligned_msg->message_type = message_type;
	aligned_msg->payload_size = payload_size;
	memcpy((void*) aligned_msg->payload, payload, payload_size);

	status = hv_vmbus_do_hypercall(
		    HV_CALL_POST_MESSAGE, aligned_msg, 0) & 0xFFFF;

	free((void *) addr, M_DEVBUF);
	return (status);
}

/**
 * @brief Signal an event on the specified connection using the hypervisor
 * event IPC. (This involves a hypercall.)
 */
hv_vmbus_status
hv_vmbus_signal_event(void *con_id)
{
	hv_vmbus_status status;

	status = hv_vmbus_do_hypercall(
		    HV_CALL_SIGNAL_EVENT,
		    con_id,
		    0) & 0xFFFF;

	return (status);
}

/**
 * @brief hv_vmbus_synic_init
 */
void
hv_vmbus_synic_init(void *arg)

{
	int			cpu;
	uint64_t		hv_vcpu_index;
	hv_vmbus_synic_simp	simp;
	hv_vmbus_synic_siefp	siefp;
	hv_vmbus_synic_scontrol sctrl;
	hv_vmbus_synic_sint	shared_sint;
	uint64_t		version;
	hv_setup_args* 		setup_args = (hv_setup_args *)arg;

	cpu = PCPU_GET(cpuid);

	if (hv_vmbus_g_context.hypercall_page == NULL)
	    return;

	/*
	 * TODO: Check the version
	 */
	version = rdmsr(HV_X64_MSR_SVERSION);
	
	hv_vmbus_g_context.syn_ic_msg_page[cpu] =
	    setup_args->page_buffers[2 * cpu];
	hv_vmbus_g_context.syn_ic_event_page[cpu] =
	    setup_args->page_buffers[2 * cpu + 1];

	/*
	 * Setup the Synic's message page
	 */

	simp.as_uint64_t = rdmsr(HV_X64_MSR_SIMP);
	simp.u.simp_enabled = 1;
	simp.u.base_simp_gpa = ((hv_get_phys_addr(
	    hv_vmbus_g_context.syn_ic_msg_page[cpu])) >> PAGE_SHIFT);

	wrmsr(HV_X64_MSR_SIMP, simp.as_uint64_t);

	/*
	 * Setup the Synic's event page
	 */
	siefp.as_uint64_t = rdmsr(HV_X64_MSR_SIEFP);
	siefp.u.siefp_enabled = 1;
	siefp.u.base_siefp_gpa = ((hv_get_phys_addr(
	    hv_vmbus_g_context.syn_ic_event_page[cpu])) >> PAGE_SHIFT);

	wrmsr(HV_X64_MSR_SIEFP, siefp.as_uint64_t);

	/*HV_SHARED_SINT_IDT_VECTOR + 0x20; */
	shared_sint.as_uint64_t = 0;
	shared_sint.u.vector = setup_args->vector;
	shared_sint.u.masked = FALSE;
	shared_sint.u.auto_eoi = TRUE;

	wrmsr(HV_X64_MSR_SINT0 + HV_VMBUS_MESSAGE_SINT,
	    shared_sint.as_uint64_t);

	/* Enable the global synic bit */
	sctrl.as_uint64_t = rdmsr(HV_X64_MSR_SCONTROL);
	sctrl.u.enable = 1;

	wrmsr(HV_X64_MSR_SCONTROL, sctrl.as_uint64_t);

	hv_vmbus_g_context.syn_ic_initialized = TRUE;

	/*
	 * Set up the cpuid mapping from Hyper-V to FreeBSD.
	 * The array is indexed using FreeBSD cpuid.
	 */
	hv_vcpu_index = rdmsr(HV_X64_MSR_VP_INDEX);
	hv_vmbus_g_context.hv_vcpu_index[cpu] = (uint32_t)hv_vcpu_index;

	return;
}

/**
 * @brief Cleanup routine for hv_vmbus_synic_init()
 */
void hv_vmbus_synic_cleanup(void *arg)
{
	hv_vmbus_synic_sint	shared_sint;
	hv_vmbus_synic_simp	simp;
	hv_vmbus_synic_siefp	siefp;

	if (!hv_vmbus_g_context.syn_ic_initialized)
	    return;

	shared_sint.as_uint64_t = rdmsr(
	    HV_X64_MSR_SINT0 + HV_VMBUS_MESSAGE_SINT);

	shared_sint.u.masked = 1;

	/*
	 * Disable the interrupt
	 */
	wrmsr(
	    HV_X64_MSR_SINT0 + HV_VMBUS_MESSAGE_SINT,
	    shared_sint.as_uint64_t);

	simp.as_uint64_t = rdmsr(HV_X64_MSR_SIMP);
	simp.u.simp_enabled = 0;
	simp.u.base_simp_gpa = 0;

	wrmsr(HV_X64_MSR_SIMP, simp.as_uint64_t);

	siefp.as_uint64_t = rdmsr(HV_X64_MSR_SIEFP);
	siefp.u.siefp_enabled = 0;
	siefp.u.base_siefp_gpa = 0;

	wrmsr(HV_X64_MSR_SIEFP, siefp.as_uint64_t);
}

