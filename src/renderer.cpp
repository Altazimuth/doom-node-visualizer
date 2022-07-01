#include "renderer.h"
#include "map.h"
#include "system.h"
#include "memory.h"
#include "vectors.h"

#include "math.h"



i32 pointOnLineSide(f32 x, f32 y, const Node& node) {
	return (y - node.y) * node.dx > node.dy * (x - node.x);
}


i32 pointOnLineSide(f32 testx, f32 testy, f32 linex, f32 liney, f32 dx, f32 dy) {
	i32 result = (testy - liney) * dx > dy * (testx - linex);
	return result;
}


void drawLine(DrawContext &context, i32 x1, i32 y1, i32 x2, i32 y2, Color color) {
	i32 dx = x2 - x1;
	i32 dy = y2 - y1;

	if (abs(dx) >= abs(dy)) {
		if (x2 < x1) {
			i32 temp = x2;
			x2 = x1;
			x1 = temp;

			temp = y2;
			y2 = y1;
			y1 = temp;

			dx = x2 - x1;
			dy = y2 - y1;
		}

		if (x2 < 0 || x1 >= context.w) return;
		if ((y2 < 0 && y1 < 0) || (y2 >= context.h && y1 >= context.h)) return;
		
		i32 clipx = x1 < 0 ? -x1 : 0;

		f32 ystep = (f32)dy / (f32)dx;
		f32 yf = y1 + clipx * ystep;

		if (x2 >= context.w) x2 = context.w - 1;

		for (i32 x = x1 < 0 ? 0 : x1; x <= x2; ++x) {
			i32 y = (i32)yf;

			if (y >= 0 && y < context.h) {
				u32* dest = (u32*)(context.pixels + (y * context.pitch)) + x;

				*dest = (color.r << context.rshift)
					| (color.g << context.gshift)
					| (color.b << context.bshift);
			}

			yf += ystep;
		}
	}
	else {
		if (y2 < y1) {
			i32 temp = x2;
			x2 = x1;
			x1 = temp;

			temp = y2;
			y2 = y1;
			y1 = temp;

			dx = x2 - x1;
			dy = y2 - y1;
		}

		if (y2 < 0 || y1 >= context.h) return;
		if ((x2 < 0 && x1 < 0) || (x2 >= context.w && x1 >= context.w)) return;

		i32 clipy = y1 < 0 ? -y1 : 0;
		f32 xstep = (f32)dx / (f32)dy;
		f32 xf = x1 + clipy * xstep;

		if (y2 >= context.h) y2 = context.h - 1;

		for (i32 y = y1 < 0 ? 0 : y1; y <= y2; ++y) {
			i32 x = (int)xf;

			if (x >= 0 && x < context.w) {
				u32* dest = (u32*)(context.pixels + (y * context.pitch)) + x;

				*dest = (color.r << context.rshift)
					| (color.g << context.gshift)
					| (color.b << context.bshift);
			}

			xf += xstep;
		}
	}
}


void drawWorldLine(View &view, DrawContext &context, f32 fx1, f32 fy1, f32 fx2, f32 fy2, Color color) {
	f32 x_offset = context.xcenter - view.offset.x;
	f32 y_offset = context.ycenter + view.offset.y;

	// TODO: The casting should really be done inside drawLine so sub-pixel calculations can be done
	i32 x1 = (i32)(x_offset + (fx1 * view.zoom));
	i32 x2 = (i32)(x_offset + (fx2 * view.zoom));
	i32 y1 = (i32)(y_offset - (fy1 * view.zoom));
	i32 y2 = (i32)(y_offset - (fy2 * view.zoom));

	drawLine(context, x1, y1, x2, y2, color);
}


static void drawWorldBox(View &view, DrawContext &context, const f32 bbox[4], Color color) {
	f32 fx1, fy1, fx2, fy2;

	fx1 = bbox[BoxLeft];
	fy1 = bbox[BoxTop];
	fx2 = bbox[BoxRight];
	fy2 = bbox[BoxBottom];

	drawWorldLine(view, context, fx1, fy1, fx1, fy2, color);
	drawWorldLine(view, context, fx2, fy1, fx2, fy2, color);
	drawWorldLine(view, context, fx1, fy1, fx2, fy1, color);
	drawWorldLine(view, context, fx1, fy2, fx2, fy2, color);
}


static Color NormalLine = { 79, 79, 79 };
static Color SplitLine = { 119, 255, 111 };
static Color SplitRay = { 32, 68, 24 };
static Color LightBox = { 0, 0, 203 };
static Color DimBox = { 0, 0, 83 };


static void renderSubSector(View &view, DrawContext &context, Map *map, i16 subsector) {
	auto ss = map->subsectors[subsector];

	for (i16 i = 0; i < ss.numsegs; ++i) {
		auto seg = map->segs[i + ss.firstseg];

		auto v1 = map->vertexes[seg.v1];
		auto v2 = map->vertexes[seg.v2];

		drawWorldLine(view, context, v1.x, v1.y, v2.x, v2.y, NormalLine);
	}
}

static void renderBspNode(Map* map, View &view, DrawContext &context, i32 nodeNum, RenderState& state) {
	i32 cursor = nodeNum;
	const Node* splitNode = 0;

	while(!(cursor & SubsectorChildFlag)) {
		const Node& node = map->nodes[cursor];

		if (cursor == state.selectedNode) {
			drawWorldBox(view, context, node.bbox[state.highlightedSide ^ 1], DimBox);

			f32 xextent = node.dx * 128;
			f32 yextent = node.dy * 128;
			drawWorldLine(view, context, node.x - xextent, node.y - yextent, node.x + xextent * 2, node.y + yextent * 2, SplitRay);

			splitNode = &node;
		}

		i32 side = 0;
		renderBspNode(map, view, context, node.children[side], state);

		side ^= 1;
		cursor = node.children[side];
	}

	renderSubSector(view, context, map, cursor & ~SubsectorChildFlag);

	if (splitNode != 0) {
		drawWorldBox(view, context, splitNode->bbox[state.highlightedSide], LightBox);

		drawWorldLine(view, context, splitNode->x, splitNode->y, splitNode->x + splitNode->dx, splitNode->y + splitNode->dy, SplitLine);
	}
}


void initRenderer(DrawContext drawContext) {
}


View calculateView(Map* map, DrawContext& drawContext, i32 nodeNum) {
	View result = {};

	if (map == 0) return result;

	v2f v1, v2;
	auto node = map->nodes.data + nodeNum;

	f32 bufferFactor = 1.02;

	v1.x = min(node->bbox[0][BoxLeft], node->bbox[1][BoxLeft]);
	v1.y = min(node->bbox[0][BoxBottom], node->bbox[1][BoxBottom]);

	v2.x = max(node->bbox[0][BoxRight], node->bbox[1][BoxRight]);
	v2.y = max(node->bbox[0][BoxTop], node->bbox[1][BoxTop]);

	result.offset = v1 + ((v2 - v1) / 2.0f);
	result.zoom = fminf(
		(f32)drawContext.w / (fabsf(v2.x - v1.x) * bufferFactor),
		(f32)drawContext.h / (fabsf(v2.y - v1.y) * bufferFactor)
	);

	result.offset *= result.zoom;

	return result;
}


void clearScreen(DrawContext& drawContext) {
	u32  src = ((0 << drawContext.rshift) & drawContext.rmask) |
		((0 << drawContext.gshift) & drawContext.gmask) |
		((0 << drawContext.bshift) & drawContext.bmask);

	for (i32 y = 0; y < drawContext.h; ++y) {
		u32* dest = (u32*)(drawContext.pixels + (y * drawContext.pitch));

		for (i32 x = 0; x < drawContext.w; ++x) {
			*dest = src;
			dest++;
		}
	}
}

void renderMap(Map* map, View& view, DrawContext& drawContext, RenderState& state) {
	clearScreen(drawContext);

	renderBspNode(map, view, drawContext, (i32)map->nodes.length - 1, state);
}