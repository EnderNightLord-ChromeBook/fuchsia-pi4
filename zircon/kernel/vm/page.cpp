// Copyright 2016 The Fuchsia Authors
// Copyright (c) 2014 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <err.h>
#include <inttypes.h>
#include <lib/console.h>
#include <stdio.h>
#include <string.h>
#include <trace.h>
#include <vm/page.h>
#include <vm/physmap.h>
#include <vm/pmm.h>
#include <vm/vm.h>

#define LOCAL_TRACE 0

ktl::atomic<size_t> vm_page::count_by_state[VM_PAGE_STATE_COUNT_];
ktl::atomic<size_t> vm_page::count_total;

const char* page_state_to_string(unsigned int state) {
    switch (state) {
    case VM_PAGE_STATE_FREE:
        return "free";
    case VM_PAGE_STATE_ALLOC:
        return "alloc";
    case VM_PAGE_STATE_WIRED:
        return "wired";
    case VM_PAGE_STATE_HEAP:
        return "heap";
    case VM_PAGE_STATE_OBJECT:
        return "object";
    case VM_PAGE_STATE_MMU:
        return "mmu";
    case VM_PAGE_STATE_IPC:
        return "ipc";
    default:
        return "unknown";
    }
}

void vm_page::dump() const {
    printf("page %p: address %#" PRIxPTR " state %s flags %#x\n", this, paddr(),
           page_state_to_string(state_priv), flags);
}

void vm_page::set_state(vm_page_state new_state) {
    constexpr uint32_t kMask = (1 << VM_PAGE_STATE_BITS) - 1;
    DEBUG_ASSERT_MSG(new_state == (new_state & kMask), "invalid state %u\n", new_state);

    count_by_state[state_priv].fetch_sub(1, ktl::memory_order_relaxed);
    count_by_state[new_state].fetch_add(1, ktl::memory_order_relaxed);
    state_priv = (new_state & kMask);
}

size_t vm_page::get_count(vm_page_state state) {
    return count_by_state[state].load(ktl::memory_order_relaxed);
}

size_t vm_page::get_count_total() {
    return count_total.load(ktl::memory_order_relaxed);
}

void vm_page::add_to_initial_count(vm_page_state state, size_t n) {
    count_by_state[state].fetch_add(n, ktl::memory_order_relaxed);
    count_total.fetch_add(n, ktl::memory_order_relaxed);
}

static int cmd_vm_page(int argc, const cmd_args* argv, uint32_t flags) {
    if (argc < 2) {
    notenoughargs:
        printf("not enough arguments\n");
    usage:
        printf("usage:\n");
        printf("%s dump <address>\n", argv[0].str);
        printf("%s hexdump <address>\n", argv[0].str);
        return ZX_ERR_INTERNAL;
    }

    if (!strcmp(argv[1].str, "dump")) {
        if (argc < 2) {
            goto notenoughargs;
        }

        vm_page* page = reinterpret_cast<vm_page*>(argv[2].u);

        page->dump();
    } else if (!strcmp(argv[1].str, "hexdump")) {
        if (argc < 2) {
            goto notenoughargs;
        }

        vm_page* page = reinterpret_cast<vm_page*>(argv[2].u);

        paddr_t pa = page->paddr();
        void* ptr = paddr_to_physmap(pa);
        if (!ptr) {
            printf("bad page or page not mapped in kernel space\n");
            return -1;
        }
        hexdump(ptr, PAGE_SIZE);
    } else {
        printf("unknown command\n");
        goto usage;
    }

    return ZX_OK;
}

STATIC_COMMAND_START
#if LK_DEBUGLEVEL > 0
STATIC_COMMAND("vm_page", "vm_page debug commands", &cmd_vm_page)
#endif
STATIC_COMMAND_END(vm_page)
