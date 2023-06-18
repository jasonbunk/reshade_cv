#pragma once

inline bool DXBC_Disassembly_FriendlyNaming() { return true; }
inline bool DXBC_Disassembly_ProcessVendorShaderExts() { return true; }

// this is declared centrally so it can be shared with any backend - the name is a misnomer but kept
// for backwards compatibility reasons.
inline rdcarray<rdcstr> DXBC_Debug_SearchDirPaths() { return {""}; } // Paths to search for separated shader debug PDBs.
