#include "Ctu.h"

/*$IPC$
partial nn::ro::detail::IRoInterface {
	class GuestNro;
	std::map<gptr, GuestNro*> nroMap;
}
*/

uint32_t nn::ro::detail::IRoInterface::Unknown4(IN uint64_t pid_placeholder, IN gpid _1, IN shared_ptr<KObject> process) {
	LOG_DEBUG(IpcStubs, "Stub implementation for nn::ro::detail::IRoInterface::Initialize");
	return 0;
}

class nn::ro::detail::IRoInterface::GuestNro {
  public:
	GuestNro(Ctu *ctu, gptr src_image_addr, gptr src_bss_addr, uint64_t nro_size, uint64_t bss_size) :
		ctu(ctu),
		src_image_addr(src_image_addr),
		src_bss_addr(src_bss_addr),
		nro_size(nro_size),
		bss_size(bss_size) {

		dst_addr = target_address;
		target_address+= nro_size + bss_size;

		char *image = new char[nro_size];
		ctu->cpu.map(dst_addr, nro_size);
		ctu->cpu.readmem(src_image_addr, image, nro_size);
		ctu->cpu.writemem(dst_addr, image, nro_size);
		ctu->cpu.map(dst_addr + nro_size, bss_size);
		ctu->cpu.unmap(src_image_addr, nro_size);
		ctu->cpu.unmap(src_bss_addr, bss_size);
	}
	~GuestNro() {
		ctu->cpu.map(src_image_addr, nro_size);
		ctu->cpu.map(src_bss_addr, bss_size);
		ctu->cpu.unmap(dst_addr, nro_size);
		ctu->cpu.unmap(dst_addr + nro_size, bss_size);
	}

	static gptr target_address;
	
	gptr dst_addr;
	Ctu *ctu;
	const gptr src_image_addr;
	const gptr src_bss_addr;
	const uint64_t nro_size;
	const uint64_t bss_size;
};
gptr nn::ro::detail::IRoInterface::GuestNro::target_address = 0x7800000000;
	
uint32_t nn::ro::detail::IRoInterface::Unknown0(IN uint64_t pid_placeholder, IN uint64_t nro_image_addr, IN uint64_t nro_size, IN uint64_t bss_addr, IN uint64_t bss_size, IN gpid _5, OUT uint64_t& nro_load_addr) {
	LOG_DEBUG(IpcStubs, "Stub implementation for nn::ro::detail::IRoInterface::LoadNro");

	GuestNro *nro = new GuestNro(ctu, nro_image_addr, bss_addr, nro_size, bss_size);
	nro_load_addr = nro->dst_addr;
	nroMap[nro->dst_addr] = nro;
	return 0;
}

uint32_t nn::ro::detail::IRoInterface::Unknown2(IN uint64_t pid_placeholder, IN uint64_t nrr_addr, IN uint64_t nrr_size, IN gpid _3) {
	LOG_DEBUG(IpcStubs, "Stub implementation for nn::ro::detail::IRoInterface::LoadNrr");
	return 0;
}

uint32_t nn::ro::detail::IRoInterface::Unknown1(IN uint64_t pid_placeholder, IN uint64_t nro_load_addr, IN gpid _2) {
	LOG_DEBUG(IpcStubs, "Stub implementation for nn::ro::detail::IRoInterface::UnloadNro");
	delete nroMap[nro_load_addr];
	return 0;
}

uint32_t nn::ro::detail::IRoInterface::Unknown3(IN uint64_t pid_placeholder, IN uint64_t nrr_addr, IN gpid _2) {
	LOG_DEBUG(IpcStubs, "Stub implementation for nn::ro::detail::IRoInterface::UnloadNrr");
	return 0;
}
