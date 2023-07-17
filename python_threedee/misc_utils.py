# Copyright (C) 2022 Jason Bunk
import os
import glob

def files_glob(filesorglobs):
  allfiles = []
  if isinstance(filesorglobs,str):
    return files_glob([filesorglobs,])
  for forg in filesorglobs:
    assert isinstance(forg,str), str(type(forg))+'\n'+str(forg)
    if os.path.isfile(forg):
      allfiles.append(forg)
    else:
      assert '*' in forg or '?' in forg, forg
      foundglob = glob.glob(forg)
      assert len(foundglob) >= 0, forg
      for f2 in foundglob:
        assert os.path.isfile(f2), f2
        allfiles.append(f2)
  assert len(allfiles) > 0, '\n'.join(filesorglobs)
  return allfiles
