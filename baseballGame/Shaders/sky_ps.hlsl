// sky_ps.hlsl
// Day/Night cycle sky shader with sun disk and atmospheric scattering approximation

// --- Constant Buffers ---

// b1: Scene constants (camera_position, view_projection etc.)
cbuffer scene_constants : register(b1)
{
    float4x4 view_projection;
    float4   camera_position;
    float4   camera_right;
    float4   camera_up;
};

// b9: Sky constants
cbuffer sky_constants : register(b9)
{
    float4   sun_direction;      // world-space direction TOWARD the sun (normalized), w unused
    float4   sun_color;          // color of the sun disk
    float4   sky_zenith_color;   // color at top of sky
    float4   sky_horizon_color;  // color at horizon
    float4   sky_ground_color;   // color below horizon
    float    time_of_day;        // 0.0 = midnight, 0.25 = sunrise, 0.5 = noon, 0.75 = sunset, 1.0 = midnight
    float    sun_size;           // angular size of sun disk in radians (try 0.025)
    float    sun_bloom_size;     // soft glow radius (try 0.12)
    float    sky_dummy;
};

// --- Input from vertex shader ---
struct PS_INPUT
{
    float4 position  : SV_POSITION;
    float2 texcoord  : TEXCOORD0;
};

// --- Utility: reconstruct world-space ray direction from clip-space ---
float3 ReconstructRayDir(float2 uv)
{
    // uv in [0,1] -> ndc in [-1,1]
    float2 ndc = uv * 2.0f - 1.0f;
    ndc.y = -ndc.y;

    // Inverse view-projection to get world direction
    float4x4 inv_vp = view_projection;

    // We need the inverse of view_projection.
    // Build a near-plane world position and a far-plane position, subtract.
    float4 near_h = float4(ndc.x, ndc.y, 0.0f, 1.0f);
    float4 far_h  = float4(ndc.x, ndc.y, 1.0f, 1.0f);

    // Manually invert VP matrix
    float4x4 VP = view_projection;

    // Helper: use the camera basis vectors baked into scene_constants
    // Simpler approach: reconstruct from camera vectors
    // We derive screen-space to world via the transpose trick
    // Actually, let's just reconstruct from scratch with an inverse VP.

    // Standard inverse via adjugate is complex in HLSL, so instead
    // we rebuild the ray from camera_position + screen direction.
    // The camera vectors (right, up) are available; we compute forward = cross(right, up)
    // but we also need the FOV. Since we do not have it directly, reconstruct from VP matrix.

    // Extract camera forward from VP:
    // Projection removes translation, view encodes camera.
    // Row 2 of VP (row-major HLSL) is the Z-row after projection.
    // Instead use the simplest reconstruction: unproject two clip points.

    // VP is row-major in HLSL (row vectors)
    // To unproject: solve VP * w_pos = h_pos  =>  w_pos = h_pos * VP^-1
    // We will use the 4x4 minor-expansion inverse.

    // Actually the cleanest approach: use camera_right, camera_up, compute forward
    float3 right   = normalize(camera_right.xyz);
    float3 up      = normalize(camera_up.xyz);
    float3 forward = cross(right, up);  // left-handed: right x up = backward, so negate
    forward = -forward;

    // Recover half-width and half-height from the projection matrix (column-major layout)
    // VP[0][0] = (1/tan(fovX/2)) * (H/W), VP[1][1] = 1/tan(fovY/2)
    float inv_tan_fov_x = VP[0][0]; // scales X
    float inv_tan_fov_y = VP[1][1]; // scales Y

    float3 ray = forward
               + right   * (ndc.x / inv_tan_fov_x)
               + up      * (ndc.y / inv_tan_fov_y);

    return normalize(ray);
}

// --- Sky gradient ---
float3 SkyGradient(float3 ray_dir, float3 sun_dir)
{
    // Height factor: -1 (ground) to +1 (zenith)
    float height = ray_dir.y; // [-1, 1]

    // Below horizon
    float horizon_blend = smoothstep(-0.05f, 0.05f, height);
    float zenith_blend  = saturate(height * 1.5f);

    float3 horizon_color = sky_horizon_color.rgb;
    float3 zenith_color  = sky_zenith_color.rgb;
    float3 ground_color  = sky_ground_color.rgb;

    float3 sky = lerp(horizon_color, zenith_color, zenith_blend);
    sky = lerp(ground_color, sky, horizon_blend);

    return sky;
}

// --- Atmospheric haze near horizon ---
float3 HorizonHaze(float3 ray_dir, float3 sun_dir)
{
    float horizon_factor = 1.0f - abs(ray_dir.y);
    horizon_factor = pow(saturate(horizon_factor), 4.0f);

    // Sunrise/sunset haze is warm near the sun
    float sun_align = dot(ray_dir, sun_dir) * 0.5f + 0.5f;
    float3 warm_haze = lerp(float3(0.3f, 0.15f, 0.05f), float3(1.0f, 0.5f, 0.2f), sun_align);
    float3 cold_haze = float3(0.6f, 0.7f, 0.9f);

    // Blend based on time (sun_color brightness indicates day/night)
    float day = saturate(sun_dir.y * 3.0f);
    float3 haze_color = lerp(warm_haze, cold_haze, day);

    return haze_color * horizon_factor * 0.6f;
}

// --- Sun disk and corona ---
float3 SunDisk(float3 ray_dir, float3 sun_dir)
{
    float cos_angle = dot(ray_dir, sun_dir);
    float cos_size  = cos(sun_size);           // hard edge
    float cos_bloom = cos(sun_bloom_size);     // soft glow

    // Hard disk
    float disk = step(cos_size, cos_angle);

    // Soft glow / corona
    float bloom_t = saturate((cos_angle - cos_bloom) / (1.0f - cos_bloom));
    float bloom   = pow(bloom_t, 6.0f);

    // Limb darkening on the disk edge (darker at edge of sun)
    float limb = saturate((cos_angle - cos_size) / (1.0f - cos_size));
    float limb_darken = lerp(0.7f, 1.0f, sqrt(limb));

    // Only show sun above horizon
    float above_horizon = smoothstep(-0.02f, 0.05f, sun_dir.y);

    float3 sun_contrib = sun_color.rgb * (disk * limb_darken + bloom * 0.5f);
    return sun_contrib * above_horizon;
}



// --- Moon ---
float3 Moon(float3 ray_dir, float3 sun_dir, float night_factor)
{
    if (night_factor < 0.01f) return float3(0, 0, 0);

    // Moon is opposite to sun
    float3 moon_dir = -sun_dir;
    moon_dir.y = abs(moon_dir.y); // keep moon above horizon at night (artistic choice)

    float cos_angle = dot(ray_dir, moon_dir);
    float cos_size  = cos(0.018f); // smaller than sun
    float cos_bloom = cos(0.06f);

    float disk  = step(cos_size, cos_angle);
    float bloom_t = saturate((cos_angle - cos_bloom) / (1.0f - cos_bloom));
    float bloom   = pow(bloom_t, 4.0f);

    float above = smoothstep(-0.02f, 0.05f, moon_dir.y);

    float3 moon_color = float3(0.9f, 0.92f, 1.0f) * (disk * 0.9f + bloom * 0.15f) * above;
    return moon_color * night_factor;
}

// --- Main ---
float4 main(PS_INPUT input) : SV_TARGET
{
    float3 ray_dir = ReconstructRayDir(input.texcoord);

    float3 sun_dir = normalize(sun_direction.xyz);

    // Night factor: when sun is below horizon
    float night_factor = saturate(1.0f - smoothstep(-0.2f, 0.2f, sun_dir.y));
    // During twilight (sun near horizon) partial night
    float twilight = saturate(1.0f - abs(sun_dir.y) * 5.0f);

    // Sky base gradient
    float3 sky = SkyGradient(ray_dir, sun_dir);

    // Atmospheric haze
    sky += HorizonHaze(ray_dir, sun_dir);

    // Moon
    sky += Moon(ray_dir, sun_dir, night_factor);

    // Sun disk (only shown when above horizon, fades during twilight into glow)
    sky += SunDisk(ray_dir, sun_dir);

    return float4(sky, 1.0f);
}
