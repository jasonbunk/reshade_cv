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
template<typename FT> constexpr uint64_t CAMBUFNUMBYTES() { return 8ull + (NUMCAMFLOATSHASHED + 3ull) * sizeof(FT); }

// A simple lua script can write camera coordinates into an array of contiguous floats.
// During a memory scan, this function can check a potentially matching memory buffer by reading the floats and verifying their hash.
template<typename FT>
bool checkbufscriptedcammatrixcounter(const uint8_t* buf, uint64_t buflen) {
  if (buflen < CAMBUFNUMBYTES<FT>()) {
    return false;
  }
  const FT* bufvals = reinterpret_cast<const FT*>(buf+8);
  if (std::isfinite(bufvals[0]) && bufvals[0] > 0.5) {
    FT allsumhash = bufvals[0]; // start with the counter; the counter is included in the hash
    FT plusorminushhash = bufvals[0];
    for (int ii = 1; ii < (1 + NUMCAMFLOATSHASHED); ++ii) {
      allsumhash += bufvals[ii];
      if (ii % 2 == 0) {
        plusorminushhash += bufvals[ii];
      } else {
        plusorminushhash -= bufvals[ii];
      }
    }
    const bool retval = floats_nearly_equal<FT>(allsumhash, bufvals[1+NUMCAMFLOATSHASHED])
                     && floats_nearly_equal<FT>(plusorminushhash, bufvals[2+NUMCAMFLOATSHASHED]);
    return retval;
  }
  return false;
}

template<typename FT>
bool GameWithCameraDataInOneDLL::read_scripted_cambuf_and_copytomatrix(CamMatrix& rcam, std::string& errstr) {
  if (cam_matrix_mem_loc_saved == 0 && !scan_all_memory_for_scripted_cam_matrix(errstr)) return false;
  uint8_t copybuf[CAMBUFNUMBYTES<FT>()];
  FT* dbuf = nullptr;
  SIZE_T nbytesread = 0;
  if (tryreadmemory(gamename_verbose() + std::string("scriptedcam"), errstr, mygame_handle_exe, (LPCVOID)(cam_matrix_mem_loc_saved), (LPVOID)copybuf, CAMBUFNUMBYTES<FT>(), &nbytesread)) {
    if (checkbufscriptedcammatrixcounter<FT>(copybuf, CAMBUFNUMBYTES<FT>())) {
      dbuf = reinterpret_cast<FT*>(copybuf+8) + 1;
      for (int ii = 0; ii < 12; ++ii) {
        rcam.arr3x4[ii] = dbuf[ii];
      }
      rcam.fov = dbuf[12];
      return true;
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
  if (mattype == GameCamDLLMatrix_allmemscanrequiredtofindscriptedtransform_buf_double) {
    if(read_scripted_cambuf_and_copytomatrix<double>(rcam, errstr)) return CamMatrix_AllGood;
    return CamMatrix_Uninitialized;
  } else if (mattype == GameCamDLLMatrix_allmemscanrequiredtofindscriptedtransform_buf_float) {
    if (read_scripted_cambuf_and_copytomatrix<float>(rcam, errstr)) return CamMatrix_AllGood;
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
  if(mattype != GameCamDLLMatrix_allmemscanrequiredtofindscriptedtransform_buf_double && mattype != GameCamDLLMatrix_allmemscanrequiredtofindscriptedtransform_buf_float) {
    errstr += std::string("mat type ") + std::to_string(((int)mattype)) + std::string(" not compatible with scan_all_memory_for_scripted_cam_matrix()");
    return false;
  }
  const bool usedouble = (mattype == GameCamDLLMatrix_allmemscanrequiredtofindscriptedtransform_buf_double);
  const uint64_t bufbytes = usedouble ? CAMBUFNUMBYTES<double>() : CAMBUFNUMBYTES<float>();
  constexpr uint64_t magicbytes = 4429373075689993337ull; // should rarely occur in game memory; so, easy to search for

  AllMemScanner scanner(mygame_handle_exe, bufbytes, errstr, magicbytes);
  double highestcounter = 0.5;
  uint64_t highestctrmloc = 0;
  uint64_t foundloc = 0;
  const uint8_t* foundbuf;
  uint64_t f_b_l;
  double foundcounter;
  while (scanner.iterate_next(foundloc, foundbuf, f_b_l, usedouble ? checkbufscriptedcammatrixcounter<double> : checkbufscriptedcammatrixcounter<float>)) {
    foundcounter = usedouble ? reinterpret_cast<const double*>(foundbuf+8)[0] : reinterpret_cast<const float*>(foundbuf+8)[0];
    if (foundcounter > highestcounter) {
      highestcounter = foundcounter;
      highestctrmloc = foundloc;
      break; // immediately break upon first finding a matching array (with a valid float hash)
    }
  }
  cam_matrix_mem_loc_saved = highestctrmloc;
  return highestctrmloc > 0;
}
