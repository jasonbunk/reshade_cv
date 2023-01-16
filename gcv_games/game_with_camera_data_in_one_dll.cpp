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


static constexpr uint64_t NUMCAMFLOATSHASHED = 13ull;
static constexpr uint64_t CAMBUFNUMBYTES = (NUMCAMFLOATSHASHED + 4ull) * 8ull;

// A simple lua script can write camera coordinates into an array of contiguous doubles.
// During a memory scan, this function can check a potentially matching memory buffer by reading the doubles and verifying their hash.
bool checkbufscriptedcammatrixcounter(const uint8_t* buf, uint64_t buflen) {
  if (buflen < CAMBUFNUMBYTES) {
    return false;
  }
  const double* bufvals = reinterpret_cast<const double*>(buf);
  if (std::isfinite(bufvals[1]) && bufvals[1] > 0.5) {
    double allsumhash = bufvals[1]; // start with the counter; the counter is included in the hash
    double plusorminushhash = bufvals[1];
    for (int ii = 2; ii < (2 + NUMCAMFLOATSHASHED); ++ii) {
      allsumhash += bufvals[ii];
      if (ii % 2 == 0) {
        plusorminushhash -= bufvals[ii];
      } else {
        plusorminushhash += bufvals[ii];
      }
    }
    const bool retval = floats_nearly_equal<double>(allsumhash, bufvals[2+NUMCAMFLOATSHASHED])
                     && floats_nearly_equal<double>(plusorminushhash, bufvals[3+NUMCAMFLOATSHASHED]);
    return retval;
  }
  return false;
}


uint8_t GameWithCameraDataInOneDLL::get_camera_matrix(CamMatrix& rcam, std::string& errstr) {
  if (!init_in_game()) return CamMatrix_Uninitialized;
  const GameCamDLLMatrixType mattype = camera_dll_matrix_format();
  SIZE_T nbytesread = 0;
  if (mattype == GameCamDLLMatrix_allmemscanrequiredtofindscriptedtransform) {
    if (cam_matrix_mem_loc_saved > 0 || scan_all_memory_for_scripted_cam_matrix(errstr)) {
      double dbuf[4+NUMCAMFLOATSHASHED];
      if (tryreadmemory(gamename_verbose() + std::string("scriptedcam"), errstr, mygame_handle_exe, (LPCVOID)(cam_matrix_mem_loc_saved), (LPVOID)dbuf, CAMBUFNUMBYTES, &nbytesread)) {
        if (checkbufscriptedcammatrixcounter(reinterpret_cast<const uint8_t*>(dbuf), CAMBUFNUMBYTES)) {
          for (int ii = 0; ii < 12; ++ii) {
            rcam.arr3x4[ii] = dbuf[2+ii];
          }
          rcam.fov = dbuf[14];
          return CamMatrix_AllGood;
        } else {
          errstr += std::string("after reading memory at ") + std::to_string(cam_matrix_mem_loc_saved) + std::string(", float hash failed");
        }
      } else {
        errstr += std::string("failed to read memory at ") + std::to_string(cam_matrix_mem_loc_saved);
      }
    } else {
      errstr += std::string("failed to scan memory; cam matrix memloc ") + std::to_string(cam_matrix_mem_loc_saved);
    }
    return CamMatrix_Uninitialized;
  }
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
  AllMemScanner scanner(mygame_handle_exe, CAMBUFNUMBYTES, errstr, 4429373075689993337ull);
  double highestcounter = 0.5;
  uint64_t highestctrmloc = 0;
  uint64_t foundloc = 0;
  const uint8_t* foundbuf;
  uint64_t f_b_l;
  double foundcounter;
  while (scanner.iterate_next(foundloc, foundbuf, f_b_l, checkbufscriptedcammatrixcounter)) {
    foundcounter = reinterpret_cast<const double*>(foundbuf)[1];
    if (foundcounter > highestcounter) {
      highestcounter = foundcounter;
      highestctrmloc = foundloc;
      break; // immediately break upon first finding a matching array (with a valid float hash)
    }
  }
  cam_matrix_mem_loc_saved = highestctrmloc;
  return highestctrmloc > 0;
}
