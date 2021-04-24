#define DrawCommand(batch, type) (type *) __DrawCommand(batch, DrawCommand_##type, sizeof(type))
internal void *__DrawCommand(Render_Batch *batch, Draw_Command_Type type, umm size) {
    void *result = 0;

    Draw_Command_Buffer *buffer = batch->buffer;

    umm full_size = size + sizeof(u32);
    if ((buffer->used + full_size) <= buffer->size) {
        u32 *header = cast(u32 *) (buffer->base + buffer->used);
        *header = cast(u32) type;

        result = cast(void *) (header + 1);

        buffer->used += full_size;
    }

    return result;
}

// @Note: Actual memory to load the texture is positioned directly after the info header
//
internal Texture_Transfer_Info *GetTextureTransferMemory(Texture_Transfer_Buffer *buffer, u32 width, u32 height) {
    Texture_Transfer_Info *result = 0;

    umm required_size = (4 * width * height) + sizeof(Texture_Transfer_Info);
    if ((buffer->used + required_size) <= buffer->size) {
        result = cast(Texture_Transfer_Info *) (buffer->base + buffer->used);

        result->transfer_size = (required_size - sizeof(Texture_Transfer_Info));

        buffer->used += required_size;
    }

    return result;
}

internal Texture_Handle CreateTextureHandle(u32 index, u32 width, u32 height) {
    Texture_Handle result;

    // @Todo(James): Should probably check if the width/height actually fit inside 16 bits, that said
    // it is unlikely we will have textures > 65535x65535
    //
    result.width  = cast(u16) width;
    result.height = cast(u16) height;
    result.index  = index;

    return result;
}

internal void SetCameraTransform(Render_Batch *batch, u32 flags,
        v3 x = V3(1, 0, 0), v3 y = V3(0, 1, 0), v3 z = V3(0, 0, 1), v3 p = V3(0, 0, 0),
        f32 near = 0.1f, f32 far = 1000.0f, f32 fov = Radians(50))
{
    b32 is_ortho = (flags & RenderTransform_Orthographic) != 0;

    Render_Transform tx = batch->game_tx;

    tx.flags = flags;

    tx.camera.x = x;
    tx.camera.y = y;
    tx.camera.z = z;
    tx.camera.p = p;

    v2 dim = V2(batch->buffer->setup.render_size);
    f32 aspect = dim.w / dim.h;

    mat4_inv projection;
    if (is_ortho) {
        projection = OrthographicProjection(aspect, near, far);
    }
    else {
        projection = PerspectiveProjection(fov, aspect, near, far);
    }

    mat4_inv camera_tx = CameraTransform(x, y, z, p);

    projection.forward = (projection.forward * camera_tx.forward);
    projection.inverse = (camera_tx.inverse * projection.inverse);

    tx.projection = projection;

    batch->game_tx = tx;
}

internal v3 UnprojectAt(Render_Transform *tx, v2 clip_xy, f32 z) {
    v3 result;

    v4 probe = tx->projection.forward * V4(tx->camera.p - (z * tx->camera.z), 1.0f);
    clip_xy *= probe.w;

    v4 world = tx->projection.inverse * V4(clip_xy.x, clip_xy.y, probe.z, probe.w);
    result = V3(world);

    return result;
}

internal v3 Unproject(Render_Transform *tx, v2 clip_xy) {
    v3 result = UnprojectAt(tx, clip_xy, tx->camera.p.z);
    return result;
}

internal rect3 GetCameraBoundsAt(Render_Transform *tx, f32 z) {
    rect3 result;
    result.min = UnprojectAt(tx, V2(-1, -1), z);
    result.max = UnprojectAt(tx, V2( 1,  1), z);

    return result;
}

internal rect3 GetCameraBounds(Render_Transform *tx) {
    rect3 result;
    result.min = UnprojectAt(tx, V2(-1, -1), tx->camera.p.z);
    result.max = UnprojectAt(tx, V2( 1,  1), tx->camera.p.z);

    return result;
}

internal void DrawClear(Render_Batch *batch, v4 colour = V4(1, 1, 1, 1), b32 depth = true) {
    Draw_Command_Clear *command = DrawCommand(batch, Draw_Command_Clear);
    if (command) {
        command->colour = colour;
        command->depth  = depth;
    }
}

internal Render_Batch CreateRenderBatch(Draw_Command_Buffer *command_buffer, Asset_Manager *assets,
        v4 clear_colour = V4(1, 1, 1, 1))
{
    Render_Batch result = {};

    result.buffer = command_buffer;
    result.assets = assets;

    SetCameraTransform(&result, 0);

    DrawClear(&result, clear_colour, true);

    return result;
}

internal Draw_Command_Vertex_Batch *DrawCommandVertexBatch(Render_Batch *batch, Texture_Handle texture) {
    Draw_Command_Vertex_Batch *result = batch->vertex_batch;
    if (!result || !IsEqual(result->texture, texture)) {
        result = DrawCommand(batch, Draw_Command_Vertex_Batch);
        if (result) {
            result->texture       = texture;
            result->transform     = batch->game_tx.projection.forward;

            result->vertex_offset = batch->buffer->vertex_count;
            result->vertex_count  = 0;

            result->index_offset  = batch->buffer->index_count;
            result->index_count   = 0;
        }

        batch->vertex_batch = result;
    }

    return result;
}

internal void DrawQuad(Render_Batch *batch, Texture_Handle texture,
        v3 p0, v2 uv0, u32 c0,
        v3 p1, v2 uv1, u32 c1,
        v3 p2, v2 uv2, u32 c2,
        v3 p3, v2 uv3, u32 c3)
{
    Draw_Command_Vertex_Batch *vertex_batch = DrawCommandVertexBatch(batch, texture);
    if (vertex_batch) {
        Draw_Command_Buffer *buffer = batch->buffer;

        Textured_Vertex *vertices = &buffer->vertices[buffer->vertex_count];
        u32 *indices = &buffer->indices[buffer->index_count];

        vertices[0] = { p0, uv0, c0 };
        vertices[1] = { p1, uv1, c1 };
        vertices[2] = { p2, uv2, c2 };
        vertices[3] = { p3, uv3, c3 };

        u32 offset = buffer->vertex_count;

        indices[0] = offset + 0;
        indices[1] = offset + 1;
        indices[2] = offset + 3;

        indices[3] = offset + 1;
        indices[4] = offset + 3;
        indices[5] = offset + 2;

        buffer->vertex_count += 4;
        buffer->index_count  += 6;

        vertex_batch->vertex_count += 4;
        vertex_batch->index_count  += 6;
    }
}


internal void DrawQuad(Render_Batch *batch, Image_Handle image,
        v3 centre, v2 dim, f32 angle = 0, v4 colour = V4(1, 1, 1, 1))
{
    v2 half_dim = 0.5f * dim;

    u32 packed_colour = ABGRPack(colour);
    v2 rot = Arm2(angle);

    v3 p0 = centre + V3(Rotate(V2(-half_dim.x, half_dim.y), rot));
    v3 p1 = centre + V3(Rotate(-half_dim, rot));
    v3 p2 = centre + V3(Rotate(V2(half_dim.x, -half_dim.y), rot));
    v3 p3 = centre + V3(Rotate(half_dim, rot));

    Texture_Handle texture = GetImageData(batch->assets, image);
    DrawQuad(batch, texture,
            p0, V2(0, 0), packed_colour,
            p1, V2(0, 1), packed_colour,
            p2, V2(1, 1), packed_colour,
            p3, V2(1, 0), packed_colour);
}

internal void DrawQuad(Render_Batch *batch, Image_Handle image,
        v2 centre, v2 dim, f32 angle = 0, v4 colour = V4(1, 1, 1, 1))
{
    DrawQuad(batch, image, V3(centre), dim, angle, colour);
}


internal void DrawQuad(Render_Batch *batch, Image_Handle image,
        v3 centre, f32 scale, f32 angle, v4 colour = V4(1, 1, 1, 1))
{
    Amt_Image *info = GetImageInfo(batch->assets, image);

    v2 dim;
    if (info->width > info->height) {
        dim.w = 1;
        dim.h = info->height / cast(f32) info->width;
    }
    else {
        dim.w = info->width / cast(f32) info->height;
        dim.h = 1;
    }

    dim *= scale;

    DrawQuad(batch, image, centre, dim, angle, colour);
}

internal void DrawQuad(Render_Batch *batch, Image_Handle image,
        v2 centre, f32 scale, f32 angle, v4 colour = V4(1, 1, 1, 1))
{
    DrawQuad(batch, image, V3(centre), scale, angle, colour);
}

internal void DrawCircle(Render_Batch *batch, Image_Handle image,
        v2 centre, f32 radius, f32 angle = 0, v4 colour = V4(1, 1, 1, 1), u32 segment_count = 40)
{
    Texture_Handle texture = GetImageData(batch->assets, image);
    Draw_Command_Vertex_Batch *vertex_batch = DrawCommandVertexBatch(batch, texture);
    if (vertex_batch) {
        Draw_Command_Buffer *buffer = batch->buffer;
        u32 packed_colour = ABGRPack(colour);

        Textured_Vertex *vertices = &buffer->vertices[buffer->vertex_count];
        u32 *indices = &buffer->indices[buffer->index_count];

        vertices[0] = { V3(centre), V2(0.5, 0.5), packed_colour };

        u32 offset = buffer->vertex_count;

        f32 cur_angle = angle;
        f32 angle_per_segment = (2.0f * PI32) / cast(f32) segment_count;
        for (u32 it = 0; it < (segment_count - 1); ++it) {
            v2 arm = Arm2(cur_angle);

            v3 p  = V3((radius * arm) + centre);
            v2 uv = V2(0.5f * (1.0f + arm.x), 0.5f * (1.0f + arm.y));

            vertices[it + 1] = { p, uv, packed_colour };
            cur_angle += angle_per_segment;

            indices[0] = offset;
            indices[1] = offset + it + 1;
            indices[2] = offset + it + 2;

            indices += 3;
        }

        v2 arm = Arm2(cur_angle);
        v2 uv  = V2(0.5f * (1.0f + arm.x), 0.5f * (1.0f + arm.y));

        vertices[segment_count] = { V3((radius * arm) + centre), uv, packed_colour };

        indices[0] = offset;
        indices[1] = offset + segment_count;
        indices[2] = offset + 1;

        buffer->vertex_count += (segment_count + 1);
        buffer->index_count  += (3 * segment_count);

        vertex_batch->vertex_count += (segment_count + 1);
        vertex_batch->index_count  += (3 * segment_count);
    }
}

internal void DrawLine(Render_Batch *batch, v2 start, v2 end,
        v4 start_colour = V4(1, 1, 1, 1), v4 end_colour = V4(1, 1, 1, 1), f32 thickness = 0.03)
{
    v2 perp = Normalise(Perp(end - start));
    thickness *= 0.5f;

    v3 p0 = V3(start - (thickness * perp), 0);
    v3 p1 = V3(end   - (thickness * perp), 0);
    v3 p2 = V3(end   + (thickness * perp), 0);
    v3 p3 = V3(start + (thickness * perp), 0);

    u32 start_packed = ABGRPack(start_colour);
    u32 end_packed   = ABGRPack(end_colour);

    u32 c0 = start_packed;
    u32 c1 = end_packed;
    u32 c2 = end_packed;
    u32 c3 = start_packed;

    Texture_Handle texture = { 0 };
    DrawQuad(batch, texture,
            p0, V2(0, 0), c0,
            p1, V2(0, 1), c1,
            p2, V2(1, 1), c2,
            p3, V2(1, 0), c3);
}

internal void DrawQuadOutline(Render_Batch *batch, v2 centre, v2 dim,
        f32 angle = 0, v4 colour = V4(1, 1, 1, 1), f32 thickness = 0.03)
{
    v2 half_dim = 0.5f * dim;

    v2 start, end;
    v2 rot = Arm2(angle);

    start = centre + Rotate(V2(-half_dim.x, half_dim.y), rot);
    end   = centre + Rotate(-half_dim, rot);
    DrawLine(batch, start, end, colour, colour, thickness);

    start = end;
    end   = centre + Rotate(V2(half_dim.x, -half_dim.y), rot);
    DrawLine(batch, start, end, colour, colour, thickness);

    start = end;
    end   = centre + Rotate(half_dim, rot);
    DrawLine(batch, start, end, colour, colour, thickness);

    start = end;
    end   = centre + Rotate(V2(-half_dim.x, half_dim.y), rot);
    DrawLine(batch, start, end, colour, colour, thickness);
}


internal Animation CreateAnimation(Image_Handle image, u32 rows, u32 columns, f32 time_per_frame) {
    Animation result = {};
    result.image = image;

    result.time_per_frame = time_per_frame;

    result.rows = rows;
    result.columns = columns;
    result.total_frames = (rows * columns);

    return result;
}

internal void UpdateAnimation(Animation *anim, f32 delta_time) {
    anim->time += delta_time;
    if (anim->time >= anim->time_per_frame) {
        anim->time = 0;
        anim->current_frame = (anim->current_frame + 1) % anim->total_frames;
    }
}

internal void DrawAnimation(Render_Batch *batch, Animation *anim, f32 delta_time,
        v3 centre, v2 scale, f32 angle = 0, v4 colour = V4(1, 1, 1, 1))
{
    Amt_Image *info = GetImageInfo(batch->assets, anim->image);

    v2 dim = V2(info->width / cast(f32) anim->rows, info->height / cast(f32) anim->columns);
    if (dim.w > dim.h) {
        dim.h = dim.h / dim.w;
        dim.w = 1;
    }
    else {
        dim.w = dim.w / dim.h;
        dim.h = 1;
    }

    dim *= scale;
    v2 half_dim = 0.5f * dim;

    u32 packed_colour = ABGRPack(colour);
    v2 rot = Arm2(angle);

    v3 p0 = centre + V3(Rotate(V2(-half_dim.x, half_dim.y), rot));
    v3 p1 = centre + V3(Rotate(-half_dim, rot));
    v3 p2 = centre + V3(Rotate(V2(half_dim.x, -half_dim.y), rot));
    v3 p3 = centre + V3(Rotate(half_dim, rot));

    v2 uv_size = V2(1.0f / cast(f32) anim->rows, 1.0f / cast(f32) anim->columns);
    v2 uv_min  = V2(uv_size.x * (anim->current_frame / anim->columns),
                    uv_size.y * (anim->current_frame % anim->columns));

    v2 uv0 = uv_min;
    v2 uv1 = V2(uv_min.x, uv_min.y + uv_size.y);
    v2 uv2 = uv_min + uv_size;
    v2 uv3 = V2(uv_min.x + uv_size.x, uv_min.y);

    Texture_Handle texture = GetImageData(batch->assets, anim->image);
    DrawQuad(batch, texture,
            p0, uv0, packed_colour,
            p1, uv1, packed_colour,
            p2, uv2, packed_colour,
            p3, uv3, packed_colour);

}
