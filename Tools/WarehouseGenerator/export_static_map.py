"""Export navigation footprints from the editable warehouse, without SLAM."""
import json
import math
from pathlib import Path

import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parent
RESOLUTION = 0.05


def bounds(objects):
    points = [o.matrix_world @ Vector(c) for o in objects for c in o.bound_box]
    if not points:
        raise ValueError("Empty footprint")
    return [min(p.x for p in points), min(p.y for p in points),
            max(p.x for p in points), max(p.y for p in points)]


def export_map():
    bpy.context.view_layer.update()
    floor = bpy.data.objects['SM_Warehouse_Floor']
    xmin, ymin, xmax, ymax = bounds([floor])
    width = math.ceil((xmax - xmin) / RESOLUTION)
    height = math.ceil((ymax - ymin) / RESOLUTION)
    rectangles = []
    shell = bpy.data.collections['Warehouse_Shell']
    for obj in shell.objects:
        if 'Wall' in obj.name or obj.name.startswith('SM_Structure_Column'):
            rectangles.append((obj.name, bounds([obj])))
    for name in ('A1', 'A2', 'A3'):
        # Reserve the whole rack footprint, including gaps below elevated shelves.
        objects = [o for o in bpy.data.collections[f'Rack_Row_{name}'].objects
                   if o.type == 'MESH' and 'Sign' not in o.name]
        rectangles.append((name, bounds(objects)))
    props = bpy.data.collections['Loose_Props']
    for prefix in sorted({'_'.join(o.name.split('_')[:2]) for o in props.objects}):
        rectangles.append((prefix, bounds([o for o in props.objects
                                           if o.name.startswith(prefix + '_')])))
    pixels = bytearray([254]) * (width * height)
    for _, (left, bottom, right, top) in rectangles:
        # Fill every intersecting cell. PGM rows run from north to south.
        x0 = max(0, math.floor((left - xmin) / RESOLUTION + 1e-5))
        x1 = min(width, math.ceil((right - xmin) / RESOLUTION - 1e-5))
        y0 = max(0, math.floor((bottom - ymin) / RESOLUTION + 1e-5))
        y1 = min(height, math.ceil((top - ymin) / RESOLUTION - 1e-5))
        for y in range(y0, y1):
            offset = (height - 1 - y) * width
            pixels[offset + x0:offset + x1] = bytes(x1 - x0)
    output = ROOT / 'Output' / 'ROS2'
    output.mkdir(parents=True, exist_ok=True)
    (output / 'warehouse_map.pgm').write_bytes(
        f'P5\n{width} {height}\n255\n'.encode('ascii') + pixels)
    image = bpy.data.images.new('WarehouseMapPreview', width=width, height=height)
    rgba = [0.0] * (width * height * 4)
    for source_row in range(height):
        blender_row = height - 1 - source_row
        for x in range(width):
            value = pixels[source_row * width + x] / 255.0
            index = (blender_row * width + x) * 4
            rgba[index:index + 4] = (value, value, value, 1.0)
    image.pixels = rgba
    image.filepath_raw = str(output / 'warehouse_map_preview.png')
    image.file_format = 'PNG'
    image.save()
    bpy.data.images.remove(image)
    (output / 'warehouse_map.yaml').write_text(
        'image: warehouse_map.pgm\nmode: trinary\n'
        f'resolution: {RESOLUTION}\norigin: [{xmin}, {ymin}, 0.0]\n'
        'negate: 0\noccupied_thresh: 0.65\nfree_thresh: 0.25\n', encoding='utf-8')
    (output / 'map_metadata.json').write_text(json.dumps({
        'width': width, 'height': height, 'resolution': RESOLUTION,
        'origin': [xmin, ymin, 0], 'footprints': rectangles,
        'source': bpy.data.filepath,
        'assumption': 'Default -Y forward/Z up FBX import; no additional UE actor transforms. '
                      'Blender XY -> UE (100X,-100Y) -> ROS (X,Y).',
        'policy': 'Conservative complete rack footprints; no robot-radius inflation.',
    }, indent=2), encoding='utf-8')
    # Verify walls, an aisle, and all rack centres before reporting success.
    def value(x, y):
        col = int((x - xmin) / RESOLUTION)
        row = height - 1 - int((y - ymin) / RESOLUTION)
        return pixels[row * width + col]
    assert value(0, 3.3) == 254, 'Aisle must remain free'
    for x, y in [(0, 0), (0, -6.6), (0, 6.6),
                 (xmin + .05, 0), (xmax - .05, 0),
                 (0, ymin + .05), (0, ymax - .05)]:
        assert value(x, y) == 0, (x, y)
    print(f'STATIC_MAP_OK {width}x{height} origin=({xmin},{ymin}) {output}')


if __name__ == '__main__':
    export_map()
