#pragma once
// Copyright (C) 2022 Jason Bunk
#include "game_interface.h"

enum GameCamDLLMatrixType {
  GameCamDLLMatrix_positiononly,
  GameCamDLLMatrix_3x4,
  GameCamDLLMatrix_4x4,
  GameCamDLLMatrix_pos_and_3x3rot,
  GameCamDLLMatrix_pos_and_4x3rot,
  GameCamDLLMatrix_allmemscanrequiredtofindscriptedcambuf,
};

typedef bool (*scriptedcam_checkbuf_funptr)(const void*, const uint8_t*, uint64_t);

class GameWithCameraDataInOneDLL : public GameInterface {
protected:
  HMODULE camera_dll = 0;
  uint64_t cam_matrix_mem_loc_saved = 0; // used if all memory needs to be scanned

  // if you subclass from this, you need to implement these,
  // and also the two from GameInterface returning the string name of the game
  virtual std::string camera_dll_name() const = 0;
  virtual uint64_t camera_dll_mem_start() const = 0;
  virtual uint64_t camera_dll_second_memloc() const { return 0; } // only used if rotation matrix and position are in different locations
  virtual GameCamDLLMatrixType camera_dll_matrix_format() const = 0;

  virtual scriptedcam_checkbuf_funptr get_scriptedcambuf_checkfun() const;
  virtual uint64_t get_scriptedcambuf_sizebytes() const { return 0; }
  virtual bool copy_scriptedcambuf_to_matrix(uint8_t* buf, uint64_t buflen, CamMatrix& rcam, std::string& errstr) const { return false; }
  bool read_scripted_cambuf_and_copy_to_matrix(CamMatrix& rcam, std::string& errstr); // not virtual, no need to override; just override the above three

public:
  virtual bool init_in_game() override;
  virtual uint8_t get_camera_matrix(CamMatrix& rcam, std::string& errstr) override;
  // memory scans
  virtual bool scan_all_memory_for_scripted_cam_matrix(std::string& errstr) override;
};
