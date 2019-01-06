from collections import Iterable

import pcb as pm

max_x = 2500
max_y = 4000

lth = 10

pcb = pm.pcb_pcb(max_x, max_y)
l1 = pcb.add_layer(1, "top", "copper")
l10 = pcb.add_layer(10, "top silk", "silk")

def obj_array(objtype, n, **kwargs):
  arr = []
  if not isinstance(n, Iterable): n = range(n)
  for i in n:
    objkws = {}
    for k in kwargs: 
      objkws[k] = kwargs[k](i) if callable(kwargs[k]) else kwargs[k]

    o = objtype(**objkws)
    arr.append(o)

  return arr

row_spacing = 75

#
# Arc polygon clearance tests
#
def arc_clear_arr(x0, y0):
  objs = obj_array(pm.pcb_arc, range(1, 11), 
                   x=x0, y=lambda i: y0+i*row_spacing,
                   cl=lambda i:2*i)

  labs = obj_array(pm.pcb_text, range(1, 11),
                   x=x0, y=lambda i: y0-50+i*row_spacing,
                   txt=lambda i: "{} mils".format(i), scale=50)

  l1.objects += objs
  l10.objects += labs

poly = pm.pcb_polygon()
poly.rect(50, 100, 1500, 1000)
l1.objects.append(poly)

l10.objects.append(pm.pcb_text(100, 150, "Arc clearance", 50))
l10.objects.append(pm.pcb_text(100, 175, "In polygon", 50))
arc_clear_arr(150, 200)

l10.objects.append(pm.pcb_text(500, 150, "Arc clearance", 50))
l10.objects.append(pm.pcb_text(500, 175, "In object clearance", 50))
arc_clear_arr(550, 200)
# generate some clearance
line = pm.pcb_line(600, 250, 600, 950, cl=250)
l1.objects.append(line)

l10.objects.append(pm.pcb_text(1000, 150, "Arc clearance", 50))
l10.objects.append(pm.pcb_text(1000, 175, "In polygon hole", 50))
arc_clear_arr(1050, 200)
# generate some clearance
hole = poly.new_hole()
hole.rect(950, 150, 200, 900)

l10.objects.append(pm.pcb_text(1500, 150, "Arc clearance", 50))
l10.objects.append(pm.pcb_text(1500, 175, "On polygon edge", 50))
arc_clear_arr(1550, 200)


l10.objects.append(pm.pcb_text(2000, 150, "Arc clearance", 50))
l10.objects.append(pm.pcb_text(2000, 175, "Outside polygon", 50))
arc_clear_arr(2050, 200)

#
# Arc clearance to polygon
#
def arc_shift_arr(x0, y0, **kwargs):
  objs = obj_array(pm.pcb_arc, range(0, 11), 
                   x=lambda i: x0-i, y=lambda i: y0+i*row_spacing,
                   cl=0, **kwargs)

  labs = obj_array(pm.pcb_text, range(0, 11),
                   x=x0, y=lambda i: y0-50+i*row_spacing,
                   txt=lambda i: "{} mils".format(i), scale=50)

  l1.objects += objs
  l10.objects += labs

poly = pm.pcb_polygon()
poly.rect(155, 1200, 1395, 1000)
l1.objects.append(poly)

l10.objects.append(pm.pcb_text(100, 1200, "Arc-polygon spacing", 50))
l10.objects.append(pm.pcb_text(100, 1225, "Polygon clearance = 0", 50))
l10.objects.append(pm.pcb_text(100, 1250, "Arc-cap to polygon edge", 50))
arc_shift_arr(150, 1350)

l10.objects.append(pm.pcb_text(500, 1200, "Arc-polygon spacing", 50))
l10.objects.append(pm.pcb_text(500, 1225, "Polygon clearance = 0", 50))
l10.objects.append(pm.pcb_text(500, 1250, "Arc-cap to obj clearance edge", 50))
arc_shift_arr(595, 1350)
# generate some clearance
line = pm.pcb_line(500, 1250, 500, 2050, cl=190)
l1.objects.append(line)

l10.objects.append(pm.pcb_text(1000, 1200, "Arc-polygon spacing", 50))
l10.objects.append(pm.pcb_text(1000, 1225, "Polygon clearance = 0", 50))
l10.objects.append(pm.pcb_text(1000, 1250, "Arc-cap to poly hole edge", 50))
arc_shift_arr(1145, 1350)
# generate some clearance
hole = poly.new_hole()
hole.rect(950, 1200, 200, 950)


poly = pm.pcb_polygon()
poly.rect(205, 2400, 1395, 1000)
l1.objects.append(poly)

l10.objects.append(pm.pcb_text(100, 2300, "Arc-polygon spacing", 50))
l10.objects.append(pm.pcb_text(100, 2325, "Polygon clearance = 0", 50))
l10.objects.append(pm.pcb_text(100, 2350, "Arc to polygon edge", 50))
arc_shift_arr(150, 2475, sa=135, da=90)

l10.objects.append(pm.pcb_text(500, 2300, "Arc-polygon spacing", 50))
l10.objects.append(pm.pcb_text(500, 2325, "Polygon clearance = 0", 50))
l10.objects.append(pm.pcb_text(500, 2350, "Arc to obj clearance edge", 50))
arc_shift_arr(545, 2475, sa=135, da=90)
# generate some clearance
line = pm.pcb_line(500, 2450, 500, 3250, cl=190)
l1.objects.append(line)

l10.objects.append(pm.pcb_text(1000, 2300, "Arc-polygon spacing", 50))
l10.objects.append(pm.pcb_text(1000, 2325, "Polygon clearance = 0", 50))
l10.objects.append(pm.pcb_text(1000, 2350, "Arc to poly hole edge", 50))
arc_shift_arr(1095, 2475, sa=135, da=90)
# generate some clearance
hole = poly.new_hole()
hole.rect(950, 2400, 200, 950)



with open("test.pcb", 'w') as f:
  f.write(str(pcb))
