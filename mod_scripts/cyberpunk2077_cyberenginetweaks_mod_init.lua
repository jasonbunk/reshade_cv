-- Copyright (C) 2022 Jason Bunk
-- This is a mod for Cyber Engine Tweaks: https://github.com/yamashi/CyberEngineTweaks
-- Stashes camera coordinates in a contiguous memory buffer, so it can be found by a c++ memory scan.
-- The buffer starts with a few distinct bytes, and is hashed for verification.
-- See c++ function: scan_all_memory_for_scripted_cam_matrix()

MyCamCoordsStash = {
    CamTransform = nil,
    counter = -1.5,
    contiguousmembuf = { 1.38097189588312856e-12, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17 },
}

local function zero_the_cambuf()
    for i=3,15 do
        MyCamCoordsStash.contiguousmembuf[i] = 0;
    end
    -- "hash" still valid, for findability, but the matrix is not useful
    MyCamCoordsStash.contiguousmembuf[16] = MyCamCoordsStash.contiguousmembuf[2];
    MyCamCoordsStash.contiguousmembuf[17] = MyCamCoordsStash.contiguousmembuf[2];
end

registerForEvent("onUpdate", function(delta)
    if Game == nil then
        zero_the_cambuf();
    else
        print("lua version " .. _VERSION);
        if MyCamCoordsStash.CamTransform ~= nil then
            MyCamCoordsStash.counter = MyCamCoordsStash.counter + 1;
            if MyCamCoordsStash.counter > 9999.5 then
                MyCamCoordsStash.counter = 1;
            end
        else
            MyCamCoordsStash.CamTransform = Transform.new();
            MyCamCoordsStash.counter = 3;
        end
        local camsystem = Game.GetCameraSystem();
        if camsystem == nil then
            zero_the_cambuf();
        else
            if camsystem:GetActiveCameraWorldTransform(MyCamCoordsStash.CamTransform) then
                local camfov = Game.GetCameraSystem():GetActiveCameraFOV();
                local tmat = Transform.ToMatrix(MyCamCoordsStash.CamTransform);
                local poshash1 = (MyCamCoordsStash.counter + tmat.X.x + tmat.Y.x + tmat.Z.x + MyCamCoordsStash.CamTransform.position.x + tmat.X.y + tmat.Y.y + tmat.Z.y + MyCamCoordsStash.CamTransform.position.y + tmat.X.z + tmat.Y.z + tmat.Z.z + MyCamCoordsStash.CamTransform.position.z + camfov);
                local poshash2 = (MyCamCoordsStash.counter - tmat.X.x + tmat.Y.x - tmat.Z.x + MyCamCoordsStash.CamTransform.position.x - tmat.X.y + tmat.Y.y - tmat.Z.y + MyCamCoordsStash.CamTransform.position.y - tmat.X.z + tmat.Y.z - tmat.Z.z + MyCamCoordsStash.CamTransform.position.z - camfov);
                MyCamCoordsStash.contiguousmembuf[ 2] = MyCamCoordsStash.counter;
                MyCamCoordsStash.contiguousmembuf[ 3] = tmat.X.x;
                MyCamCoordsStash.contiguousmembuf[ 4] = tmat.Y.x;
                MyCamCoordsStash.contiguousmembuf[ 5] = tmat.Z.x;
                MyCamCoordsStash.contiguousmembuf[ 6] = MyCamCoordsStash.CamTransform.position.x;
                MyCamCoordsStash.contiguousmembuf[ 7] = tmat.X.y;
                MyCamCoordsStash.contiguousmembuf[ 8] = tmat.Y.y;
                MyCamCoordsStash.contiguousmembuf[ 9] = tmat.Z.y;
                MyCamCoordsStash.contiguousmembuf[10] = MyCamCoordsStash.CamTransform.position.y;
                MyCamCoordsStash.contiguousmembuf[11] = tmat.X.z;
                MyCamCoordsStash.contiguousmembuf[12] = tmat.Y.z;
                MyCamCoordsStash.contiguousmembuf[13] = tmat.Z.z;
                MyCamCoordsStash.contiguousmembuf[14] = MyCamCoordsStash.CamTransform.position.z;
                MyCamCoordsStash.contiguousmembuf[15] = camfov;
                MyCamCoordsStash.contiguousmembuf[16] = poshash1;
                MyCamCoordsStash.contiguousmembuf[17] = poshash2;
            else
                for i=3,17 do
                    MyCamCoordsStash.contiguousmembuf[i] = i
                end
            end
        end
    end
end)