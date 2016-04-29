/*
 * File		pci-acpi.h
 *
 * Copyright (C) 2004 Intel
 * Copyright (C) Tom Long Nguyen (tom.l.nguyen@intel.com)
 */

#ifndef _PCI_ACPI_H_
#define _PCI_ACPI_H_

#include <linux/acpi.h>

#ifdef CONFIG_ACPI
extern acpi_status pci_acpi_add_bus_pm_notifier(struct acpi_device *dev);
static inline acpi_status pci_acpi_remove_bus_pm_notifier(struct acpi_device *dev)
{
	return acpi_remove_pm_notifier(dev);
}
extern acpi_status pci_acpi_add_pm_notifier(struct acpi_device *dev,
					     struct pci_dev *pci_dev);
static inline acpi_status pci_acpi_remove_pm_notifier(struct acpi_device *dev)
{
	return acpi_remove_pm_notifier(dev);
}
extern int acpi_pci_bus_domain_nr(struct device *parent);
extern phys_addr_t acpi_pci_root_get_mcfg_addr(acpi_handle handle);

static inline acpi_handle acpi_find_root_bridge_handle(struct pci_dev *pdev)
{
	struct pci_bus *pbus = pdev->bus;

	/* Find a PCI root bus */
	while (!pci_is_root_bus(pbus))
		pbus = pbus->parent;

	return ACPI_HANDLE(pbus->bridge);
}

static inline acpi_handle acpi_pci_get_bridge_handle(struct pci_bus *pbus)
{
	struct device *dev;

	if (pci_is_root_bus(pbus))
		dev = pbus->bridge;
	else {
		/* If pbus is a virtual bus, there is no bridge to it */
		if (!pbus->self)
			return NULL;

		dev = &pbus->self->dev;
	}

	return ACPI_HANDLE(dev);
}

struct acpi_pci_root;
struct acpi_pci_root_ops;

struct acpi_pci_root_info {
	struct acpi_pci_root		*root;
	struct acpi_device		*bridge;
	struct acpi_pci_root_ops	*ops;
	struct list_head		resources;
	char				name[16];
};

struct acpi_pci_root_ops {
	struct pci_ops *pci_ops;
	int (*init_info)(struct acpi_pci_root_info *info);
	void (*release_info)(struct acpi_pci_root_info *info);
	int (*prepare_resources)(struct acpi_pci_root_info *info);
};

extern int acpi_pci_probe_root_resources(struct acpi_pci_root_info *info);
extern struct pci_bus *acpi_pci_root_create(struct acpi_pci_root *root,
					    struct acpi_pci_root_ops *ops,
					    struct acpi_pci_root_info *info,
					    void *sd);

void acpi_pci_add_bus(struct pci_bus *bus);
void acpi_pci_remove_bus(struct pci_bus *bus);

#ifdef	CONFIG_ACPI_PCI_SLOT
void acpi_pci_slot_init(void);
void acpi_pci_slot_enumerate(struct pci_bus *bus);
void acpi_pci_slot_remove(struct pci_bus *bus);
#else
static inline void acpi_pci_slot_init(void) { }
static inline void acpi_pci_slot_enumerate(struct pci_bus *bus) { }
static inline void acpi_pci_slot_remove(struct pci_bus *bus) { }
#endif

#ifdef	CONFIG_HOTPLUG_PCI_ACPI
void acpiphp_init(void);
void acpiphp_enumerate_slots(struct pci_bus *bus);
void acpiphp_remove_slots(struct pci_bus *bus);
void acpiphp_check_host_bridge(struct acpi_device *adev);
#else
static inline void acpiphp_init(void) { }
static inline void acpiphp_enumerate_slots(struct pci_bus *bus) { }
static inline void acpiphp_remove_slots(struct pci_bus *bus) { }
static inline void acpiphp_check_host_bridge(struct acpi_device *adev) { }
#endif

extern const u8 pci_acpi_dsm_uuid[];
#define DEVICE_LABEL_DSM	0x07
#define RESET_DELAY_DSM		0x08
#define FUNCTION_DELAY_DSM	0x09

/* common API to maintain list of MCFG regions */
/* "PCI MMCONFIG %04x [bus %02x-%02x]" */
#define PCI_MMCFG_RESOURCE_NAME_LEN (22 + 4 + 2 + 2)

struct pci_mmcfg_region {
	struct list_head list;
	struct resource res;
	u64 address;
	char __iomem *virt;
	u16 segment;
	u8 start_bus;
	u8 end_bus;
	char name[PCI_MMCFG_RESOURCE_NAME_LEN];
	bool hot_added;
};

struct pci_mcfg_fixup {
	const struct dmi_system_id *system;
	int (*match)(struct pci_mcfg_fixup *, struct acpi_pci_root *);
	struct pci_ops *ops;
	int domain;
	int bus_num;
};

#define PCI_MCFG_DOMAIN_ANY	-1
#define PCI_MCFG_BUS_ANY	-1

/* Designate a routine to fix up buggy MCFG */
#define DECLARE_ACPI_MCFG_FIXUP(system, match, ops, dom, bus)		\
	static const struct pci_mcfg_fixup __mcfg_fixup_##system##dom##bus\
	 __used	__attribute__((__section__(".acpi_fixup_mcfg"),		\
				aligned((sizeof(void *))))) =		\
	{ system, match, ops, dom, bus };

extern struct pci_mmcfg_region *pci_mmconfig_lookup(int segment, int bus);
extern struct pci_mmcfg_region *pci_mmconfig_add(int segment, int start,
							int end, u64 addr);
extern int pci_mmconfig_map_resource(struct device *dev,
	struct pci_mmcfg_region *mcfg);
extern void pci_mmconfig_unmap_resource(struct pci_mmcfg_region *mcfg);
extern int pci_mmconfig_enabled(void);
extern int __init pci_mmconfig_parse_table(void);

extern struct list_head pci_mmcfg_list;

#define PCI_MMCFG_BUS_OFFSET(bus)      ((bus) << 20)
#define PCI_MMCFG_OFFSET(bus, devfn)   ((bus) << 20 | (devfn) << 12)

#ifdef	CONFIG_PCI_MMCONFIG
extern int pci_mmconfig_insert(struct device *dev, u16 seg, u8 start, u8 end,
			       phys_addr_t addr);
extern int pci_mmconfig_delete(u16 seg, u8 start, u8 end);
extern struct pci_ops *pci_mcfg_get_ops(struct acpi_pci_root *root);
extern void __iomem *pci_mcfg_dev_base(struct pci_bus *bus, unsigned int devfn,
				       int offset);
#else
static inline int pci_mmconfig_insert(struct device *dev, u16 seg, u8 start,
				      u8 end, phys_addr_t addr) { return 0; }
static inline int pci_mmconfig_delete(u16 seg, u8 start, u8 end) { return 0; }
static inline struct pci_ops *pci_mcfg_get_ops(struct acpi_pci_root *root)
{ return NULL; }
static inline void __iomem *pci_mcfg_dev_base(struct pci_bus *bus,
					      unsigned int devfn, int offset)
{ return NULL; }
#endif

#else	/* CONFIG_ACPI */
static inline void acpi_pci_add_bus(struct pci_bus *bus) { }
static inline void acpi_pci_remove_bus(struct pci_bus *bus) { }
static inline int acpi_pci_bus_domain_nr(struct device *parent) { return -1; }
#endif	/* CONFIG_ACPI */

#ifdef CONFIG_ACPI_APEI
extern bool aer_acpi_firmware_first(void);
#else
static inline bool aer_acpi_firmware_first(void) { return false; }
#endif

#endif	/* _PCI_ACPI_H_ */
