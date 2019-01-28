// XbSymbolCacheGenTest.cpp : Defines the entry point for the console
// application.
//

#include <clocale>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdlib.h>
#include <string>

#include "SimpleIni.h"
#include "XbSymbolDatabase.h"
#include "Xbe.h"
#include "helper.hpp"
#include "xxhash.h"

#define _128_MiB 0x08000000

std::map<std::string, uint32_t> g_SymbolAddresses;
static const char *section_info = "Info";
static struct {
	const char *SymbolDatabaseVersionHash = "SymbolDatabaseVersionHash";
} sect_info_keys;

static const char *section_certificate = "Certificate";
static struct {
	const char *Name = "Name";
	const char *TitleID = "TitleID";
	const char *TitleIDHex = "TitleIDHex";
	const char *Region = "Region";
} sect_certificate_keys;

static const char *section_libs = "Libs";
static struct {
	const char *BuildVersion = "BuildVersion";
} sect_libs_keys;

static const char *section_symbols = "Symbols";

void pause_for_user_input()
{
	std::cout << "Press 'enter' key to continue...";
	(void)std::getchar();
}

extern void ScanXbe(const xbe_header *pXbeHeader, bool is_raw);

int main(int argc, char **argv)
{
	// NOTE: We are ignoring the first argument which is the executable file.
	if (argc != 2) {
		std::cout << "ERROR: Must have one argument! - " << argc << "\n";
		while (argc > 0) {
			argc--;
			std::cout << "ARG" << argc << ": " << argv[argc] << "\n";
		}
		pause_for_user_input();
		return 1;
	}

	// Fix up to use executable's folder since it is meant for portable
	// executable.
	std::string execPath = argv[0];
	execPath = execPath.substr(0, execPath.find_last_of("\\/"));
	std::filesystem::current_path(execPath);

	std::setlocale(LC_ALL, "English");

	std::ifstream xbeFile =
	    std::ifstream(std::string(argv[1]), std::ios::binary);
	if (!xbeFile.is_open()) {
		std::cout << "ERROR: Unable to open the file!\n";
		pause_for_user_input();
		return 2;
	}

	std::string fileData = std::string(std::istreambuf_iterator<char>(xbeFile),
	                                   std::istreambuf_iterator<char>());
	std::cout << "File size: " << fileData.size() << " byte(s).\n";

	std::cout << "Scanning raw xbe file...\n";

	const uint8_t *xbe_data =
	    reinterpret_cast<const uint8_t *>(fileData.data());

	const xbe_header *pXbeHeader =
	    reinterpret_cast<const xbe_header *>(xbe_data);
	ScanXbe(pXbeHeader, true);

	std::map<std::string, uint32_t> g_SymbolAddressesRaw = g_SymbolAddresses;

	std::cout << "Scanning raw xbe file... COMPLETE!\n";

	void *xb_environment = std::calloc(_128_MiB, 1);

	const uint8_t *xb_env_data =
	    reinterpret_cast<const uint8_t *>(xb_environment);

	if (xb_environment == (void *)0) {
		std::cout
		    << "ERROR: Unable to allocate 128 MiB of virtual xbox memory!\n";
		pause_for_user_input();
		return 3;
	}

	std::cout << "Loading sections into virtual xbox memory...\n";

	std::memcpy((uint8_t *)xb_environment + pXbeHeader->dwBaseAddr, pXbeHeader,
	            sizeof(xbe_header));

	if (sizeof(xbe_header) < pXbeHeader->dwSizeofHeaders) {

		uint32_t extra_size = pXbeHeader->dwSizeofHeaders - sizeof(xbe_header);
		std::memcpy((uint8_t *)xb_environment + pXbeHeader->dwBaseAddr +
		                sizeof(xbe_header),
		            (uint8_t *)xbe_data + sizeof(xbe_header), extra_size);
	}

	xbe_section_header *pSectionHeaders =
	    (xbe_section_header *)((uint8_t *)xb_environment +
	                           pXbeHeader->pSectionHeadersAddr);

	// Load sections into virtualize xbox memory
	for (uint32_t s = 0; s < pXbeHeader->dwSections; s++) {

		if (pSectionHeaders[s].dwFlags.bPreload) {

			if (pSectionHeaders[s].dwVirtualAddr +
			        pSectionHeaders[s].dwVirtualSize >
			    _128_MiB) {
				std::cout << "ERROR: section request virtual size allocation "
				             "outside 128MiB "
				             "range, skipping...\n";
				continue;
			}

			if (pSectionHeaders[s].dwVirtualAddr +
			        pSectionHeaders[s].dwSizeofRaw >
			    _128_MiB) {
				std::cout << "ERROR: section request raw size allocation "
				             "outside 128MiB "
				             "range, skipping...\n";
				continue;
			}

			std::memset((uint8_t *)xb_environment +
			                pSectionHeaders[s].dwVirtualAddr,
			            0, pSectionHeaders[s].dwVirtualSize);

			std::memcpy((uint8_t *)xb_environment +
			                pSectionHeaders[s].dwVirtualAddr,
			            xbe_data + pSectionHeaders[s].dwRawAddr,
			            pSectionHeaders[s].dwSizeofRaw);

			// Let XbSymbolDatabase know this section is loaded.
			pSectionHeaders[s].dwSectionRefCount++;
			std::cout << "Section preloaded: "
			          << (const char *)((uint8_t *)xb_environment +
			                            pSectionHeaders[s].SectionNameAddr)
			          << "\n";
		}
	}

	std::cout << "Scanning virtual xbox environment...\n";

	ScanXbe((xbe_header *)((uint8_t *)xb_environment + pXbeHeader->dwBaseAddr),
	        false);

	std::free(xb_environment);

	std::cout << "Scanning virtual xbox environment... COMPLETE!\n\n";

	std::cout << "Verifying symbols registered...\n";

	// Ensure both raw and simulated xbox environment do have symbols detected.
	if (g_SymbolAddresses.size() == 0 || g_SymbolAddressesRaw.size() == 0) {
		std::cout << "ERROR: Symbols are not detected!\n";
		pause_for_user_input();
		return 4;
	}

	// Then check both raw and simulated do indeed have same size.
	if (g_SymbolAddresses.size() != g_SymbolAddressesRaw.size()) {
		std::cout << "ERROR: Registered symbols is not even.\n"
		          << "INFO: Raw xbe: " << g_SymbolAddressesRaw.size()
		          << " - Sim xbox: " << g_SymbolAddresses.size() << "\n";
		pause_for_user_input();
		return 5;
	}
	else {
		std::cout << "INFO: Symbol registered size...OK!\n";
	}

	// Finally, check each string and addresses are the same.
	if (!std::equal(g_SymbolAddresses.begin(), g_SymbolAddresses.end(),
	                g_SymbolAddressesRaw.begin())) {
		std::cout
		    << "ERROR: Symbol registered does not match of raw vs sim xbox\n";
		pause_for_user_input();
		return 6;
	}
	else {
		std::cout << "INFO: Symbol registered matching...OK!\n";
	}

#if 0
	pause_for_user_input();
#else
	std::cout << "INFO: Scanning xbe file is completed\n";
#endif

	return 0;
}

void EmuOutputMessage(xb_output_message mFlag, const char *message)
{
	switch (mFlag) {
		case XB_OUTPUT_MESSAGE_INFO: {
			std::cout << "INFO   : " << message << std::endl;
			break;
		}
		case XB_OUTPUT_MESSAGE_WARN: {
			std::cout << "WARNING: " << message << std::endl;
			break;
		}
		case XB_OUTPUT_MESSAGE_ERROR: {
			std::cout << "ERROR  : " << message << std::endl;
			break;
		}
		case XB_OUTPUT_MESSAGE_DEBUG:
		default: {
#ifdef _DEBUG_TRACE
			std::cout << "DEBUG  : " << message << std::endl;
#endif
			break;
		}
	}
}

void EmuRegisterSymbol(const char *library_str, uint32_t library_flag,
                       const char *symbol_str, uint32_t func_addr,
                       uint32_t revision)
{
	// Ignore registered symbol in current database.
	uint32_t hasSymbol = g_SymbolAddresses[symbol_str];

	if (hasSymbol != 0) {
		return;
	}

#ifdef _DEBUG
	// Output some details
	std::stringstream output;
	output << "Symbol Generator: (r" << std::dec << revision << ") 0x"
	       << std::setfill('0') << std::setw(8) << std::hex << func_addr
	       << " -> " << symbol_str << "\n";
	std::cout << output.str();
#endif

	g_SymbolAddresses[symbol_str] = func_addr;
}

void ScanXbe(const xbe_header *pXbeHeader, bool is_raw)
{
	size_t xb_start_addr =
	    reinterpret_cast<size_t>(pXbeHeader) - pXbeHeader->dwBaseAddr;
	xbe_library_version *pLibraryVersion;
	xbe_certificate *pCertificate;

	// Ensure nothing is still in g_SymbolAddresses before new scan process
	// start.
	g_SymbolAddresses.clear();

	if (pXbeHeader->pLibraryVersionsAddr != 0) {
		pLibraryVersion = reinterpret_cast<xbe_library_version *>(
		    xb_start_addr + pXbeHeader->pLibraryVersionsAddr);
	}
	else {
		std::cout << "ERROR: Xbe does not contain library versions!\n";
		pause_for_user_input();
		return;
	}

	if (pXbeHeader->pCertificateAddr != 0) {
		pCertificate = reinterpret_cast<xbe_certificate *>(
		    xb_start_addr + pXbeHeader->pCertificateAddr);
	}
	else {
		std::cout << "ERROR: Xbe does not contain certificate pointer!\n";
		pause_for_user_input();
		return;
	}

	uint16_t xdkVersion = 0;
	uint32_t XbLibScan = 0;
	uint16_t buildVersion = 0;

	// Make sure the Symbol Cache directory exists
	std::string cachePath = "SymbolCache/";
	if (!std::filesystem::exists(cachePath) &&
	    !std::filesystem::create_directory(cachePath)) {
		EmuOutputMessage(XB_OUTPUT_MESSAGE_ERROR,
		                 "Couldn't create SymbolCache folder!");
	}

	// Hash the loaded XBE's header, use it as a filename
	uint32_t uiHash = XXH32((void *)pXbeHeader, sizeof(xbe_header), 0);
	std::stringstream sstream;
	char tAsciiTitle[40] = "Unknown";
	std::mbstate_t state = std::mbstate_t();
	std::wcstombs(tAsciiTitle, pCertificate->wszTitleName, sizeof(tAsciiTitle));
	std::string szTitleName(tAsciiTitle);
	PurgeBadChar(szTitleName);
	sstream << cachePath << szTitleName << "-" << std::hex << uiHash << " ("
	        << (is_raw ? "raw" : "sim") << ")"
	        << ".ini";
	std::string filename = sstream.str();
	bool scan_ret;

	CSimpleIniA symbolCacheData;

	//
	// initialize Microsoft XDK scanning
	//
	if (pLibraryVersion != nullptr) {

		std::cout
		    << "Symbol Generator: Detected Microsoft XDK application...\n";

		XbSymbolSetOutputMessage(EmuOutputMessage);

		scan_ret = XbSymbolScan(pXbeHeader, EmuRegisterSymbol, is_raw);

		if (!scan_ret) {
			std::cout << "ERROR: XbSymbolScan failed!\n";
			pause_for_user_input();
			return;
		}
	}
	else {
		std::cout << "ERROR: Xbe does not contain library versions!\n";
		pause_for_user_input();
		return;
	}

	std::cout << "\n";

	// Perform a reset just in case a cached file data still exist.
	symbolCacheData.Reset();

	// Store Symbol Database version
	symbolCacheData.SetLongValue(
	    section_info, sect_info_keys.SymbolDatabaseVersionHash,
	    XbSymbolLibraryVersion(), nullptr, /*UseHex =*/false);

	// Store Certificate Details
	symbolCacheData.SetValue(section_certificate, sect_certificate_keys.Name,
	                         tAsciiTitle);
	symbolCacheData.SetValue(section_certificate, sect_certificate_keys.TitleID,
	                         FormatTitleId(pCertificate->dwTitleId).c_str());
	symbolCacheData.SetLongValue(
	    section_certificate, sect_certificate_keys.TitleIDHex,
	    pCertificate->dwTitleId, nullptr, /*UseHex =*/true);
	symbolCacheData.SetLongValue(
	    section_certificate, sect_certificate_keys.Region,
	    pCertificate->dwGameRegion, nullptr, /*UseHex =*/true);

	// Store Library Details
	for (uint32_t i = 0; i < pXbeHeader->dwLibraryVersions; i++) {
		std::string LibraryName(pLibraryVersion[i].szName,
		                        pLibraryVersion[i].szName + 8);
		symbolCacheData.SetLongValue(section_libs, LibraryName.c_str(),
		                             pLibraryVersion[i].wBuildVersion, nullptr,
		                             /*UseHex =*/false);

		if (buildVersion < pLibraryVersion[i].wBuildVersion) {
			buildVersion = pLibraryVersion[i].wBuildVersion;
		}
	}

	symbolCacheData.SetLongValue(section_libs, sect_libs_keys.BuildVersion,
	                             buildVersion, nullptr,
	                             /*UseHex =*/false);

	// Store detected symbol addresses
	for (auto it = g_SymbolAddresses.begin(); it != g_SymbolAddresses.end();
	     ++it) {
		symbolCacheData.SetLongValue(section_symbols, it->first.c_str(),
		                             it->second, nullptr, /*UseHex =*/true);
	}

	// Save data to unique symbol cache file
	symbolCacheData.SaveFile(filename.c_str());
}
