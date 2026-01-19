#include <arch/x86_64/Paging/PageTableManager.hpp>

PageTableManager::PageTableManager(uint64_t pml4_phys){
    // CRITICAL: We're still in UEFI page tables here
    // So we can access the PML4 directly via its physical address
    this->pml4_phys = pml4_phys;
    this->pml4_virt = (PageTable*)pml4_phys;

    // Clear the PML4
    for (int i = 0; i < 512; i++) {
        this->pml4_virt->entries[i].value = 0;
    }
}

PageTable* PageTableManager::GetNextTable(PageTableEntry& entry, bool user)
{
    if (!entry.getFlag(PAGE_PRESENT)) {
        // Request a new page from PMM
        uint64_t phys = (uint64_t)gPmm.requestPage();
        if (!phys) {
            if (gScreenwriter) {
                gScreenwriter->print("PageTableManager: Out of memory!\n", true);
            }
            return nullptr;
        }

        // CRITICAL: Access the page directly via physical address
        // We're building page tables while still in UEFI's page tables
        // which are identity-mapped
        PageTable* virt = (PageTable*)phys;

        // Clear the new table
        for (int i = 0; i < 512; i++) {
            virt->entries[i].value = 0;
        }

        // Set up the entry
        entry.setAddress(phys);
        entry.setFlag(PAGE_PRESENT, true);
        entry.setFlag(PAGE_WRITABLE, true);
        entry.setFlag(PAGE_USER, user);
    }

    // Return the physical address (which is identity-mapped in UEFI tables)
    return (PageTable*)entry.getAddress();
}

void PageTableManager::mapPage(
    uint64_t virt,
    uint64_t phys,
    bool writable,
    bool user,
    bool executable)
{
    // Extract page table indices from virtual address
    uint64_t i4 = (virt >> 39) & 0x1FF;  // PML4 index
    uint64_t i3 = (virt >> 30) & 0x1FF;  // PDP index
    uint64_t i2 = (virt >> 21) & 0x1FF;  // PD index
    uint64_t i1 = (virt >> 12) & 0x1FF;  // PT index

    // Walk the page table hierarchy, creating tables as needed
    PageTable* pdp = GetNextTable(pml4_virt->entries[i4], user);
    if (!pdp) {
        if (gScreenwriter) {
            gScreenwriter->print("PageTableManager: Failed to get PDP\n", true);
        }
        return;
    }

    PageTable* pd = GetNextTable(pdp->entries[i3], user);
    if (!pd) {
        if (gScreenwriter) {
            gScreenwriter->print("PageTableManager: Failed to get PD\n", true);
        }
        return;
    }

    PageTable* pt = GetNextTable(pd->entries[i2], user);
    if (!pt) {
        if (gScreenwriter) {
            gScreenwriter->print("PageTableManager: Failed to get PT\n", true);
        }
        return;
    }

    // Set up the final page table entry
    PageTableEntry& e = pt->entries[i1];
    e.setAddress(phys);
    e.setFlag(PAGE_PRESENT, true);
    e.setFlag(PAGE_WRITABLE, writable);
    e.setFlag(PAGE_USER, user);
    e.setFlag(PAGE_NO_EXECUTE, !executable);

    // Note: Don't invalidate TLB yet - we're not using these tables
    // TLB will be flushed when we switch CR3
}

void PageTableManager::unmapPage(uint64_t virt)
{
    // Extract indices
    uint64_t i4 = (virt >> 39) & 0x1FF;
    uint64_t i3 = (virt >> 30) & 0x1FF;
    uint64_t i2 = (virt >> 21) & 0x1FF;
    uint64_t i1 = (virt >> 12) & 0x1FF;

    // Check if tables exist
    if (!pml4_virt->entries[i4].getFlag(PAGE_PRESENT)) return;
    PageTable* pdp = (PageTable*)pml4_virt->entries[i4].getAddress();

    if (!pdp->entries[i3].getFlag(PAGE_PRESENT)) return;
    PageTable* pd = (PageTable*)pdp->entries[i3].getAddress();

    if (!pd->entries[i2].getFlag(PAGE_PRESENT)) return;
    PageTable* pt = (PageTable*)pd->entries[i2].getAddress();

    // Clear the entry
    pt->entries[i1].value = 0;

    // Invalidate TLB
    asm volatile("invlpg (%0)" :: "r"(virt) : "memory");
}

uint64_t PageTableManager::getPhysicalAddress(uint64_t virt)
{
    // Extract indices
    uint64_t i4 = (virt >> 39) & 0x1FF;
    uint64_t i3 = (virt >> 30) & 0x1FF;
    uint64_t i2 = (virt >> 21) & 0x1FF;
    uint64_t i1 = (virt >> 12) & 0x1FF;
    uint64_t offset = virt & 0xFFF;

    // Walk page tables
    if (!pml4_virt->entries[i4].getFlag(PAGE_PRESENT)) return 0;
    PageTable* pdp = (PageTable*)pml4_virt->entries[i4].getAddress();

    if (!pdp->entries[i3].getFlag(PAGE_PRESENT)) return 0;
    PageTable* pd = (PageTable*)pdp->entries[i3].getAddress();

    if (!pd->entries[i2].getFlag(PAGE_PRESENT)) return 0;
    PageTable* pt = (PageTable*)pd->entries[i2].getAddress();

    if (!pt->entries[i1].getFlag(PAGE_PRESENT)) return 0;

    return pt->entries[i1].getAddress() + offset;
}

uint64_t PageTableManager::getPmL4Phys() const
{
    return pml4_phys;
}
