#pragma once

#include "types.h"
#include "map.h"
#include "vectors.h"

struct View {
	v2f offset;
	f32 zoom;
};

struct RenderState {
	i32 selectedNode;
	i32 highlightedSide;
};

struct DrawContext {
	i32 w;
	i32 h;
	i32 xcenter;
	i32 ycenter;
	i32 pitch;
	i32 bytesPerPixel;
	u8* pixels;
	u32 rshift, gshift, bshift;
	u32 rmask, gmask, bmask;
};
View calculateView(Map* map, DrawContext& drawContext, i32 nodeNum);

i32 pointOnLineSide(f32 x, f32 y, const Node& node);
i32 pointOnLineSide(f32 testx, f32 testy, f32 linex, f32 liney, f32 dx, f32 dy);

void drawLine(DrawContext& context, i32 x1, i32 y1, i32 x2, i32 y2, Color color);
void drawWorldLine(View& view, DrawContext& context, f32 fx1, f32 fy1, f32 fx2, f32 fy2, Color color);

void initRenderer(DrawContext drawContext);

void renderMap(Map* map, View& view, DrawContext& drawContext, RenderState& state);
