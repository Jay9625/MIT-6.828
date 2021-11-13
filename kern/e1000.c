#include <kern/e1000.h>
#include <kern/pmap.h>

volatile uint32_t* pci_e1000;

void e1000_transmit_init() {
    memset(tx_list, 0, sizeof(struct tx_desc) * TX_MAX);
    memset(tx_buf, 0, sizeof(struct packet) * TX_MAX);
    for (int i = 0; i < TX_MAX; ++i) {
        tx_list[i].addr = PADDR(tx_buf[i].buf);
        tx_list[i].cmd = (E1000_TX)
    }
}

// LAB 6: Your driver code here
int pci_e1000_attach(struct pci_func* pcif) {
    int r;
    
    pci_func_enable(pcif);
    pci_e1000 = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
    cprintf("E1000 status: [%08x]\n", pci_e1000[E1000_STATUS >> 2]);
    return 0;
}