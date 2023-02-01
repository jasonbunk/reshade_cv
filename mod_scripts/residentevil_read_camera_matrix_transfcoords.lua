-- Copyright (C) 2022 Jason Bunk
-- This is a mod for REFramework: https://github.com/praydog/REFramework
-- Stashes camera coordinates in a contiguous memory buffer, so it can be found by a c++ memory scan.
-- The buffer starts with a few distinct bytes, and is hashed for verification.
-- See c++ function: scan_all_memory_for_scripted_cam_matrix()

MyCamCoordsStash = {
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

re.on_application_entry("BeginRendering", function()
    if MyCamCoordsStash.counter < 0.0 then
        MyCamCoordsStash.counter = 2.0;
        -- log.info("lua version " .. _VERSION); -- 5.4
    else
        MyCamCoordsStash.counter = MyCamCoordsStash.counter + 1.0;
        if MyCamCoordsStash.counter > 9999.5 then
            MyCamCoordsStash.counter = 1.0;
        end
    end
    local gamecam = sdk.get_primary_camera();
    if gamecam ~= nil then
        -- gamecam is of type via.Camera
        -- gamecam:get_ProjectionMatrix(); -- returns Matrix4x4f;
        -- gamecam:get_ViewMatrix(); -- returns Matrix4x4f;
        -- gamecam:get_WindowSize(); -- returns float screen_size[2];
        local exm = gamecam:call("get_WorldMatrix"); -- gamecam:get_WorldMatrix();
        local fovmat = gamecam:call("get_ProjectionMatrix"); -- intrinsic matrix; we just need fov
        if exm ~= nil and fovmat ~= nil then
            -- exm[j] is the j'th column Vector4f of the extrinsic matrix cam2world
            -- coordinates are being rotated to match other games
            local fovhdeg = math.atan(1.0/(fovmat[0].x))*(360.0/math.pi);
            local poshash1 = (MyCamCoordsStash.counter + exm[0].x - exm[2].x + exm[1].x + exm[3].x - exm[0].z + exm[2].z - exm[1].z - exm[3].z + exm[0].y - exm[2].y + exm[1].y + exm[3].y + fovhdeg);
            local poshash2 = (MyCamCoordsStash.counter - exm[0].x - exm[2].x - exm[1].x + exm[3].x + exm[0].z + exm[2].z + exm[1].z - exm[3].z - exm[0].y - exm[2].y - exm[1].y + exm[3].y - fovhdeg);
            MyCamCoordsStash.contiguousmembuf[ 2] = MyCamCoordsStash.counter;
            MyCamCoordsStash.contiguousmembuf[ 3] = exm[0].x;
            MyCamCoordsStash.contiguousmembuf[ 4] = -exm[2].x;
            MyCamCoordsStash.contiguousmembuf[ 5] = exm[1].x;
            MyCamCoordsStash.contiguousmembuf[ 6] = exm[3].x;
            MyCamCoordsStash.contiguousmembuf[ 7] = -exm[0].z;
            MyCamCoordsStash.contiguousmembuf[ 8] = exm[2].z;
            MyCamCoordsStash.contiguousmembuf[ 9] = -exm[1].z;
            MyCamCoordsStash.contiguousmembuf[10] = -exm[3].z;
            MyCamCoordsStash.contiguousmembuf[11] = exm[0].y;
            MyCamCoordsStash.contiguousmembuf[12] = -exm[2].y;
            MyCamCoordsStash.contiguousmembuf[13] = exm[1].y;
            MyCamCoordsStash.contiguousmembuf[14] = exm[3].y;
            MyCamCoordsStash.contiguousmembuf[15] = fovhdeg;
            MyCamCoordsStash.contiguousmembuf[16] = poshash1;
            MyCamCoordsStash.contiguousmembuf[17] = poshash2;
        else
            log.info("beginrendering(): exm nil...");
            zero_the_cambuf();
        end
    else
        log.info("beginrendering(): gamecam nil...");
        zero_the_cambuf();
    end
end)
