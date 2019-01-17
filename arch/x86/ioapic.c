#include <c4/arch/mp/ioapic.h>
#include <c4/debug.h>
#include <stddef.h>

// TODO: .bss is zero'd by multiboot, right?
//       make sure to double-check this
static void *ioapics[MAX_IOAPICS];

void ioapic_add(void *ioapic, unsigned id) {
	if (id >= MAX_IOAPICS) {
		debug_printf(" - WARNING: ioapic id %u greater than MAX_IOAPICS!\n");
		return;
	}

	if (ioapics[id]) {
		debug_printf(" - WARNING: already have an ioapic with id %u!\n", id);
		return;
	}

	ioapics[id] = ioapic;
}

void *ioapic_get(ioapic_t id) {
	if (id < MAX_IOAPICS && ioapics[id]) {
		return ioapics[id];
	}

	return NULL;
}

uint32_t ioapic_version_raw(ioapic_t id) {
	void *ioapic = ioapic_get(id);

	if (!ioapic) {
		return 0;
	}

	return ioapic_read(ioapic, IOAPIC_REG_VERSION);
}

uint32_t ioapic_version(ioapic_t id) {
	return ioapic_version_raw(id) & 0xff;
}

uint32_t ioapic_max_redirects(ioapic_t id) {
	return (ioapic_version_raw(id) >> 16) & 0xff;
}

static inline bool is_valid_redirect(ioapic_t id, unsigned entry) {
	void *ioapic = ioapic_get(id);

	if (!ioapic) {
		return false;
	}

	return entry < ioapic_max_redirects(id);
}

ioapic_redirect_t ioapic_get_redirect(ioapic_t id, unsigned entry) {
	ioapic_redirect_t ret;
	void *ioapic = ioapic_get(id);

	if (!is_valid_redirect(id, entry)) {
		// TODO: error handling
		return ret;
	}

	uint32_t index = IOAPIC_REG_REDIRECT + (entry * 2);
	ret.lower = ioapic_read(ioapic, index);
	ret.upper = ioapic_read(ioapic, index + 1);

	return ret;
}

void ioapic_set_redirect(ioapic_t id,
                         unsigned entry,
                         ioapic_redirect_t *redirect)
{
	void *ioapic = ioapic_get(id);

	if (!is_valid_redirect(id, entry)) {
		// TODO: error handling
		return;
	}

	uint32_t index = IOAPIC_REG_REDIRECT + (entry * 2);
	ioapic_write(ioapic, index,     redirect->lower);
	ioapic_write(ioapic, index + 1, redirect->upper);
}

void ioapic_set_mask(ioapic_t id, unsigned entry, bool mask) {
	void *ioapic = ioapic_get(id);

	if (!ioapic) {
		return;
	}

	ioapic_redirect_t redirect = ioapic_get_redirect(id, entry);
	redirect.mask = mask;
	ioapic_set_redirect(id, entry, &redirect);
}

void ioapic_unmask(ioapic_t id, unsigned entry) {
	ioapic_set_mask(id, entry, false);
}

void ioapic_mask(ioapic_t id, unsigned entry) {
	ioapic_set_mask(id, entry, true);
}

void ioapic_print_redirect(ioapic_t id, unsigned entry) {
	if (!is_valid_redirect(id, entry)) {
		debug_printf(" - ioapic %u: invalid redirect %u\n", id, entry);
		return;
	}

	ioapic_redirect_t redirect = ioapic_get_redirect(id, entry);

	debug_printf(
		"    - lower: %x, upper: %x\n"
		"    - vector: %x\n"
		"    - delivery: %x\n"
		"    - destination mode: %x\n"
		"    - delivery status: %x\n"
		"    - polarity: %x\n"
		"    - remote irr: %x\n"
		"    - trigger mode: %x\n"
		"    - mask: %x\n"
		"    - destination: %x\n",
		redirect.lower, redirect.upper,
		redirect.vector, redirect.delivery, redirect.destination_mode,
		redirect.status, redirect.polarity, redirect.remote_irr,
		redirect.trigger, redirect.mask, redirect.destination
	);
}

void ioapic_print_redirects(ioapic_t id) {
	unsigned max_entries = ioapic_max_redirects(id);

	if (!ioapic_get(id)) {
		debug_printf(" - WARNING: Invalid ioapic %u!\n", id);
		return;
	}

	debug_printf(" - ioapic %u redirection table:\n", id);
	for (unsigned i = 0; i < max_entries; i++) {
		debug_printf(" > entry %u:\n", i);
		ioapic_print_redirect(id, i);
	}
}
