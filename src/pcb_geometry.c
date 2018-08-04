/*!
 * \file src/pcb_geometry.c
 *
 * \brief Geometry functions on pcb objects.
 *
 * <hr>
 *
 * <h1><b>Copyright.</b></h1>\n
 *
 * PCB, interactive printed circuit board design
 *
 * Copyright (C) 1994,1995,1996 Thomas Nau
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Contact addresses for paper mail and Email:
 *
 * Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *
 * Thomas.Nau@rz.uni-ulm.de
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>

#include "global.h"

#include "data.h"
#include "misc.h" /* Distance */
#include "pcb_geometry.h"
#include "polygon.h"

static void
get_arc_ends (Coord *box, ArcType *arc)
{
  box[0] = arc->X - arc->Width  * cos (M180 * arc->StartAngle);
  box[1] = arc->Y + arc->Height * sin (M180 * arc->StartAngle);
  box[2] = arc->X - arc->Width  * cos (M180 * (arc->StartAngle + arc->Delta));
  box[3] = arc->Y + arc->Height * sin (M180 * (arc->StartAngle + arc->Delta));
}

/*!
 * \brief Writes vertices of a squared line.
 */
static void 
form_slanted_rectangle (PointType p[4], LineType *l)
{
   double dwx = 0, dwy = 0;
   if (l->Point1.Y == l->Point2.Y)
     dwx = l->Thickness / 2.0;
   else if (l->Point1.X == l->Point2.X)
     dwy = l->Thickness / 2.0;
   else 
     {
       Coord dX = l->Point2.X - l->Point1.X;
       Coord dY = l->Point2.Y - l->Point1.Y;
       double r = Distance (l->Point1.X, l->Point1.Y, l->Point2.X, l->Point2.Y);
       dwx = l->Thickness / 2.0 / r * dX;
       dwy = l->Thickness / 2.0 / r * dY;
     }
    p[0].X = l->Point1.X - dwx + dwy; p[0].Y = l->Point1.Y - dwy - dwx;
    p[1].X = l->Point1.X - dwx - dwy; p[1].Y = l->Point1.Y - dwy + dwx;
    p[2].X = l->Point2.X + dwx - dwy; p[2].Y = l->Point2.Y + dwy + dwx;
    p[3].X = l->Point2.X + dwx + dwy; p[3].Y = l->Point2.Y + dwy - dwx;
}

/*!
 * \brief Checks if a point is on a pin.
 */
bool
IsPointOnPin (Coord X, Coord Y, Coord Radius, PinType *pin)
{
  Coord t = PIN_SIZE (pin) / 2;
  if (TEST_FLAG (SQUAREFLAG, pin))
    {
      BoxType b;

      b.X1 = pin->X - t;
      b.X2 = pin->X + t;
      b.Y1 = pin->Y - t;
      b.Y2 = pin->Y + t;
      if (IsPointInBox (X, Y, &b, Radius))
	return true;
    }
  else if (Distance (pin->X, pin->Y, X, Y) <= Radius + t)
    return true;
  return false;
}

/*!
 * \brief Checks if a rat-line end is on a PV.
 */
bool
IsPointOnLineEnd (Coord X, Coord Y, RatType *Line)
{
  if (((X == Line->Point1.X) && (Y == Line->Point1.Y)) ||
      ((X == Line->Point2.X) && (Y == Line->Point2.Y)))
    return (true);
  return (false);
}

/*!
 * \brief Checks if a line intersects with a PV.
 *
 * <pre>
 * let the point be (X,Y) and the line (X1,Y1)(X2,Y2)
 * the length of the line is
 *
 *   L = ((X2-X1)^2 + (Y2-Y1)^2)^0.5
 * 
 * let Q be the point of perpendicular projection of (X,Y) onto the line
 *
 *   QX = X1 + D1*(X2-X1) / L
 *   QY = Y1 + D1*(Y2-Y1) / L
 * 
 * with (from vector geometry)
 *
 *        (Y1-Y)(Y1-Y2)+(X1-X)(X1-X2)
 *   D1 = ---------------------------
 *                     L
 *
 *   D1 < 0   Q is on backward extension of the line
 *   D1 > L   Q is on forward extension of the line
 *   else     Q is on the line
 *
 * the signed distance from (X,Y) to Q is
 *
 *        (Y2-Y1)(X-X1)-(X2-X1)(Y-Y1)
 *   D2 = ----------------------------
 *                     L
 *
 * Finally, D1 and D2 are orthogonal, so we can sum them easily
 * by pythagorean theorem.
 * </pre>
 */
bool
IsPointOnLine (Coord X, Coord Y, Coord Radius, LineType *Line)
{
  double D1, D2, L;

  /* Get length of segment */
  L = Distance (Line->Point1.X, Line->Point1.Y, Line->Point2.X, Line->Point2.Y);
  if (L < 0.1)
    return Distance (X, Y, Line->Point1.X, Line->Point1.Y) < Radius + Line->Thickness / 2;

  /* Get distance from (X1, Y1) to Q (on the line) */
  D1 = ((double) (Y - Line->Point1.Y) * (Line->Point2.Y - Line->Point1.Y)
        + (double) (X - Line->Point1.X) * (Line->Point2.X - Line->Point1.X)) / L;
  /* Translate this into distance to Q from segment */
  if (D1 < 0)       D1 = -D1;
  else if (D1 > L)  D1 -= L;
  else              D1 = 0;
  /* Get distance from (X, Y) to Q */
  D2 = ((double) (X - Line->Point1.X) * (Line->Point2.Y - Line->Point1.Y)
        - (double) (Y - Line->Point1.Y) * (Line->Point2.X - Line->Point1.X)) / L;
  /* Total distance is then the pythagorean sum of these */
  return hypot (D1, D2) <= Radius + Line->Thickness / 2;
}

/*!
 * \brief Checks if a line crosses a rectangle.
 */
bool
IsLineInRectangle (Coord X1, Coord Y1, Coord X2, Coord Y2, LineType *Line, Coord bloat)
{
  LineType line;

  /* first, see if point 1 is inside the rectangle */
  /* in case the whole line is inside the rectangle */
  if (X1 < Line->Point1.X && X2 > Line->Point1.X &&
      Y1 < Line->Point1.Y && Y2 > Line->Point1.Y)
    return (true);
  /* construct a set of dummy lines and check each of them */
  line.Thickness = 0;
  line.Flags = NoFlags ();

  /* upper-left to upper-right corner */
  line.Point1.Y = line.Point2.Y = Y1;
  line.Point1.X = X1;
  line.Point2.X = X2;
  if (LineLineIntersect (&line, Line, bloat))
    return (true);

  /* upper-right to lower-right corner */
  line.Point1.X = X2;
  line.Point1.Y = Y1;
  line.Point2.Y = Y2;
  if (LineLineIntersect (&line, Line, bloat))
    return (true);

  /* lower-right to lower-left corner */
  line.Point1.Y = Y2;
  line.Point1.X = X1;
  line.Point2.X = X2;
  if (LineLineIntersect (&line, Line, bloat))
    return (true);

  /* lower-left to upper-left corner */
  line.Point2.X = X1;
  line.Point1.Y = Y1;
  line.Point2.Y = Y2;
  if (LineLineIntersect (&line, Line, bloat))
    return (true);

  return (false);
}

/*!
 * \brief Checks if a point (of null radius) is in a slanted rectangle.
 *
 * TODO: Does this need to consider bloat?
 */
bool
IsPointInQuadrangle(PointType p[4], PointType *l)
{
  Coord dx, dy, x, y;
  double prod0, prod1;

  dx = p[1].X - p[0].X;
  dy = p[1].Y - p[0].Y;
  x = l->X - p[0].X;
  y = l->Y - p[0].Y;
  prod0 = (double) x * dx + (double) y * dy;
  x = l->X - p[1].X;
  y = l->Y - p[1].Y;
  prod1 = (double) x * dx + (double) y * dy;
  if (prod0 * prod1 <= 0)
    {
      dx = p[1].X - p[2].X;
      dy = p[1].Y - p[2].Y;
      prod0 = (double) x * dx + (double) y * dy;
      x = l->X - p[2].X;
      y = l->Y - p[2].Y;
      prod1 = (double) x * dx + (double) y * dy;
      if (prod0 * prod1 <= 0)
	return true;
    }
  return false;
}

bool
IsPinOnRat(PinType * pin, RatType * rat)
{
  return IsPointOnLineEnd(pin->X, pin->Y, rat);
}

bool
IsPinOnArc(PinType * pin, ArcType * arc, Coord bloat)
{
  if (TEST_FLAG(SQUAREFLAG, pin))
      return IsArcInRectangle(pin->X -MAX((pin->Thickness+1)/2 +bloat,0), /* X1 */
	                          pin->Y -MAX((pin->Thickness+1)/2 +bloat,0), /* Y1 */
			                  pin->X +MAX((pin->Thickness+1)/2 +bloat,0), /* X2 */
					          pin->Y +MAX((pin->Thickness+1)/2 +bloat,0), /* Y2 */
					          arc, bloat);
  else
	  return IsPointOnArc(pin->X, pin->Y,MAX(pin->Thickness/2.0 + bloat, 0.0), arc);

}

bool 
IsPinOnPad(PinType * pin, PadType * pad, Coord bloat)
{
  return IsPointInPad(pin->X, pin->Y, MAX(pin->Thickness/2 +bloat,0), pad);

}

/*!
 * \brief Checks if a line crosses a quadrangle: almost copied from
 * IsLineInRectangle().
 *
 * \note Actually this quadrangle is a slanted rectangle.
 */
bool
IsLineInQuadrangle (PointType p[4], LineType *Line, Coord bloat)
{
  LineType line;

  /* first, see if point 1 is inside the rectangle */
  /* in case the whole line is inside the rectangle */
  if (IsPointInQuadrangle(p,&(Line->Point1)))
    return true;
  if (IsPointInQuadrangle(p,&(Line->Point2)))
    return true;
  /* construct a set of dummy lines and check each of them */
  line.Thickness = 0;
  line.Flags = NoFlags ();

  /* upper-left to upper-right corner */
  line.Point1.X = p[0].X; line.Point1.Y = p[0].Y;
  line.Point2.X = p[1].X; line.Point2.Y = p[1].Y;
  if (LineLineIntersect (&line, Line, bloat))
    return (true);

  /* upper-right to lower-right corner */
  line.Point1.X = p[2].X; line.Point1.Y = p[2].Y;
  if (LineLineIntersect (&line, Line, bloat))
    return (true);

  /* lower-right to lower-left corner */
  line.Point2.X = p[3].X; line.Point2.Y = p[3].Y;
  if (LineLineIntersect (&line, Line, bloat))
    return (true);

  /* lower-left to upper-left corner */
  line.Point1.X = p[0].X; line.Point1.Y = p[0].Y;
  if (LineLineIntersect (&line, Line, bloat))
    return (true);

  return (false);
}
/*!
 * \brief Checks if an arc crosses a square.
 */
bool
IsArcInRectangle (Coord X1, Coord Y1, Coord X2, Coord Y2, ArcType *Arc, Coord bloat)
{
  LineType line;

  /* construct a set of dummy lines and check each of them */
  line.Thickness = 0;
  line.Flags = NoFlags ();

  /* upper-left to upper-right corner */
  line.Point1.Y = line.Point2.Y = Y1;
  line.Point1.X = X1;
  line.Point2.X = X2;
  if (LineArcIntersect (&line, Arc, bloat))
    return (true);

  /* upper-right to lower-right corner */
  line.Point1.X = line.Point2.X = X2;
  line.Point1.Y = Y1;
  line.Point2.Y = Y2;
  if (LineArcIntersect (&line, Arc, bloat))
    return (true);

  /* lower-right to lower-left corner */
  line.Point1.Y = line.Point2.Y = Y2;
  line.Point1.X = X1;
  line.Point2.X = X2;
  if (LineArcIntersect (&line, Arc, bloat))
    return (true);

  /* lower-left to upper-left corner */
  line.Point1.X = line.Point2.X = X1;
  line.Point1.Y = Y1;
  line.Point2.Y = Y2;
  if (LineArcIntersect (&line, Arc, bloat))
    return (true);

  return (false);
}

/*!
 * \brief Check if a circle of Radius with center at (X, Y) intersects
 * a Pad.
 *
 * Written to enable arbitrary pad directions; for rounded pads, too.
 */
bool
IsPointInPad (Coord X, Coord Y, Coord Radius, PadType *Pad)
{
  double r, Sin, Cos;
  Coord x; 
  Coord t2 = (Pad->Thickness + 1) / 2, range;
  PadType pad = *Pad;

  /* series of transforms saving range */
  /* move Point1 to the origin */
  X -= pad.Point1.X;
  Y -= pad.Point1.Y;

  pad.Point2.X -= pad.Point1.X;
  pad.Point2.Y -= pad.Point1.Y;
  /* so, pad.Point1.X = pad.Point1.Y = 0; */

  /* rotate round (0, 0) so that Point2 coordinates be (r, 0) */
  r = Distance (0, 0, pad.Point2.X, pad.Point2.Y);
  if (r < .1)
    {
      Cos = 1;
      Sin = 0;
    }
  else
    {
      Sin = pad.Point2.Y / r;
      Cos = pad.Point2.X / r;
    }
  x = X;
  X = X * Cos + Y * Sin;
  Y = Y * Cos - x * Sin;
  /* now pad.Point2.X = r; pad.Point2.Y = 0; */

  /* take into account the ends */
  if (TEST_FLAG (SQUAREFLAG, Pad))
    {
      r += Pad->Thickness;
      X += t2;
    }
  if (Y < 0)
    Y = -Y;	/* range value is evident now*/

  if (TEST_FLAG (SQUAREFLAG, Pad))
    {
      if (X <= 0)
	{
	  if (Y <= t2)
            range = -X;
          else
	    return Radius > Distance (0, t2, X, Y);
	}
      else if (X >= r)
	{
	  if (Y <= t2)
            range = X - r;
          else 
	    return Radius > Distance (r, t2, X, Y);
	}
      else
	range = Y - t2;
    }
  else/*Rounded pad: even more simple*/
    {
      if (X <= 0)
	return (Radius + t2) > Distance (0, 0, X, Y);
      else if (X >= r) 
	return (Radius + t2) > Distance (r, 0, X, Y);
      else
	range = Y - t2;
    }
  return range < Radius;
}

/*!
 * \brief .
 *
 * \note Assumes box has point1 with numerically lower X and Y
 * coordinates.
 */
bool
IsPointInBox (Coord X, Coord Y, BoxType *box, Coord Radius)
{
  Coord width, height, range;

  /* Compute coordinates relative to Point1 */
  X -= box->X1;
  Y -= box->Y1;

  width =  box->X2 - box->X1;
  height = box->Y2 - box->Y1;

  if (X <= 0)
    {
      if (Y < 0)
        return Radius > Distance (0, 0, X, Y);
      else if (Y > height)
        return Radius > Distance (0, height, X, Y);
      else
        range = -X;
    }
  else if (X >= width)
    {
      if (Y < 0)
        return Radius > Distance (width, 0, X, Y);
      else if (Y > height)
        return Radius > Distance (width, height, X, Y);
      else
        range = X - width;
    }
  else
    {
      if (Y < 0)
        range = -Y;
      else if (Y > height)
        range = Y - height;
      else
        return true;
    }

  return range < Radius;
}

/*!
 * \brief .
 *
 * \todo This code is BROKEN in the case of non-circular arcs, and in
 * the case that the arc thickness is greater than the radius.
 */
bool
IsPointOnArc (Coord X, Coord Y, Coord Radius, ArcType *Arc)
{
  /* Calculate angle of point from arc center */
  double p_dist = Distance (X, Y, Arc->X, Arc->Y);
  double p_cos = (X - Arc->X) / p_dist;
  Angle p_ang = acos (p_cos) * RAD_TO_DEG;
  Angle ang1, ang2;

  /* Convert StartAngle, Delta into bounding angles in [0, 720) */
  if (Arc->Delta > 0)
    {
      ang1 = NormalizeAngle (Arc->StartAngle);
      ang2 = NormalizeAngle (Arc->StartAngle + Arc->Delta);
    }
  else
    {
      ang1 = NormalizeAngle (Arc->StartAngle + Arc->Delta);
      ang2 = NormalizeAngle (Arc->StartAngle);
    }
  if (ang1 > ang2)
    ang2 += 360;
  /* Make sure full circles aren't treated as zero-length arcs */
  if (Arc->Delta == 360 || Arc->Delta == -360)
    ang2 = ang1 + 360;

  if (Y > Arc->Y)
    p_ang = -p_ang;
  p_ang += 180;

  /* Check point is outside arc range, check distance from endpoints */
  if (ang1 >= p_ang || ang2 <= p_ang)
    {
      Coord ArcX, ArcY;

      ArcX = Arc->X + Arc->Width *
              cos ((Arc->StartAngle + 180) / RAD_TO_DEG);
      ArcY = Arc->Y - Arc->Width *
              sin ((Arc->StartAngle + 180) / RAD_TO_DEG);
      if (Distance (X, Y, ArcX, ArcY) < Radius + Arc->Thickness / 2)
        return true;

      ArcX = Arc->X + Arc->Width *
              cos ((Arc->StartAngle + Arc->Delta + 180) / RAD_TO_DEG);
      ArcY = Arc->Y - Arc->Width *
              sin ((Arc->StartAngle + Arc->Delta + 180) / RAD_TO_DEG);
      if (Distance (X, Y, ArcX, ArcY) < Radius + Arc->Thickness / 2)
        return true;
      return false;
    }
  /* If point is inside the arc range, just compare it to the arc */
  return fabs (Distance (X, Y, Arc->X, Arc->Y) - Arc->Width) < Radius + Arc->Thickness / 2;
}

/*!
 * \brief Tests if point is same as line end point.
 */
bool
IsRatPointOnLineEnd (PointType *Point, LineType *Line)
{
  if ((Point->X == Line->Point1.X
       && Point->Y == Line->Point1.Y)
      || (Point->X == Line->Point2.X && Point->Y == Line->Point2.Y))
    return (true);
  return (false);
}

/*!
 * \brief Checks if an arc has a connection to a polygon.
 *
 * - first check if the arc can intersect with the polygon by
 *   evaluating the bounding boxes.
 * - check the two end points of the arc. If none of them matches
 * - check all segments of the polygon against the arc.
 *
 * The bloat parameter allows the calling function to specify if any additional
 * width should be added to the arc when looking for overlap, e.g. to check
 * clearances. bloat is a clearance, so, it's added on both sides of the arc.
 */
bool
IsArcInPolygon (ArcType *Arc, PolygonType *Polygon, Coord bloat)
{
  BoxType *Box = (BoxType *) Arc;

  /* arcs with clearance never touch polys */
  if (TEST_FLAG (CLEARPOLYFLAG, Polygon) && TEST_FLAG (CLEARLINEFLAG, Arc))
    return false;
  if (!Polygon->Clipped)
    return false;
  if (Box->X1 <= Polygon->Clipped->contours->xmax + bloat
      && Box->X2 >= Polygon->Clipped->contours->xmin - bloat
      && Box->Y1 <= Polygon->Clipped->contours->ymax + bloat
      && Box->Y2 >= Polygon->Clipped->contours->ymin - bloat)
    {
      POLYAREA *ap;

      if (!(ap = ArcPoly (Arc, Arc->Thickness + 2*bloat)))
        return false;           /* error */
      return isects (ap, Polygon, true);
    }
  return false;
}

/*!
 * \brief Checks if a line has a connection to a polygon.
 *
 * - first check if the line can intersect with the polygon by
 *   evaluating the bounding boxes
 * - check the two end points of the line. If none of them matches
 * - check all segments of the polygon against the line.
 *
 * The bloat parameter allows the calling function to specify if any additional
 * width should be added to the arc when looking for overlap, e.g. to check
 * clearances. bloat is a clearance, so, it's added on both sides of the arc.
 *
 * This function was corrected to use 2x bloat. Supposedly this breaks
 * something. If we figure out what, then we should change the parameter this
 * function is called with.
 */
bool
IsLineInPolygon (LineType *Line, PolygonType *Polygon, Coord bloat)
{
  BoxType *Box = (BoxType *) Line;
  POLYAREA *lp;

  /* lines with clearance never touch polygons */
  if (TEST_FLAG (CLEARPOLYFLAG, Polygon) && TEST_FLAG (CLEARLINEFLAG, Line))
    return false;
  if (!Polygon->Clipped)
    return false;
  if (TEST_FLAG(SQUAREFLAG,Line)&&(Line->Point1.X==Line->Point2.X||Line->Point1.Y==Line->Point2.Y))
     {
       Coord wid = (Line->Thickness + 2*bloat + 1) / 2;
       Coord x1, x2, y1, y2;

       x1 = MIN (Line->Point1.X, Line->Point2.X) - wid;
       y1 = MIN (Line->Point1.Y, Line->Point2.Y) - wid;
       x2 = MAX (Line->Point1.X, Line->Point2.X) + wid;
       y2 = MAX (Line->Point1.Y, Line->Point2.Y) + wid;
       return IsRectangleInPolygon (x1, y1, x2, y2, Polygon);
     }
  if (Box->X1 <= Polygon->Clipped->contours->xmax + bloat
      && Box->X2 >= Polygon->Clipped->contours->xmin - bloat
      && Box->Y1 <= Polygon->Clipped->contours->ymax + bloat
      && Box->Y2 >= Polygon->Clipped->contours->ymin - bloat)
    {
      if (!(lp = LinePoly (Line, Line->Thickness + 2*bloat)))
        return FALSE;           /* error */
      return isects (lp, Polygon, true);
    }
  return false;
}

/*!
 * \brief Checks if a pad connects to a non-clearing polygon.
 *
 * The polygon is assumed to already have been proven non-clearing.
 */
bool
IsPadInPolygon (PadType *pad, PolygonType *polygon, Coord bloat)
{
    return IsLineInPolygon ((LineType *) pad, polygon, bloat);
}

/*!
 * \brief Checks if a polygon has a connection to a second one.
 *
 * First check all points out of P1 against P2 and vice versa.
 * If both fail check all lines of P1 against the ones of P2.
 */
bool
IsPolygonInPolygon (PolygonType *P1, PolygonType *P2, Coord bloat)
{
  if (!P1->Clipped || !P2->Clipped)
    return false;
  assert (P1->Clipped->contours);
  assert (P2->Clipped->contours);

  /* first check if both bounding boxes intersect. If not, return quickly */
  if (P1->Clipped->contours->xmin - bloat > P2->Clipped->contours->xmax ||
      P1->Clipped->contours->xmax + bloat < P2->Clipped->contours->xmin ||
      P1->Clipped->contours->ymin - bloat > P2->Clipped->contours->ymax ||
      P1->Clipped->contours->ymax + bloat < P2->Clipped->contours->ymin)
    return false;

  /* first check un-bloated case */
  if (isects (P1->Clipped, P2, false))
    return TRUE;

  /* now the difficult case of bloated */
  if (bloat > 0)
    {
      PLINE *c;
      for (c = P1->Clipped->contours; c; c = c->next)
        {
          LineType line;
          VNODE *v = &c->head;
          if (c->xmin - bloat <= P2->Clipped->contours->xmax &&
              c->xmax + bloat >= P2->Clipped->contours->xmin &&
              c->ymin - bloat <= P2->Clipped->contours->ymax &&
              c->ymax + bloat >= P2->Clipped->contours->ymin)
            {

              line.Point1.X = v->point[0];
              line.Point1.Y = v->point[1];
              line.Thickness = 0;
              line.Clearance = 0;
              line.Flags = NoFlags ();
              for (v = v->next; v != &c->head; v = v->next)
                {
                  line.Point2.X = v->point[0];
                  line.Point2.Y = v->point[1];
                  SetLineBoundingBox (&line);
                  if (IsLineInPolygon (&line, P2, bloat))
                    return (true);
                  line.Point1.X = line.Point2.X;
                  line.Point1.Y = line.Point2.Y;
                }
            }
        }
    }

  return (false);
}


/*!
 * \brief Checks if two lines intersect.
 *
 * <pre>
 * From news FAQ:
 *
 * Let A,B,C,D be 2-space position vectors.
 *
 * Then the directed line segments AB & CD are given by:
 *
 *      AB=A+r(B-A), r in [0,1]
 *
 *      CD=C+s(D-C), s in [0,1]
 *
 * If AB & CD intersect, then
 *
 *      A+r(B-A)=C+s(D-C), or
 *
 *      XA+r(XB-XA)=XC+s*(XD-XC)
 *
 *      YA+r(YB-YA)=YC+s(YD-YC)  for some r,s in [0,1]
 *
 * Solving the above for r and s yields
 *
 *          (YA-YC)(XD-XC)-(XA-XC)(YD-YC)
 *      r = -----------------------------  (eqn 1)
 *          (XB-XA)(YD-YC)-(YB-YA)(XD-XC)
 *
 *          (YA-YC)(XB-XA)-(XA-XC)(YB-YA)
 *      s = -----------------------------  (eqn 2)
 *          (XB-XA)(YD-YC)-(YB-YA)(XD-XC)
 *
 * Let I be the position vector of the intersection point, then:
 *
 *      I=A+r(B-A) or
 *
 *      XI=XA+r(XB-XA)
 *
 *      YI=YA+r(YB-YA)
 *
 * By examining the values of r & s, you can also determine some
 * other limiting conditions:
 *
 *      If 0<=r<=1 & 0<=s<=1, intersection exists
 *
 *          r<0 or r>1 or s<0 or s>1 line segments do not intersect
 *
 *      If the denominator in eqn 1 is zero, AB & CD are parallel
 *
 *      If the numerator in eqn 1 is also zero, AB & CD are coincident
 *
 * If the intersection point of the 2 lines are needed (lines in this
 * context mean infinite lines) regardless whether the two line
 * segments intersect, then
 *
 *      If r>1, I is located on extension of AB
 *      If r<0, I is located on extension of BA
 *      If s>1, I is located on extension of CD
 *      If s<0, I is located on extension of DC
 *
 * Also note that the denominators of eqn 1 & 2 are identical.
 * </pre>
 */
bool
LineLineIntersect (LineType *Line1, LineType *Line2, Coord bloat)
{
  double s, r;
  double line1_dx, line1_dy, line2_dx, line2_dy,
         point1_dx, point1_dy;
  if (TEST_FLAG (SQUAREFLAG, Line1))/* pretty reckless recursion */
    {
      PointType p[4];
      form_slanted_rectangle (p, Line1);
      return IsLineInQuadrangle (p, Line2, bloat);
    }
  /* here come only round Line1 because IsLineInQuadrangle()
     calls LineLineIntersect() with first argument rounded*/
  if (TEST_FLAG (SQUAREFLAG, Line2))
    {
      PointType p[4];
      form_slanted_rectangle (p, Line2);
      return IsLineInQuadrangle (p, Line1, bloat);
    }
  /* now all lines are round */

  /* Check endpoints: this provides a quick exit, catches
   *  cases where the "real" lines don't intersect but the
   *  thick lines touch, and ensures that the dx/dy business
   *  below does not cause a divide-by-zero. */
  if (IsPointInPad (Line2->Point1.X, Line2->Point1.Y,
                    MAX (Line2->Thickness / 2 + bloat, 0),
                    (PadType *) Line1)
       || IsPointInPad (Line2->Point2.X, Line2->Point2.Y,
                        MAX (Line2->Thickness / 2 + bloat, 0),
                        (PadType *) Line1)
       || IsPointInPad (Line1->Point1.X, Line1->Point1.Y,
                        MAX (Line1->Thickness / 2 + bloat, 0),
                        (PadType *) Line2)
       || IsPointInPad (Line1->Point2.X, Line1->Point2.Y,
                        MAX (Line1->Thickness / 2 + bloat, 0),
                        (PadType *) Line2))
    return true;

  /* setup some constants */
  line1_dx = Line1->Point2.X - Line1->Point1.X;
  line1_dy = Line1->Point2.Y - Line1->Point1.Y;
  line2_dx = Line2->Point2.X - Line2->Point1.X;
  line2_dy = Line2->Point2.Y - Line2->Point1.Y;
  point1_dx = Line1->Point1.X - Line2->Point1.X;
  point1_dy = Line1->Point1.Y - Line2->Point1.Y;

  /* If either line is a point, we have failed already, since the
   *   endpoint check above will have caught an "intersection". */
  if ((line1_dx == 0 && line1_dy == 0)
      || (line2_dx == 0 && line2_dy == 0))
    return false;

  /* set s to cross product of Line1 and the line
   *   Line1.Point1--Line2.Point1 (as vectors) */
  s = point1_dy * line1_dx - point1_dx * line1_dy;

  /* set r to cross product of both lines (as vectors) */
  r = line1_dx * line2_dy - line1_dy * line2_dx;

  /* No cross product means parallel lines, or maybe Line2 is
   *  zero-length. In either case, since we did a bounding-box
   *  check before getting here, the above IsPointInPad() checks
   *  will have caught any intersections. */
  if (r == 0.0)
    return false;

  s /= r;
  r = (point1_dy * line2_dx - point1_dx * line2_dy) / r;

  /* intersection is at least on AB */
  if (r >= 0.0 && r <= 1.0)
    return (s >= 0.0 && s <= 1.0);

  /* intersection is at least on CD */
  /* [removed this case since it always returns false --asp] */
  return false;
}

/*!
 * \brief Check for line intersection with an arc.
 *
 * Mostly this is like the circle/line intersection
 * found in IsPointOnLine (search.c) see the detailed
 * discussion for the basics there.
 *
 * Since this is only an arc, not a full circle we need
 * to find the actual points of intersection with the
 * circle, and see if they are on the arc.
 *
 * To do this, we translate along the line from the point Q
 * plus or minus a distance delta = sqrt(Radius^2 - d^2)
 * but it's handy to normalize with respect to l, the line
 * length so a single projection is done (e.g. we don't first
 * find the point Q.
 *
 * <pre>
 * The projection is now of the form:
 *
 *      Px = X1 + (r +- r2)(X2 - X1)
 *      Py = Y1 + (r +- r2)(Y2 - Y1)
 * </pre>
 *
 * Where r2 sqrt(Radius^2 l^2 - d^2)/l^2
 * note that this is the variable d, not the symbol d described in
 * IsPointOnLine (variable d = symbol d * l).
 *
 * The end points are hell so they are checked individually.
 */
bool
LineArcIntersect (LineType *Line, ArcType *Arc, Coord bloat)
{
  double dx, dy, dx1, dy1, l, d, r, r2, Radius;
  BoxType *box;

  dx = Line->Point2.X - Line->Point1.X;
  dy = Line->Point2.Y - Line->Point1.Y;
  dx1 = Line->Point1.X - Arc->X;
  dy1 = Line->Point1.Y - Arc->Y;
  l = dx * dx + dy * dy;
  d = dx * dy1 - dy * dx1;
  d *= d;

  /* use the larger diameter circle first */
  Radius =
    Arc->Width + MAX (0.5 * (Arc->Thickness + Line->Thickness) + bloat, 0.0);
  Radius *= Radius;
  r2 = Radius * l - d;
  /* projection doesn't even intersect circle when r2 < 0 */
  if (r2 < 0)
    return (false);
  /* check the ends of the line in case the projected point */
  /* of intersection is beyond the line end */
  if (IsPointOnArc
      (Line->Point1.X, Line->Point1.Y,
       MAX (0.5 * Line->Thickness + bloat, 0.0), Arc))
    return (true);
  if (IsPointOnArc
      (Line->Point2.X, Line->Point2.Y,
       MAX (0.5 * Line->Thickness + bloat, 0.0), Arc))
    return (true);
  if (l == 0.0)
    return (false);
  r2 = sqrt (r2);
  Radius = -(dx * dx1 + dy * dy1);
  r = (Radius + r2) / l;
  if (r >= 0 && r <= 1
      && IsPointOnArc (Line->Point1.X + r * dx,
                       Line->Point1.Y + r * dy,
                       MAX (0.5 * Line->Thickness + bloat, 0.0), Arc))
    return (true);
  r = (Radius - r2) / l;
  if (r >= 0 && r <= 1
      && IsPointOnArc (Line->Point1.X + r * dx,
                       Line->Point1.Y + r * dy,
                       MAX (0.5 * Line->Thickness + bloat, 0.0), Arc))
    return (true);
  /* check arc end points */
  box = GetArcEnds (Arc);
  if (IsPointInPad (box->X1, box->Y1, Arc->Thickness * 0.5 + bloat, (PadType *)Line))
    return true;
  if (IsPointInPad (box->X2, box->Y2, Arc->Thickness * 0.5 + bloat, (PadType *)Line))
    return true;
  return false;
}

bool
PinLineIntersect (PinType *PV, LineType *Line, Coord bloat)
{
  /* IsLineInRectangle already has Bloat factor */
  return TEST_FLAG (SQUAREFLAG,
                    PV) ? IsLineInRectangle (PV->X - (PIN_SIZE (PV) + 1) / 2,
                                             PV->Y - (PIN_SIZE (PV) + 1) / 2,
                                             PV->X + (PIN_SIZE (PV) + 1) / 2,
                                             PV->Y + (PIN_SIZE (PV) + 1) / 2,
                                             Line, bloat) : IsPointInPad (PV->X,
                                                                    PV->Y,
								   MAX (PIN_SIZE (PV)
                                                                         /
                                                                         2.0 +
                                                                         bloat,
                                                                         0.0),
                                                                    (PadType *)Line);
}

bool
BoxBoxIntersection (BoxType *b1, BoxType *b2)
{
  if (b2->X2 < b1->X1 || b2->X1 > b1->X2)
    return false;
  if (b2->Y2 < b1->Y1 || b2->Y1 > b1->Y2)
    return false;
  return true;
}

bool
PadPadIntersect (PadType *p1, PadType *p2, Coord bloat)
{
  return LinePadIntersect ((LineType *) p1, p2, bloat);
}


bool
PinPinIntersect(PinType *PV1, PinType *PV2, Coord bloat)
{
  double t1, t2;
  BoxType b1, b2;

  t1 = MAX (PV1->Thickness / 2.0 + bloat, 0);
  t2 = MAX (PV2->Thickness / 2.0 + bloat, 0);
  if (IsPointOnPin (PV1->X, PV1->Y, t1, PV2)
      || IsPointOnPin (PV2->X, PV2->Y, t2, PV1))
    return true;
  if (!TEST_FLAG (SQUAREFLAG, PV1) || !TEST_FLAG (SQUAREFLAG, PV2))
    return false;
  /* check for square/square overlap */
  b1.X1 = PV1->X - t1;
  b1.X2 = PV1->X + t1;
  b1.Y1 = PV1->Y - t1;
  b1.Y2 = PV1->Y + t1;
  t2 = PV2->Thickness / 2.0;
  b2.X1 = PV2->X - t2;
  b2.X2 = PV2->X + t2;
  b2.Y1 = PV2->Y - t2;
  b2.Y2 = PV2->Y + t2;
  return BoxBoxIntersection (&b1, &b2);
}

bool
LinePadIntersect (LineType *Line, PadType *Pad, Coord bloat)
{
  return LineLineIntersect ((Line), (LineType *)Pad, bloat);
}

bool
ArcPadIntersect (ArcType *Arc, PadType *Pad, Coord bloat)
{
  return LineArcIntersect ((LineType *) (Pad), (Arc), bloat);
}


/*!
 * \brief Reduce arc start angle and delta to 0..360.
 */
static void
normalize_angles (Angle *sa, Angle *d)
{
  if (*d < 0)
    {
      *sa += *d;
      *d = - *d;
    }
  if (*d > 360) /* full circle */
    *d = 360;
  *sa = NormalizeAngle (*sa);
}

static int
radius_crosses_arc (double x, double y, ArcType *arc)
{
  double alpha = atan2 (y - arc->Y, -x + arc->X) * RAD_TO_DEG;
  Angle sa = arc->StartAngle, d = arc->Delta;

  normalize_angles (&sa, &d);
  if (alpha < 0)
    alpha += 360;
  if (sa <= alpha)
    return (sa + d) >= alpha;
  return (sa + d - 360) >= alpha;
}

/*!
 * \brief Check if two arcs intersect.
 *
 * First we check for circle intersections,
 * then find the actual points of intersection
 * and test them to see if they are on arcs.
 *
 * Consider a, the distance from the center of arc 1
 * to the point perpendicular to the intersecting points.
 *
 * \ta = (r1^2 - r2^2 + l^2)/(2l)
 *
 * The perpendicular distance to the point of intersection
 * is then:
 *
 * \td = sqrt(r1^2 - a^2)
 *
 * The points of intersection would then be:
 *
 * \tx = X1 + a/l dx +- d/l dy
 *
 * \ty = Y1 + a/l dy -+ d/l dx
 *
 * Where dx = X2 - X1 and dy = Y2 - Y1.
 */
bool
ArcArcIntersect (ArcType *Arc1, ArcType *Arc2, Coord bloat)
{
  double x, y, dx, dy, r1, r2, a, d, l, t, t1, t2, dl;
  Coord pdx, pdy;
  Coord box[8];

  t  = 0.5 * Arc1->Thickness + bloat;
  t2 = 0.5 * Arc2->Thickness;
  t1 = t2 + bloat;

  /* too thin arc */
  if (t < 0 || t1 < 0)
    return false;

  /* try the end points first */
  get_arc_ends (&box[0], Arc1);
  get_arc_ends (&box[4], Arc2);
  if (IsPointOnArc (box[0], box[1], t, Arc2)
      || IsPointOnArc (box[2], box[3], t, Arc2)
      || IsPointOnArc (box[4], box[5], t, Arc1)
      || IsPointOnArc (box[6], box[7], t, Arc1))
    return true;

  pdx = Arc2->X - Arc1->X;
  pdy = Arc2->Y - Arc1->Y;
  dl = Distance (Arc1->X, Arc1->Y, Arc2->X, Arc2->Y);
  /* concentric arcs, simpler intersection conditions */
  if (dl < 0.5)
    {
      if ((Arc1->Width - t >= Arc2->Width - t2
           && Arc1->Width - t <= Arc2->Width + t2)
          || (Arc1->Width + t >= Arc2->Width - t2
              && Arc1->Width + t <= Arc2->Width + t2))
        {
	  Angle sa1 = Arc1->StartAngle, d1 = Arc1->Delta;
	  Angle sa2 = Arc2->StartAngle, d2 = Arc2->Delta;
	  /* NB the endpoints have already been checked,
	     so we just compare the angles */

	  normalize_angles (&sa1, &d1);
	  normalize_angles (&sa2, &d2);
	  /* sa1 == sa2 was caught when checking endpoints */
	  if (sa1 > sa2)
            if (sa1 < sa2 + d2 || sa1 + d1 - 360 > sa2)
              return true;
	  if (sa2 > sa1)
	    if (sa2 < sa1 + d1 || sa2 + d2 - 360 > sa1)
              return true;
        }
      return false;
    }
  r1 = Arc1->Width;
  r2 = Arc2->Width;
  /* arcs centerlines are too far or too near */
  if (dl > r1 + r2 || dl + r1 < r2 || dl + r2 < r1)
    {
      /* check the nearest to the other arc's center point */
      dx = pdx * r1 / dl;
      dy = pdy * r1 / dl;
      if (dl + r1 < r2) /* Arc1 inside Arc2 */
	{
	  dx = - dx;
	  dy = - dy;
	}

      if (radius_crosses_arc (Arc1->X + dx, Arc1->Y + dy, Arc1)
	  && IsPointOnArc (Arc1->X + dx, Arc1->Y + dy, t, Arc2))
	return true;

      dx = - pdx * r2 / dl;
      dy = - pdy * r2 / dl;
      if (dl + r2 < r1) /* Arc2 inside Arc1 */
	{
	  dx = - dx;
	  dy = - dy;
	}

      if (radius_crosses_arc (Arc2->X + dx, Arc2->Y + dy, Arc2)
	  && IsPointOnArc (Arc2->X + dx, Arc2->Y + dy, t1, Arc1))
	return true;
      return false;
    }

  l = dl * dl;
  r1 *= r1;
  r2 *= r2;
  a = 0.5 * (r1 - r2 + l) / l;
  r1 = r1 / l;
  d = r1 - a * a;
  /* the circles are too far apart to touch or probably just touch:
     check the nearest point */
  if (d < 0)
    d = 0;
  else
    d = sqrt (d);
  x = Arc1->X + a * pdx;
  y = Arc1->Y + a * pdy;
  dx = d * pdx;
  dy = d * pdy;
  if (radius_crosses_arc (x + dy, y - dx, Arc1)
      && IsPointOnArc (x + dy, y - dx, t, Arc2))
    return true;
  if (radius_crosses_arc (x + dy, y - dx, Arc2)
      && IsPointOnArc (x + dy, y - dx, t1, Arc1))
    return true;

  if (radius_crosses_arc (x - dy, y + dx, Arc1)
      && IsPointOnArc (x - dy, y + dx, t, Arc2))
    return true;
  if (radius_crosses_arc (x - dy, y + dx, Arc2)
      && IsPointOnArc (x - dy, y + dx, t1, Arc1))
    return true;
  return false;
}

