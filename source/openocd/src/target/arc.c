/***************************************************************************
 *   Copyright (C) 2013-2014 Synopsys, Inc.                                *
 *   Frank Dols <frank.dols@synopsys.com>                                  *
 *   Anton Kolesov <anton.kolesov@synopsys.com>                            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.           *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "arc.h"
#include "quark_se.h"


int quark_se_arc_write_memory(struct target *t, uint32_t addr, uint32_t size,
			uint32_t count, const uint8_t *buf)
{
	LOG_DEBUG("addr=0x%08" PRIx32 ", size=%" PRIu32 ", count=0x%" PRIx32 ", buf=%p",
				addr, size, count, buf);

	if (t->state != TARGET_HALTED) {
		LOG_WARNING("target not halted");
		return ERROR_TARGET_NOT_HALTED;
	}
	if (!count || !buf || !addr) {
		LOG_ERROR("%s invalid params count=0x%" PRIx32 ", buf=%p, addr=0x%08" PRIx32,
					__func__, count, buf, addr);
		return ERROR_COMMAND_ARGUMENT_INVALID;
	}

	if (((addr >= FLASH0_BASE_ADDR) && (addr <= FLASH0_LIMT))
			|| ((addr >= FLASH1_BASE_ADDR) && (addr <= FLASH1_LIMT))
			|| ((addr >= ROM_BASE_ADDR) && (addr <= ROM_LIMIT))) {
		/* Quark SE FLASH&ROM WRITE */
		return quark_se_flash_write(t, addr, size, count, buf);
	} else {
		/* Quark SE SRAM WRITE */
		return arc_mem_write(t, addr, size, count, buf);
	}

}

int quark_se_arc_assert_reset(struct target *target)
{
	/* halt target before to restore memory */
	if (target->state == TARGET_RUNNING) {
		if (target_halt(target) != ERROR_OK) {
			LOG_ERROR("%s could not halt target and therefore, "
					"breakpoints in flash may not be deleted", __func__);
		}
	}
	/* restore memory and delete breakpoints */
	arc_dbg_reset_breakpoints_watchpoints(target);
	/* do generic arc reset */
	return arc_ocd_assert_reset(target);
}

struct target_type arc32_target = {
	.name = "arc32",

	.poll =	arc_ocd_poll,

	.arch_state = arc32_arch_state,

	.target_request_data = arc_trgt_request_data,

	.halt = arc_dbg_halt,
	.resume = arc_dbg_resume,
	.step = arc_dbg_step,

	.assert_reset = quark_se_arc_assert_reset,
	.deassert_reset = arc_ocd_deassert_reset,

	.soft_reset_halt = arc_core_soft_reset_halt,

	.get_gdb_reg_list = arc_regs_get_gdb_reg_list,

	.read_memory = arc_mem_read,
	.write_memory = quark_se_arc_write_memory,
	.checksum_memory = arc_mem_checksum,
	.blank_check_memory = arc_mem_blank_check,

	.add_breakpoint = arc_dbg_add_breakpoint,
	.add_context_breakpoint = arc_dbg_add_context_breakpoint,
	.add_hybrid_breakpoint = arc_dbg_add_hybrid_breakpoint,
	.remove_breakpoint = arc_dbg_remove_breakpoint,
	.add_watchpoint = arc_dbg_add_watchpoint,
	.remove_watchpoint = arc_dbg_remove_watchpoint,

	.run_algorithm = arc_mem_run_algorithm,
	.start_algorithm = arc_mem_start_algorithm,
	.wait_algorithm = arc_mem_wait_algorithm,

	.commands = arc_monitor_command_handlers, /* see: arc_mntr.c|.h */

	.target_create = arc_ocd_target_create,
	.init_target = arc_ocd_init_target,
	.examine = arc_ocd_examine,

	.virt2phys = arc_mem_virt2phys,
	.read_phys_memory = arc_mem_read_phys_memory,
	.write_phys_memory = arc_mem_write_phys_memory,
	.mmu = arc_mem_mmu,
};
