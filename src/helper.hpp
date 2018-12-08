// *  XbSymbolCachGenTest is free software; you can redistribute them
// *  and/or modify them under the terms of the GNU General Public
// *  License as published by the Free Software Foundation; either
// *  version 2 of the license, or (at your option) any later version.
// *
// *  This program is distributed in the hope that it will be useful,
// *  but WITHOUT ANY WARRANTY; without even the implied warranty of
// *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// *  GNU General Public License for more details.
// *
// *  You should have recieved a copy of the GNU General Public License
// *  along with this program; see the file COPYING.
// *  If not, write to the Free Software Foundation, Inc.,
// *  59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
// *
// *  (c) 2018 Luke Usher <luke.usher@outlook.com>
// *  (c) 2018 ergo720
// *
// *  All rights reserved

#pragma once

// made by Luke Usher and ergo720
static std::string FormatTitleId(uint32_t title_id)
{
	std::stringstream ss;

	// If the Title ID prefix is a printable character, parse it
	// This shows the correct game serial number for retail titles!
	// EG: MS-001 for 1st tile published by MS, EA-002 for 2nd title by EA, etc
	// Some special Xbes (Dashboard, XDK Samples) use non-alphanumeric serials
	// We fall back to Hex for those
	// ergo720: we cannot use isalnum() here because it will treat chars in the
	// range -1 - 255 as valid ascii chars which can lead to unicode characters
	// being printed in the title (e.g.: dashboard uses 0xFE and 0xFF)
	uint8_t pTitleId1 = (title_id >> 24) & 0xFF;
	uint8_t pTitleId2 = (title_id >> 16) & 0xFF;

	if ((pTitleId1 < 65 || pTitleId1 > 90) ||
	    (pTitleId2 < 65 || pTitleId2 > 90)) {
		// Prefix was non-printable, so we need to print a hex reprentation of
		// the entire title_id
		ss << std::setfill('0') << std::setw(8) << std::hex << std::uppercase
		   << title_id;
		return ss.str();
	}

	ss << pTitleId1 << pTitleId2;
	ss << "-";
	ss << std::setfill('0') << std::setw(3) << std::dec
	   << (title_id & 0x0000FFFF);

	return ss.str();
}

// made by ergo720
static void PurgeBadChar(std::string &s, const std::string &illegalChars = "\\/:?\"<>|")
{
	for (auto it = s.begin(); it < s.end(); ++it) {
		bool found = illegalChars.find(*it) != std::string::npos;
		if (found) {
			*it = '_';
		}
	}
}
