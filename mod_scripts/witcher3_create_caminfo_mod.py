# Copyright (C) 2022 Jason Bunk
import os
thispath = os.path.dirname(os.path.abspath(__file__))
import argparse
from diff_match_patch import diff_match_patch # pip install diff-match-patch

# This creates a Witcher 3 mod in the game folder.
# It's a simple script mod which stashes the camera coordinates every frame to a float[] buffer.

modpatch = os.path.join(thispath,'witcher3_r4Game_caminfo_patch.wspatchdmp')
assert os.path.isfile(modpatch), modpatch

parser = argparse.ArgumentParser()
parser.add_argument('witcher3root', help='Game directory, for example: \"C:\\Program Files (x86)\\Steam\\steamapps\\common\\The Witcher 3\"')
args = parser.parse_args()
assert os.path.isdir(args.witcher3root), args.witcher3root

# this is the game script that was modded
gamescriptfile = os.path.join(args.witcher3root,
  'content','content0','scripts','game','r4Game.ws')
assert os.path.isfile(gamescriptfile), gamescriptfile

# load patch and script and apply diff
with open(modpatch,'r') as infile:
  patchdiff = infile.read()

with open(gamescriptfile,'rb') as infile:
  gamescript = infile.read().decode('utf16', 'strict')

dmp = diff_match_patch()
patches = dmp.patch_fromText(patchdiff)
new_text, _ = dmp.patch_apply(patches, gamescript)

# create mod folder and copy the modified game script into it
modfullpath = args.witcher3root
for modfolder in ['Mods','modCameraInfoBuffer','content','scripts','game']:
  modfullpath = os.path.join(modfullpath, modfolder)
  os.makedirs(modfullpath, exist_ok=True)

with open(os.path.join(modfullpath,'r4Game.ws'), 'wb') as outfile:
  outfile.write(new_text.encode('utf16'))
