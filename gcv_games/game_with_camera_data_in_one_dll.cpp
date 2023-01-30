// Copyright (C) 2022 Jason Bunk
#include "game_with_camera_data_in_one_dll.h"
#include "gcv_utils/miscutils.h"
#include "gcv_utils/memread.h"
#include "gcv_utils/scan_for_camera_matrix.h"

bool GameWithCameraDataInOneDLL::init_in_game() {
  if (camera_dll != 0)
    return true;
  if (camera_dll_name().empty()) {
    camera_dll = GetModuleHandle(0); // use the memory space of the main exe, not a dll
    return camera_dll != 0;

  }
  camera_dll = GetModuleHandle(wstring_from_string(camera_dll_name()).c_str());
  return camera_dll != 0;
}


bool dummy_check_scriptedcambuf(const void* scanctx, const uint8_t* buf, uint64_t buflen) {
    return true;
}
scriptedcam_checkbuf_funptr GameWithCameraDataInOneDLL::get_scriptedcambuf_checkfun() const {
    return dummy_check_scriptedcambuf;
}


bool GameWithCameraDataInOneDLL::read_scripted_cambuf_and_copy_to_matrix(CamMatrix& rcam, std::string& errstr) {
  if (cam_matrix_mem_loc_saved == 0 && !scan_all_memory_for_scripted_cam_matrix(errstr)) return false;
  std::vector<uint8_t> copybuf(get_scriptedcambuf_sizebytes());
  SIZE_T nbytesread = 0;
  if (tryreadmemory(gamename_verbose() + std::string("scriptedcam"), errstr, mygame_handle_exe, (LPCVOID)(cam_matrix_mem_loc_saved), (LPVOID)(copybuf.data()), get_scriptedcambuf_sizebytes(), &nbytesread)) {
    if (get_scriptedcambuf_checkfun()(nullptr, copybuf.data(), get_scriptedcambuf_sizebytes())) {
      return copy_scriptedcambuf_to_matrix(copybuf.data(), nbytesread, rcam, errstr);
    } else {
      errstr += std::string("after reading memory at ") + std::to_string(cam_matrix_mem_loc_saved) + std::string(", float hash failed");
    }
  } else {
    errstr += std::string("failed to read memory at ") + std::to_string(cam_matrix_mem_loc_saved) + std::string(", tryreadmem failed");
  }
  return false;
}


uint8_t GameWithCameraDataInOneDLL::get_camera_matrix(CamMatrix& rcam, std::string& errstr) {
  if (!init_in_game()) return CamMatrix_Uninitialized;
  const GameCamDLLMatrixType mattype = camera_dll_matrix_format();
  if (mattype == GameCamDLLMatrix_allmemscanrequiredtofindscriptedcambuf) {
    if(read_scripted_cambuf_and_copy_to_matrix(rcam, errstr)) return CamMatrix_AllGood;
    return CamMatrix_Uninitialized;
  }
  SIZE_T nbytesread = 0;
  UINT_PTR dll4cambaseaddr = (UINT_PTR)camera_dll;
  const uint64_t camloc = camera_dll_mem_start();
  if (mattype == GameCamDLLMatrix_3x4) {
    if (tryreadmemory(gamename_verbose() + std::string("_3x4cam"), errstr, mygame_handle_exe,
        (LPCVOID)(dll4cambaseaddr + camloc), reinterpret_cast<LPVOID>(rcam.arr3x4),
        12 * sizeof(float), &nbytesread)) {
      return CamMatrix_AllGood;
    } else {
      return CamMatrix_Uninitialized;
    }
  }
  float cambuf[3];
  for (uint64_t colidx = 0; colidx < 4; ++colidx) {
    if (!tryreadmemory(gamename_verbose() + std::string("_4x4cam_col") + std::to_string(colidx),
        errstr, mygame_handle_exe, (LPCVOID)(dll4cambaseaddr + camloc + colidx * 4 * sizeof(float)),
        reinterpret_cast<LPVOID>(cambuf), 3 * sizeof(float), &nbytesread)) {
      return CamMatrix_Uninitialized;
    }
    if (mattype == GameCamDLLMatrix_positiononly) {
      rcam.GetPosition().x = cambuf[0];
      rcam.GetPosition().y = cambuf[1];
      rcam.GetPosition().z = cambuf[2];
      return CamMatrix_PositionGood;
    }
    rcam.GetCol(colidx).x = cambuf[0];
    rcam.GetCol(colidx).y = cambuf[1];
    rcam.GetCol(colidx).z = cambuf[2];
  }
  return CamMatrix_AllGood;
}


bool GameWithCameraDataInOneDLL::scan_all_memory_for_scripted_cam_matrix(std::string& errstr)
{
  const GameCamDLLMatrixType mattype = camera_dll_matrix_format();
  if(mattype != GameCamDLLMatrix_allmemscanrequiredtofindscriptedcambuf) {
    errstr += std::string("mat type ") + std::to_string(((int)mattype)) + std::string(" not compatible with scan_all_memory_for_scripted_cam_matrix()");
    return false;
  }
  const uint64_t bufbytes = get_scriptedcambuf_sizebytes();
  constexpr uint64_t magicbytes = 4429373075689993337ull; // should rarely occur in game memory; so, easy to search for

  AllMemScanner scanner(mygame_handle_exe, bufbytes, errstr, magicbytes, true);
  uint64_t foundloc = 0;
  const uint8_t* foundbuf = nullptr;
  uint64_t f_b_l = 0;
  while (scanner.iterate_next(foundloc, foundbuf, f_b_l, nullptr, get_scriptedcambuf_checkfun())) {
    cam_matrix_mem_loc_saved = foundloc;
    return true; // immediately break upon first finding a matching array (with a valid float hash)
  }
  return false;
}
